/*
 * hd2_pcm_stream.cpp -- CPU -> codec-DAC PCM streaming experiment for the HD2
 * threaded build (task 2026-06-10-hd2-pcm-stream-voice-prompts).
 *
 * Recipe (docs/pcm_stream_playback.md, RE'd from vendor v2.1.3):
 *   - playback window = SAHB 0x180000a0: 80 x s16 @ 8 kHz, one frame / 10 ms
 *   - PIC source 0x1b (vec 0x3b) fires when the modem wants the next frame
 *   - handler: SOCSYS_INT_STATUS |= 0x20 (frame-supplied handshake), then
 *     copy the next 80 samples into the window (vendor pcm_isr_rd_body order)
 *   - arming: pcm_mode = 3, voice_path bit0 + 0x20, SOFT_RSTN bits 3/4 released
 *
 * The open question this op answers on HW: does the 10 ms PCM IRQ fire outside
 * the vendor's DMR-voice mode?  The reply line reports the IRQ count:
 * 0 + silence  -> more arming needed (mode FSM / dead clock -- poke via 'W');
 * ~100/s + tone -> the recipe is complete, go build the outputStream driver.
 *
 * The arm mask makes the enable steps host-selectable so arming variants can
 * be A/B'd live without a reflash (0 = all steps).
 */

#include <miosix.h>
#include <interfaces/interrupts.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "hd2_regs.h"
#include "core/audio_path.h"
#include "core/audio_stream.h"
#include "core/voicePrompts.h"
#include "core/voicePromptUtils.h"
#include "core/state.h"

/* Codec DAC->lineout->speaker warm-up + rf_freeze guard (radio_HD2.cpp);
 * same pair platform_beepStart() uses to make the PWM beep audible. */
extern "C" void hd2_audio_out_warm(void);
extern "C" volatile uint32_t g_rf_freeze;

using namespace miosix;

namespace {

int16_t toneFrame[PCM_FRAME_SAMPLES];
volatile uint32_t pcmIrqCount = 0;

/* Vec-0x3b handler: handshake first, then supply the frame (vendor order).
 * The tone frequency is rounded to a multiple of 100 Hz so one 80-sample
 * frame is integer-periodic -- no phase carry between frames, the same
 * static frame repeats. Runs with IE off in the PIC dispatcher: 80 halfword
 * stores, well under budget. */
void pcmPlayIsr(void *)
{
    SOCSYS_INT_STATUS |= INT_STATUS_PCM_PLAY_ACK;
    volatile uint16_t *dst = SAHB_PCM_PLAY;
    for(unsigned i = 0; i < PCM_FRAME_SAMPLES; ++i)
        dst[i] = static_cast<uint16_t>(toneFrame[i]);
    ++pcmIrqCount;
}

} // namespace

/* Stream a sine tone through the SAHB PCM playback window for ms milliseconds.
 * arm-mask bits (0 -> all): 1 = release SOFT_RSTN PCM bits, 2 = pcm_mode=3,
 * 4 = voice_path PCM-bridge bit0, 8 = voice_path playback side (0x20).
 * Formats the result line ("PCM irqs=... vp=... pm=... rstn=...") into out. */
extern "C" void hd2_pcm_tone(uint16_t freq_hz, uint16_t ms, uint8_t arm,
                             char *out, unsigned outsz)
{
    if(freq_hz == 0u) freq_hz = 1000u;
    if(ms == 0u)      ms = 1000u;
    if(ms > 10000u)   ms = 10000u;        /* keep the diag thread responsive */
    if(arm == 0u)     arm = 0x0fu;

    /* Integer cycles per 80-sample frame -> actual freq = cycles * 100 Hz. */
    unsigned cycles = (freq_hz + 50u) / 100u;
    if(cycles == 0u) cycles = 1u;
    if(cycles > 39u) cycles = 39u;        /* Nyquist at 8 kHz */
    for(unsigned i = 0; i < PCM_FRAME_SAMPLES; ++i)
        toneFrame[i] = static_cast<int16_t>(
            8000.0f * sinf(6.2831853f * cycles * i / PCM_FRAME_SAMPLES));

    /* Open the speaker route (codec warm + diplex unmute + PB4/PB10 low),
     * exactly like platform_beepStart -- unless a host experiment holds the
     * audio GPIOs frozen. */
    if(g_rf_freeze == 0u)
    {
        hd2_audio_out_warm();
        SOCSYS_IO_DIPLEX0 &= ~DIPLEX0_AUDIO_MUTE;
        GPIOB_DDR |=  (SPKR_AMP_BIT | AUDIO_ROUTE_BIT);
        GPIOB_DR  &= ~(SPKR_AMP_BIT | AUDIO_ROUTE_BIT);
    }

    uint32_t vpSave = SOCSYS_VOICE_PATH;
    uint32_t pmSave = SOCSYS_PCM_MODE;    /* may read 0: write-only/mode-gated */
    if(arm & 1u) SOCSYS_SYS_SOFT_RSTN |= SOFT_RSTN_PCM_BITS;
    if(arm & 2u) SOCSYS_PCM_MODE = 3u;
    if(arm & 4u) SOCSYS_VOICE_PATH |= VOICE_PATH_PCM_EN;
    if(arm & 8u) SOCSYS_VOICE_PATH |= VOICE_PATH_PLAY;

    pcmIrqCount = 0;
    {
        GlobalIrqLock lock;
        IRQregisterIrq(lock, HD2_IRQ_PCM_PLAY, &pcmPlayIsr, nullptr);
    }

    /* Prime: supply the first frame + handshake, mirroring the vendor's
     * modem_pcm_path_enable (which acks once at enable time). */
    for(unsigned i = 0; i < PCM_FRAME_SAMPLES; ++i)
        SAHB_PCM_PLAY[i] = static_cast<uint16_t>(toneFrame[i]);
    SOCSYS_INT_STATUS |= INT_STATUS_PCM_PLAY_ACK;

    Thread::sleep(ms);

    {
        GlobalIrqLock lock;
        IRQunregisterIrq(lock, HD2_IRQ_PCM_PLAY, &pcmPlayIsr, nullptr);
    }

    uint32_t vpNow   = SOCSYS_VOICE_PATH;
    uint32_t pmNow   = SOCSYS_PCM_MODE;
    uint32_t rstnNow = SOCSYS_SYS_SOFT_RSTN;

    /* Restore the path state and re-mute (beepStop semantics). */
    SOCSYS_VOICE_PATH = vpSave;
    if(arm & 2u) SOCSYS_PCM_MODE = pmSave;
    if(g_rf_freeze == 0u)
    {
        GPIOB_DR |= SPKR_AMP_BIT;
        SOCSYS_IO_DIPLEX0 |= DIPLEX0_AUDIO_MUTE;
    }

    snprintf(out, outsz, "PCM irqs=%lu vp=%08lx pm=%08lx rstn=%08lx\r\n",
             (unsigned long)pcmIrqCount, (unsigned long)vpNow,
             (unsigned long)pmNow, (unsigned long)rstnNow);
}

/* Capture probe (diag 'O'): arm the codec ADC -> SAHB PCM capture path and
 * sample the capture window, reporting peak-to-peak amplitude so we can see on
 * the bench whether received (or mic) audio is landing in the buffer.  The
 * modem FM-RX path is already set at boot (hd2_modem_fm_boot_init); this only
 * arms the SoC capture side and services the per-frame handshake.
 * arm-mask bits (0 -> all): 1 = release SOFT_RSTN PCM bits, 2 = pcm_mode=3,
 * 4 = voice_path PCM-bridge bit0, 8 = voice_path capture side (VOICE_PATH_CAP).
 * Touches only SoC PCM/voice-path regs (no AT1846S / modem retune); restored
 * on exit.  Leaves RX running (no rf_freeze) so live received audio is present. */
extern "C" void hd2_pcm_capture(uint16_t ms, uint8_t arm, char *out, unsigned outsz)
{
    if(ms == 0u)    ms = 200u;
    if(ms > 10000u) ms = 10000u;          /* keep the diag thread responsive */
    if(arm == 0u)   arm = 0x0fu;

    uint32_t vpSave = SOCSYS_VOICE_PATH;
    uint32_t pmSave = SOCSYS_PCM_MODE;
    if(arm & 1u) SOCSYS_SYS_SOFT_RSTN |= SOFT_RSTN_PCM_BITS;
    if(arm & 2u) SOCSYS_PCM_MODE = 3u;
    if(arm & 4u) SOCSYS_VOICE_PATH |= VOICE_PATH_PCM_EN;
    if(arm & 8u) SOCSYS_VOICE_PATH |= VOICE_PATH_CAP;

    volatile uint16_t *src = SAHB_PCM_CAP;
    int16_t  lo = 32767, hi = -32768;
    uint32_t frames = ms / 10u; if(frames == 0u) frames = 1u;
    for(uint32_t f = 0; f < frames; ++f)
    {
        Thread::sleep(10);                 /* one 80-sample frame / 10 ms @ 8 kHz */
        for(unsigned i = 0; i < PCM_FRAME_SAMPLES; ++i)
        {
            int16_t s = (int16_t)src[i];
            if(s < lo) lo = s;
            if(s > hi) hi = s;
        }
        SOCSYS_INT_STATUS |= INT_STATUS_PCM_CAP_ACK;   /* ack -> advance frame */
    }

    uint32_t vpNow = SOCSYS_VOICE_PATH, pmNow = SOCSYS_PCM_MODE;
    SOCSYS_VOICE_PATH = vpSave;
    if(arm & 2u) SOCSYS_PCM_MODE = pmSave;

    snprintf(out, outsz,
             "CAP pp=%d lo=%d hi=%d vp=%08lx pm=%08lx s=%04x %04x %04x %04x\r\n",
             (int)(hi - lo), (int)lo, (int)hi,
             (unsigned long)vpNow, (unsigned long)pmNow,
             (unsigned)src[0], (unsigned)src[1], (unsigned)src[2], (unsigned)src[3]);
}

/* APRS RX (diag 'w'): wait for a carrier (AT1846S RSSI rise), then stream the
 * 8 kHz demod-audio frames through the OpenRTX APRS protocol module
 * (Aprs::Decoder) -- O(1) memory, no large buffer.  Reports the first decoded
 * frame, or a status line.  <frames> = how long to decode after the carrier
 * (x10 ms).  Hardware glue only; the modem itself lives in openrtx/protocols. */
extern "C" uint16_t hd2_at1846s_read(uint8_t reg);
extern "C" void     hd2_at1846s_write(uint8_t reg, uint16_t val);
extern "C" void hd2_txpower_levels(uint16_t *apc, uint16_t *padrv);  /* UI power level -> APC + padrv */
#include "protocols/APRS/Decoder.hpp"
#include "protocols/APRS/Encoder.hpp"
static Aprs::Decoder g_aprsDec;     /* ~400 B decoder state (not on the stack) */
static Aprs::Encoder g_aprsEnc;     /* ~1.3 KB encoder state (not on the stack) */

extern "C" void hd2_aprs_rx(uint16_t frames, char *out, unsigned outsz)
{
    if(frames == 0u || frames > 600u) frames = 200u;   /* default ~2 s decode  */

    uint32_t vpSave = SOCSYS_VOICE_PATH;
    SOCSYS_SYS_SOFT_RSTN |= SOFT_RSTN_PCM_BITS;
    SOCSYS_PCM_MODE = 3u;
    SOCSYS_VOICE_PATH |= VOICE_PATH_PCM_EN | VOICE_PATH_CAP;

    /* Flatten the AT1846S audio for data: bypass the voice low/high-pass
     * filters + de-emphasis (reg 0x58 bits 5/6/7) so the 1200/2200 Hz AFSK
     * tones both survive (HW-confirmed 2026-06-13: this restores the 2200 Hz
     * space tone that the voice filtering otherwise rolls off). */
    uint16_t r58 = hd2_at1846s_read(0x58);
    hd2_at1846s_write(0x58, (uint16_t)(r58 | 0x00E0u));

    /* RSSI baseline (AT1846S reg 0x1B upper byte; dBm = -137 + x). */
    int base = 0;
    for(int i = 0; i < 8; ++i) { base += (int)(hd2_at1846s_read(0x1B) >> 8); Thread::sleep(5); }
    base /= 8;

    /* Wait up to ~8 s for a carrier (RSSI jump >= ~8 dB). */
    bool trig = false;
    for(int t = 0; t < 1600; ++t)
    {
        if((int)(hd2_at1846s_read(0x1B) >> 8) > base + 8) { trig = true; break; }
        Thread::sleep(5);
    }

    /* Stream-decode the demod audio, frame by frame.  buf/line are static to
     * keep the diag thread's small stack out of trouble. */
    g_aprsDec.reset();
    volatile uint16_t *src = SAHB_PCM_CAP;
    static int16_t buf[PCM_FRAME_SAMPLES];
    static char    line[256];
    bool     got = false;
    int16_t  lo = 32767, hi = -32768;
    uint16_t f;
    for(f = 0; f < frames && !got; ++f)
    {
        Thread::sleep(10);
        for(unsigned i = 0; i < PCM_FRAME_SAMPLES; ++i)
        {
            int16_t s = (int16_t)src[i];
            buf[i] = s;
            if(s < lo) lo = s;
            if(s > hi) hi = s;
        }
        SOCSYS_INT_STATUS |= INT_STATUS_PCM_CAP_ACK;
        if(g_aprsDec.process(buf, PCM_FRAME_SAMPLES, line, (int)sizeof line))
            got = true;
    }

    hd2_at1846s_write(0x58, r58);          /* restore the voice filters */
    SOCSYS_VOICE_PATH = vpSave;
    if(got) { snprintf(out, outsz, "APRS: %s\r\n", line); return; }

    /* Near-miss diagnosis: status + hex of the longest assembled frame, so the
     * host can diff it against the known packet to localise the bit errors. */
    int w = snprintf(out, outsz,
                     "APRS none (trig=%d pp=%d flags=%d max=%d) F=",
                     trig ? 1 : 0, (int)(hi - lo),
                     g_aprsDec.flagsSeen(), g_aprsDec.maxFrameLen());
    const uint8_t *fr = g_aprsDec.lastRawFrame();
    int frlen = g_aprsDec.lastRawLen();
    for(int i = 0; i < frlen && w < (int)outsz - 3; ++i)
        w += snprintf(out + w, outsz - w, "%02x", fr[i]);
    snprintf(out + w, (w < (int)outsz) ? outsz - w : 0, "\r\n");
}

/* vec-0x3b playback ISR for APRS TX: handshake, then fill the SAHB playback
 * window with the encoder's next 80-sample frame (vendor pcm_isr_rd order).
 * Sets g_aprsTxDone once the encoder drains (a fully-silent frame). */
static volatile bool     g_aprsTxDone   = false;
static volatile uint32_t g_aprsTxFrames = 0;

static void aprsPlayIsr(void *)
{
    SOCSYS_INT_STATUS |= INT_STATUS_PCM_PLAY_ACK;
    volatile uint16_t *dst = SAHB_PCM_PLAY;
    bool any = false;
    for(unsigned i = 0; i < PCM_FRAME_SAMPLES; ++i)
    {
        int16_t s = 0;
        if(g_aprsEnc.nextSample(s)) any = true;
        dst[i] = static_cast<uint16_t>(s);
    }
    ++g_aprsTxFrames;
    if(!any) g_aprsTxDone = true;
}

/* APRS TX (diag 'B'): build an AX.25 UI beacon, key the proven analog-FM TX
 * carrier (the same path normal PTT uses -- NOT the modem TX RAM, which is the
 * DMR/digital path and does not modulate analog FM: 2026-06-13 the tx_voice_
 * source RAM bit produced a dead/UNMODULATED carrier), and stream the AFSK as
 * audio through the CODEC PLAYBACK window (SAHB 0x180000a0, vec-0x3b ISR --
 * the vendor voice-prompt-over-air mechanism), routed into the FM modulator.
 *
 * The voice-path routing bits at 0x11000080 have conflicting RE/manual
 * meanings, so they're <flags>-selectable for live A/B (no reflash):
 *   bit0 -> 0x01 (PCM bridge / "tx_voice_source")   bit1 -> 0x20 (PLAY)
 *   bit2 -> 0x40 (fm_play_ctrl, "select FM path")    bit3 -> 0x08 (sidetone)
 *   bit4 -> drop PTB3 mic gate (mic OFF -> playback-only modulation)
 * Power follows the UI level (Settings->Radio->TX Power); keep it low (full
 * power at high duty cooked a final 2026-06-13).
 * <preamble u8> leading 0x7E flags (0->48, ~320 ms TXDELAY); <flags u8> routing
 * (0 -> 0x16 = PLAY|fm_play_ctrl|mic-off). */
extern "C" void hd2_aprs_tx(uint8_t preamble, uint8_t flags, char *out, unsigned outsz)
{
    if(preamble == 0u) preamble = 48u;
    if(flags == 0u)    flags = 0x16u;

    g_aprsEnc.buildBeacon("APZHD2", 0, "AI5QZ", 7, "WIDE1", 1,
                          "!4807.00N/01131.00E>HD2 APRS TX", preamble);

    volatile uint32_t *wm       = reinterpret_cast<volatile uint32_t*>(0x11000100u);
    volatile uint32_t *ptt      = reinterpret_cast<volatile uint32_t*>(0x11000560u);
    volatile uint32_t *imask    = reinterpret_cast<volatile uint32_t*>(0x1100039cu);
    volatile uint32_t *audioctl = reinterpret_cast<volatile uint32_t*>(0x11000080u);

    uint32_t freeze0 = g_rf_freeze;
    g_rf_freeze = 1;                        /* rtx thread off the bus while we TX */

    /* ---- Carrier: the proven analog-FM TX path (normal-PTT equivalent) ---- */
    uint32_t wm0 = *wm, imask0 = *imask, audioctl0 = *audioctl;
    *imask = 0x0001007fu;                   /* vendor FM SYS_INTERP_MASK          */
    *wm    = wm0 | 0x80u;                    /* FM analog modulator mode           */
    *ptt   = 1u;                             /* modem FM TX on                     */

    uint16_t apc, padrv;
    hd2_txpower_levels(&apc, &padrv);        /* UI-selected TX power level         */
    uint32_t gpiob0 = GPIOB_DR;
    GPIOB_DR |= (1u << 19);                  /* band-select high (430 MHz)         */
    if(flags & 0x10u) GPIOB_DR &= ~(1u << 3);/* mic gate OFF (playback-only mod)   */
    else              GPIOB_DR |=  (1u << 3);/* mic gate ON  (vendor TX parity)    */
    DAC_PD_MODE_EN &= ~0x2u;
    DAC_PD_CTRL    &= ~0x2u;
    DAC_DATA_B      = apc;

    uint16_t r40 = hd2_at1846s_read(0x40);
    uint16_t r0a = hd2_at1846s_read(0x0a);
    uint16_t r59 = hd2_at1846s_read(0x59);

    /* HD2 TX cal offset (+12.5 kHz = +200 freq-word units on 0x29/0x2A). */
    uint16_t fhi = hd2_at1846s_read(0x29);
    uint16_t flo = hd2_at1846s_read(0x2A);
    uint32_t ftx = ((((uint32_t)fhi << 16) | flo) + 200u);
    hd2_at1846s_write(0x29, (uint16_t)((ftx >> 16) & 0xffffu));
    hd2_at1846s_write(0x2A, (uint16_t)(ftx & 0xffffu));

    hd2_at1846s_write(0x0a, (uint16_t)((padrv << 11) | 0x0420u));
    hd2_at1846s_write(0x59, 0x0C50u);       /* wideband FM deviation              */
    hd2_at1846s_write(0x40, 0x0030u);
    hd2_at1846s_write(0x30, 0x4006u);
    hd2_at1846s_write(0x30, 0x4046u);       /* tx_on: key the carrier             */
    hd2_at1846s_write(0x30, 0x40c6u);       /* + bit7: PA-stage on                */

    /* ---- Modulation: stream the AFSK through the codec playback path ---- */
    uint32_t route = 0u;
    if(flags & 0x01u) route |= 0x01u;       /* PCM bridge                         */
    if(flags & 0x02u) route |= 0x20u;       /* VOICE_PATH PLAY                    */
    if(flags & 0x04u) route |= 0x40u;       /* fm_play_ctrl (select FM path)      */
    if(flags & 0x08u) route |= 0x08u;       /* fm_sidetone                        */
    uint32_t pmSave = SOCSYS_PCM_MODE;
    SOCSYS_SYS_SOFT_RSTN |= SOFT_RSTN_PCM_BITS;
    SOCSYS_PCM_MODE = 3u;
    *audioctl = audioctl0 | route;

    g_aprsTxDone = false; g_aprsTxFrames = 0;
    {
        GlobalIrqLock lock;
        IRQregisterIrq(lock, HD2_IRQ_PCM_PLAY, &aprsPlayIsr, nullptr);
    }
    /* Prime the first frame + handshake (vendor modem_pcm_path_enable order). */
    {
        volatile uint16_t *dst = SAHB_PCM_PLAY;
        for(unsigned i = 0; i < PCM_FRAME_SAMPLES; ++i)
        {
            int16_t s = 0; g_aprsEnc.nextSample(s);
            dst[i] = static_cast<uint16_t>(s);
        }
    }
    SOCSYS_INT_STATUS |= INT_STATUS_PCM_PLAY_ACK;

    /* Wait for the beacon to clock out (ISR sets done) or a ~3 s safety bound. */
    int guard;
    for(guard = 0; guard < 600 && !g_aprsTxDone; ++guard)
        Thread::sleep(5);

    {
        GlobalIrqLock lock;
        IRQunregisterIrq(lock, HD2_IRQ_PCM_PLAY, &aprsPlayIsr, nullptr);
    }

    /* ---- dekey + restore ---- */
    hd2_at1846s_write(0x30, 0x4006u);       /* dekey (clears PA bit7)             */
    DAC_DATA_B      = 0u;
    DAC_PD_CTRL    |= 0x2u;
    *ptt      = 0u;
    *wm       = wm0;
    *audioctl = audioctl0;
    *imask    = imask0;
    SOCSYS_PCM_MODE = pmSave;
    GPIOB_DR  = gpiob0;
    hd2_at1846s_write(0x29, fhi);
    hd2_at1846s_write(0x2A, flo);
    hd2_at1846s_write(0x59, r59);
    hd2_at1846s_write(0x0a, r0a);
    hd2_at1846s_write(0x40, (r40 != 0u) ? r40 : 0x0031u);
    hd2_at1846s_write(0x30, 0x4826u);       /* back to RX-on                      */
    g_rf_freeze = freeze0;

    snprintf(out, outsz, "APRSTX pre=%u flags=%02x frames=%lu apc=%03x done=%d\r\n",
             preamble, flags, (unsigned long)g_aprsTxFrames, (unsigned)apc,
             g_aprsTxDone ? 1 : 0);
}

/* Full-stack stream test: tone via audioPath_request -> audioStream_start ->
 * outputStream_HD2 driver (the proper OpenRTX route, vs. hd2_pcm_tone's raw
 * register poke).  Refills the idle half on every sync like a real producer
 * (audio_codec.c decodeFunc pattern).  Reports path/stream ids + sync count. */
extern "C" void hd2_stream_tone(uint16_t freq_hz, uint16_t ms,
                                char *out, unsigned outsz)
{
    static int16_t streamBuf[320];     /* two 160-sample halves, 20 ms each */

    if(freq_hz == 0u) freq_hz = 1000u;
    if(ms == 0u)      ms = 1000u;
    if(ms > 10000u)   ms = 10000u;

    unsigned cycles = (freq_hz + 50u) / 100u;   /* per 80-sample frame */
    if(cycles == 0u) cycles = 1u;
    if(cycles > 39u) cycles = 39u;
    for(unsigned i = 0; i < 320u; ++i)
        streamBuf[i] = static_cast<int16_t>(
            8000.0f * sinf(6.2831853f * cycles * i / 80.0f));

    pathId path = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_PROMPT);
    if(path < 0)
    {
        snprintf(out, outsz, "STREAM path_err=%d\r\n", (int)path);
        return;
    }

    streamId sid = audioStream_start(path, streamBuf, 320, 8000,
                                     STREAM_OUTPUT | BUF_CIRC_DOUBLE);
    if(sid < 0)
    {
        audioPath_release(path);
        snprintf(out, outsz, "STREAM stream_err=%d\r\n", (int)sid);
        return;
    }

    /* Each sync = one 160-sample half = 20 ms. */
    unsigned syncs = 0;
    const unsigned target = ms / 20u;
    while(syncs < target)
    {
        if(outputStream_sync(sid, false) == false)
            break;
        ++syncs;

        /* Real-producer motion: rewrite the idle half (same tone). */
        stream_sample_t *idle = outputStream_getIdleBuffer(sid);
        if(idle == NULL)
            break;
        for(unsigned i = 0; i < 160u; ++i)
            idle[i] = static_cast<int16_t>(
                8000.0f * sinf(6.2831853f * cycles * i / 80.0f));
    }

    audioStream_stop(sid);
    audioPath_release(path);

    snprintf(out, outsz, "STREAM path=%d sid=%d syncs=%u/%u\r\n",
             (int)path, (int)sid, syncs, target);
}

/* ---- IMA-ADPCM (4-bit) decode -- integer-only, realtime on soft-float ----
 * Standard IMA tables; mirrors scripts/vpc_to_adpcm.py.  Decode state carries
 * across a clip; one nibble -> one s16 sample (low nibble first). */
namespace {

const int IMA_STEP[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,
    88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,
    544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,
    2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,
    10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
const int IMA_IDX[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};

struct ImaState { int pred; int index; };

inline int16_t imaDecodeNibble(ImaState &s, uint8_t code)
{
    int step = IMA_STEP[s.index];
    int diff = step >> 3;
    if(code & 4) diff += step;
    if(code & 2) diff += step >> 1;
    if(code & 1) diff += step >> 2;
    if(code & 8) s.pred -= diff; else s.pred += diff;
    if(s.pred >  32767) s.pred =  32767;
    if(s.pred < -32768) s.pred = -32768;
    s.index += IMA_IDX[code & 0xf];
    if(s.index < 0)  s.index = 0;
    if(s.index > 88) s.index = 88;
    return (int16_t)s.pred;
}

} // namespace

extern "C" const unsigned char hd2_adpcm_sample[];
extern "C" const unsigned hd2_adpcm_sample_count;

/* ADPCM test: decode the embedded "zero" clip into the circular stream buffer,
 * refilling the idle half on each sync (real producer pattern).  Proves
 * integer ADPCM decode keeps up + the outputStream path, end to end.  Reports
 * underruns (syncs that returned early). */
extern "C" void hd2_adpcm_sample_play(char *out, unsigned outsz)
{
    static int16_t buf[320];               // two 160-sample halves (20 ms each)
    const unsigned total = hd2_adpcm_sample_count;

    ImaState ima = {0, 0};
    unsigned nib = 0;                      // nibble index into the clip
    auto nextSample = [&]() -> int16_t {
        if(nib >= total) return 0;
        uint8_t byte = hd2_adpcm_sample[nib >> 1];
        uint8_t code = (nib & 1) ? (byte >> 4) : (byte & 0xf);
        ++nib;
        return imaDecodeNibble(ima, code);
    };

    for(unsigned i = 0; i < 320u; ++i) buf[i] = nextSample();

    pathId path = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_PROMPT);
    if(path < 0) { snprintf(out, outsz, "ADPCM path_err=%d\r\n", (int)path); return; }

    streamId sid = audioStream_start(path, buf, 320, 8000,
                                     STREAM_OUTPUT | BUF_CIRC_DOUBLE);
    if(sid < 0)
    {
        audioPath_release(path);
        snprintf(out, outsz, "ADPCM stream_err=%d\r\n", (int)sid);
        return;
    }

    unsigned halves = (total + 159u) / 160u;
    unsigned filled = 0, ok = 0;
    while(filled < halves)
    {
        if(outputStream_sync(sid, false) == false) break;
        ++ok;
        stream_sample_t *idle = outputStream_getIdleBuffer(sid);
        if(idle == NULL) break;
        for(unsigned i = 0; i < 160u; ++i) idle[i] = nextSample();
        ++filled;
    }

    audioStream_stop(sid);
    audioPath_release(path);
    snprintf(out, outsz, "ADPCM samples=%u halves=%u/%u sid=%d\r\n",
             total, ok, halves, (int)sid);
}

/* Software voice-prompt test: queue a sequence + trigger playback.  The codec2
 * decode + outputStream feed happen in the UI main_thread's vp_tick(), so this
 * just primes the queue and returns.  Forces vpLevel high so the gate opens.
 *   kind 0: announce integer <arg>
 *   kind 1: announce voice prompt id <arg> (PROMPT_* enum)
 *   kind 2: announce the string "OpenRTX" (spells via letters/dictionary) */
extern "C" void hd2_vp_say(uint8_t kind, uint8_t arg)
{
    state.settings.vpLevel = vpHigh;

    vp_flush();
    switch(kind)
    {
        case 0:  vp_queueInteger((int)arg); break;
        case 1:  vp_queuePrompt((uint16_t)arg); break;
        default: vp_queueString("OpenRTX", vpAnnounceCommonSymbols); break;
    }
    vp_play();
}
