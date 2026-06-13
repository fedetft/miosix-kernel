/*
 * hd2_diag.cpp -- minimal UART0 peek/poke diagnostic thread for the HD2
 * threaded (Miosix + OpenRTX) build.
 *
 * Speaks the EXACT wire protocol the flashing/debug bridge already implements
 * (scripts/rtx_tui.py  POST /cmd -> serial), so `rtx.py dbg r|w|ww` and the TUI
 * side panel work against the threaded firmware UNCHANGED:
 *
 *     'P'                              -> reply: "RTX1 ...version...\n"  (probe)
 *     'R' <u32 addr LE> <u8 size>      -> reply: <size> raw bytes   (byte read)
 *     'r' <u32 addr LE>                -> reply: <u32 LE>           (word read)
 *     'W' <u32 addr LE> <u32 val LE>   -> reply: 'k'                (word write)
 *
 * Unknown command bytes are ignored (resync), so the bridge's probe/keepalive
 * traffic does not wedge the parser. Intended for live MMIO/RAM inspection
 * during bring-up -- e.g. reading scheduler / os-timer state when a thread is
 * hung. Use word ops ('r'/'W') for MMIO registers; 'R' for RAM dumps.
 *
 * UART0 is the same 57600 8N1 console the bsp drives (hd2_dbg_puts); we only
 * add the RX side. Replies are short; concurrent console output during an
 * interactive poke is rare -- if it ever interleaves a reply, just re-issue.
 */

#include <miosix.h>
#include <pthread.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "hd2_router.h"   /* audio-routing matrix + named device-routing twiddlers */
#include "hd2_wdt.h"      /* watchdog: 'X' reboot / auto-WDT control */
#include "hd2_crumb.h"    /* hard-lock forensics: 'H' dump + idle-loop stamp */
#include "hd2_cps_settings.h" /* 'D' op: vendor settings/records accessors */
#include "hd2_cps_records.h"
#include "drivers/NVM/flash_w25q_HD2.h" /* 'd' op: raw W25Q sector write */
extern "C" int hd2_import_vendor_codeplug(int *out_ch); /* 'g' op (hd2_vendor_import.c) */

/* Speaker self-test tone.  platform_beepStart() is the HD2's minimal speaker
 * output pathway: it warms the codec (codec DAC -> lineout), unmutes DIPLEX0 +
 * the GPIOB amp/route, and drives PWM ch1 through the codec to the speaker --
 * with NO AT1846S (RF), NO modem, NO FM demod in the loop.  Exactly the path to
 * check in isolation. */
extern "C" void platform_beepStart(uint16_t freq);
extern "C" void platform_beepStop(void);

/* AT1846S register access (radio_HD2.cpp).  CAUTION: i2c0_lockDeviceBlocking
 * is a NO-OP on HD2 (the AT1846S bit-bang bus was assumed single-user), and the
 * GPIOA RMW is non-atomic -- so q/Q is ONLY safe with the FM worker OFF
 * (g_fm_active=0).  Poking concurrently with the FM worker's 250ms RSSI bit-bang
 * corrupts BOTH transactions, including RF-register *writes*.  (If concurrent
 * access is ever needed, make i2c0_lockDeviceBlocking a real FastMutex per the
 * note in i2c_csky.c.)  Exposed here to inspect/tune the AT1846S and chase the
 * FM demod->codec path without a reflash per experiment. */
extern "C" uint16_t hd2_at1846s_read(uint8_t reg);
extern "C" void     hd2_at1846s_write(uint8_t reg, uint16_t val);

/* Radio-path test helpers (radio_HD2.cpp / hd2_rtx.c).  g_rf_freeze is
 * the global that suspends ALL firmware-initiated AT1846S traffic and
 * audio-GPIO rewrites (rtx RSSI poll, squelch amp/route gating, FM worker,
 * beeps) so a host can run live chip experiments unopposed -- see the gated
 * call-site list at its definition in radio_HD2.cpp. */
extern "C" volatile uint32_t g_rf_freeze;

/* Radio boot-inhibit (hd2_rtx.c): 0 = radio bring-up deferred at boot; the
 * 'F' op sets it to 1 to bring the radio up on command (HW-I2C wedge debug). */
extern "C" volatile int g_radio_enabled;

/* Watchdog auto-heartbeat (hd2_rtx.c / hd2_wdt.h): rtx_task arms ~10 s and
 * feeds every pass while this is set; a hard lock then self-resets the chip.
 * Diag op 'X' controls it / forces an immediate WDT reboot. */
extern "C" volatile uint32_t g_wdt_auto;

/* IAP-handoff clock restore (platform.c) -- required before any deliberate
 * jump/reset into the IAP, or its UART comes up at the wrong baud. */
extern "C" void clk_restore_prepll(void);
extern "C" void     hd2_at1846s_reinit(uint32_t freq_hz);  /* BLOCKS ~700 ms (VCO cal) */
extern "C" void     hd2_at1846s_profile(uint32_t profile); /* 0=vendor 1=GD77 gains   */
extern "C" uint16_t hd2_at1846s_afmute(uint32_t mute);     /* reg0x30 bit7 RMW        */
extern "C" uint32_t hd2_rtx_getRxFreq(void);               /* current RX freq, Hz     */
extern "C" void     hd2_rtx_setFmExtras(uint8_t flags, uint8_t vox); /* 'e' op       */

/* CPU->codec-DAC PCM stream experiment (hd2_pcm_stream.cpp).  Streams a sine
 * through the SAHB PCM playback window via the vec-0x3b frame IRQ and reports
 * the IRQ count -- the live test for docs/pcm_stream_playback.md.  BLOCKS this
 * thread for the tone duration (<= 10 s). */
extern "C" void hd2_pcm_tone(uint16_t freq_hz, uint16_t ms, uint8_t arm,
                             char *out, unsigned outsz);

/* Full-stack stream test (hd2_pcm_stream.cpp): same tone but via
 * audioPath_request -> audioStream_start -> outputStream_HD2 driver.
 * BLOCKS this thread for the tone duration (<= 10 s). */
extern "C" void hd2_stream_tone(uint16_t freq_hz, uint16_t ms,
                                char *out, unsigned outsz);

/* RX audio-capture probe (hd2_pcm_stream.cpp): arm the codec ADC -> SAHB PCM
 * capture path and report peak-to-peak amplitude of the captured window.
 * BLOCKS this thread for <ms> (<= 10 s). */
extern "C" void hd2_pcm_capture(uint16_t ms, uint8_t arm, char *out, unsigned outsz);

/* APRS RX (hd2_pcm_stream.cpp): carrier-triggered, stream-decodes 1200-baud
 * AFSK/AX.25 from the demod audio.  BLOCKS up to ~10 s. */
extern "C" void hd2_aprs_rx(uint16_t frames, char *out, unsigned outsz);

/* APRS TX (hd2_pcm_stream.cpp): keys analog-FM TX and streams a 1200-baud
 * AFSK/AX.25 beacon via the codec playback path.  TRANSMITS RF.  BLOCKS ~<3 s. */
extern "C" void hd2_aprs_tx(uint8_t preamble, uint8_t flags, char *out, unsigned outsz);

/* Voice-prompt test (hd2_diag.cpp below): forces vpLevel high, queues a
 * sequence, and triggers playback.  The actual codec2 decode + streaming runs
 * in the UI main_thread's vp_tick(), so this returns immediately. */
extern "C" void hd2_vp_say(uint8_t kind, uint8_t arg);

/* IMA-ADPCM decode test (hd2_pcm_stream.cpp): decode the embedded "zero" clip
 * to PCM and stream it.  BLOCKS this thread for the clip duration (~540 ms). */
extern "C" void hd2_adpcm_sample_play(char *out, unsigned outsz);

using namespace miosix;

namespace {

// DesignWare 16550 UART0 @ 0x14030000 -- the SAME register file the bsp console
// uses (board/hd2 bsp.cpp). DLAB is 0 in run state (bsp set LCR=0x03 at init),
// so +0x00 reads RBR / writes THR.
#define HD2_UART0(off)   (*(volatile uint32_t*)(0x14030000u + (off)))
#define UART0_RBR        HD2_UART0(0x00u)   // RX buffer (read)
#define UART0_THR        HD2_UART0(0x00u)   // TX holding (write)
#define UART0_LSR        HD2_UART0(0x14u)   // line status
static constexpr uint32_t UART_LSR_DR   = 0x01u;   // RX data ready
static constexpr uint32_t UART_LSR_THRE = 0x20u;   // TX holding empty

// Version returned by the 'P' probe. Keeps the legacy "RTX1" token first (so the
// bridge recognises a healthy link) but appends a REAL build stamp compiled into
// this TU -- a fresh build changes it, so a stale flash is immediately visible
// over the probe (cf. the 2026-05-30 stale-build bug). Newline-terminated; the
// bridge reads a variable-length line.
#define HD2_DIAG_VERSION  "RTX1 hd2-miosix " __DATE__ " " __TIME__

// Bounded byte read: wait up to ~250 ms for RX-ready, then give up so a
// desynced/partial command aborts (parser resyncs) instead of wedging.
//
// 2026-06-11: the old version busy-spun 2,000,000 iterations with NO yield.
// On a partial/stalled command (the host bridge sends an opcode then a byte
// goes missing) that pegged the CPU for hundreds of ms PER missing byte --
// a 'W' awaiting 8 bytes could starve the rtx thread (which shares the
// AT1846S bus) for seconds, presenting as "serial stops answering / system
// frozen then self-recovers".  Now: poll briefly, then Thread::sleep(1) so
// other threads run while we wait, with a wall-clock-ish bound (~250 polls
// of 1 ms).  Returns fast on the common case (byte already in the FIFO).
bool rxByte(uint8_t &out)
{
    for(int ms = 0; ms < 250; ++ms)
    {
        for(int p = 0; p < 256; ++p)
            if((UART0_LSR & UART_LSR_DR) != 0u) { out = UART0_RBR & 0xFFu; return true; }
        Thread::sleep(1);
    }
    return false;
}

void tx(uint8_t c)
{
    for(uint32_t g = 0; g < 200000u && (UART0_LSR & UART_LSR_THRE) == 0u; ++g) {}
    UART0_THR = c;
}

void txStr(const char *s) { while(*s) tx(static_cast<uint8_t>(*s++)); }

bool rxU32(uint32_t &out)                          // little-endian
{
    out = 0;
    for(int i = 0; i < 4; ++i)
    {
        uint8_t b;
        if(!rxByte(b)) return false;               // timeout -> abort/resync
        out |= static_cast<uint32_t>(b) << (8 * i);
    }
    return true;
}

void txU32(uint32_t v)                             // little-endian
{
    for(int i = 0; i < 4; ++i) tx(static_cast<uint8_t>((v >> (8 * i)) & 0xFFu));
}

/*
 * FM TX tone test (op 'Y') -- the native version of the 2026-06-11 host-poke
 * experiments, per HR_C7000 manual ch.11 (FM application):
 *
 *   FM TX = CPU writes 256-sample PCM ping-pong buffers into the FM TX RAM
 *   (0x16000000 +0x030/+0x230, 8-bit RAM -> BYTE writes only, big-endian
 *   s16, low addr = high byte), WORK_MODE(0x11000100) bit7 = FM analog
 *   modulator mode, FM_PTT(0x11000560) bit0 = modem TX on.  The modem
 *   FM-modulates the PCM out MOD1/MOD2 into the AT1846S varactor nets;
 *   the AT1846S itself only keys the carrier (reg 0x30 = 0x4046).
 *
 * The tone is 1 kHz at 8 kHz sampling = 8 samples/cycle, so 256-sample
 * buffers hold exactly 32 cycles -- the ping-pong loops seamlessly with NO
 * FM_TX_INTERP servicing.  Host-poke history (why this is firmware-side):
 * the TX RAM ignores the upper 3 bytes of word stores (8-bit RAM) and an
 * unaligned word store to it hangs the SAHB; the AT1846S carrier keying and
 * WORK_MODE/FM_PTT writes were already proven live from the host.
 */
/* flags: bit0=WORK_MODE|0x80, bit1=FM_PTT=1, bit2=fill TX RAM with tone,
 *        bit3=write SIG_CENTER/RF_MOD_BIAS mid-scale guesses.
 * 2026-06-11 isolation matrix: bare keying (flags=0) was RECEIVED as carrier
 * in the host experiments; adding wm|0x80 + FM_PTT made the carrier vanish
 * from the receiver -- suspicion: MOD DACs engage with zeroed cal and drag
 * the carrier off-frequency.  This op lets us A/B each ingredient. */
static void fm_tx_tone_test(uint8_t secs, uint8_t flags, char *out, unsigned outsz)
{
    static const int16_t tone[8] = { 0, 8485, 12000, 8485, 0, -8485, -12000, -8485 };
    volatile uint8_t  *txram = reinterpret_cast<volatile uint8_t *>(0x16000000u);
    volatile uint32_t *wm    = reinterpret_cast<volatile uint32_t*>(0x11000100u);
    volatile uint32_t *ptt   = reinterpret_cast<volatile uint32_t*>(0x11000560u);
    volatile uint32_t *sigc  = reinterpret_cast<volatile uint32_t*>(0x11000108u);
    volatile uint32_t *mbias = reinterpret_cast<volatile uint32_t*>(0x11000114u);

    uint32_t freeze0 = g_rf_freeze;
    g_rf_freeze = 1;                       /* rtx thread off the bus while we TX */

    if(flags & 0x04u)
    {
        static const unsigned bufs[2] = { 0x030u, 0x230u };
        for(unsigned b = 0; b < 2; ++b)
            for(unsigned i = 0; i < 256u; ++i)
            {
                int16_t s = tone[i & 7u];
                txram[bufs[b] + 2u*i]      = static_cast<uint8_t>((s >> 8) & 0xff);
                txram[bufs[b] + 2u*i + 1u] = static_cast<uint8_t>(s & 0xff);
            }
    }

    volatile uint32_t *imask  = reinterpret_cast<volatile uint32_t*>(0x1100039cu);
    volatile uint32_t *istat  = reinterpret_cast<volatile uint32_t*>(0x11000398u);
    volatile uint32_t *iclear = reinterpret_cast<volatile uint32_t*>(0x110003b0u);
    volatile uint32_t *addrsw = reinterpret_cast<volatile uint32_t*>(0x1100056cu);

    uint32_t wm0 = *wm, sigc0 = *sigc, mbias0 = *mbias, imask0 = *imask;
    if(flags & 0x08u)
    {
        *sigc  = 0x80800000u;              /* MOD1/MOD2 offset mid-scale guess  */
        *mbias = 0x00004040u;              /* MOD amplitude mid-scale guess     */
    }
    if(flags & 0x20u) *imask = 0x0001007fu;/* vendor FM SYS_INTERP_MASK value   */
    if(flags & 0x01u) *wm  = wm0 | 0x80u;  /* FM analog modulator mode          */
    if(flags & 0x02u) *ptt = 1u;           /* modem FM TX on                    */

    uint16_t r40 = hd2_at1846s_read(0x40);
    hd2_at1846s_write(0x40, 0x0030u);      /* vendor TX-side AF-DSP ctrl value  */
    hd2_at1846s_write(0x30, 0x4006u);
    hd2_at1846s_write(0x30, 0x4046u);      /* tx_on: key the carrier            */

    /* Engine liveness probe (flags bit4): poll the SYS_INTERP status latch
     * while keyed; ack everything seen via SYS_INTERP_CLEAR so a level-
     * triggered FM_TX_INTERP can't stall the buffer ping-pong.  irqs counts
     * status-nonzero observations; swlast tracks FM_ADDR_SW movement. */
    uint32_t irqs = 0, swseen = 0, swlast = *addrsw & 3u;
    if(flags & 0x10u)
    {
        for(unsigned t = 0; t < secs * 200u; ++t)      /* 5 ms cadence */
        {
            uint32_t st = *istat;
            if(st != 0u) { irqs++; *iclear = st; }
            uint32_t sw = *addrsw & 3u;
            if(sw != swlast) { swseen++; swlast = sw; }
            Thread::sleep(5);
        }
    }
    else
    {
        for(unsigned t = 0; t < secs; ++t) Thread::sleep(1000);
    }

    hd2_at1846s_write(0x30, 0x4006u);      /* dekey                             */
    *ptt = 0u;
    *wm  = wm0;
    *imask = imask0;
    if(flags & 0x08u) { *sigc = sigc0; *mbias = mbias0; }
    hd2_at1846s_write(0x40, (r40 != 0u) ? r40 : 0x0031u);
    hd2_at1846s_write(0x30, 0x4826u);      /* back to RX-on                     */
    g_rf_freeze = freeze0;

    snprintf(out, outsz,
             "FMTXTONE secs=%u flags=%02x wm=%08lx mb=%08lx irqs=%lu sw=%lu\r\n",
             secs, flags, (unsigned long)wm0, (unsigned long)mbias0,
             (unsigned long)irqs, (unsigned long)swseen);
}

/*
 * DTMF transmit (diag 'T'): key a bare FM carrier and send an ASCII digit
 * string.  The AT1846S generates the tone pairs internally (reg 0x35/0x36) and
 * sums them into the FM modulation -- no C7000 voice path needed.  Fixed
 * on/off timing per digit.  TRANSMITS RF.  Mirrors fm_tx_tone_test's key/dekey.
 */
static void dtmf_tx_send(const char *s, unsigned n, uint16_t onMs, uint16_t offMs,
                         char *out, unsigned outsz)
{
    static const uint16_t rows[4] = { 697, 770, 852, 941 };
    static const uint16_t cols[4] = { 1209, 1336, 1477, 1633 };

    uint32_t freeze0 = g_rf_freeze; g_rf_freeze = 1;
    uint16_t r40 = hd2_at1846s_read(0x40);

    hd2_at1846s_write(0x40, 0x0030u);
    hd2_at1846s_write(0x30, 0x4006u);
    hd2_at1846s_write(0x30, 0x4046u);                                       // tx_on
    hd2_at1846s_write(0x3A, (hd2_at1846s_read(0x3A) & ~0x7000u) | 0x3000u); // tone1+tone2
    hd2_at1846s_write(0x57, hd2_at1846s_read(0x57) | 0x0001u);              // AFOUT=DTMF
    hd2_at1846s_write(0x79, hd2_at1846s_read(0x79) & ~0xC000u);             // dtmf_direct/tx=0

    Thread::sleep(60);   // let the carrier + far-end squelch/AGC settle before the first tone

    unsigned sent = 0;
    for(unsigned i = 0; i < n; ++i)
    {
        int r, c;
        switch(s[i])
        {
            case '1':r=0;c=0;break; case '2':r=0;c=1;break; case '3':r=0;c=2;break; case 'A':case 'a':r=0;c=3;break;
            case '4':r=1;c=0;break; case '5':r=1;c=1;break; case '6':r=1;c=2;break; case 'B':case 'b':r=1;c=3;break;
            case '7':r=2;c=0;break; case '8':r=2;c=1;break; case '9':r=2;c=2;break; case 'C':case 'c':r=2;c=3;break;
            case '*':r=3;c=0;break; case '0':r=3;c=1;break; case '#':r=3;c=2;break; case 'D':case 'd':r=3;c=3;break;
            default: continue;
        }
        hd2_at1846s_write(0x35, (uint16_t)(rows[r] * 10u));
        hd2_at1846s_write(0x36, (uint16_t)(cols[c] * 10u));
        hd2_at1846s_write(0x7A, hd2_at1846s_read(0x7A) | 0x8000u);          // dtmf_en=1
        Thread::sleep(onMs);
        hd2_at1846s_write(0x7A, hd2_at1846s_read(0x7A) & ~0x8000u);         // dtmf_en=0
        Thread::sleep(offMs);
        sent++;
    }

    hd2_at1846s_write(0x57, hd2_at1846s_read(0x57) & ~0x0001u);
    hd2_at1846s_write(0x3A, (hd2_at1846s_read(0x3A) & ~0x7000u) | 0x4000u); // mic
    hd2_at1846s_write(0x30, 0x4006u);                                       // dekey
    hd2_at1846s_write(0x40, (r40 != 0u) ? r40 : 0x0031u);
    hd2_at1846s_write(0x30, 0x4826u);                                       // RX-on
    g_rf_freeze = freeze0;
    snprintf(out, outsz, "DTMF tx sent=%u\r\n", sent);
}

/*
 * DTMF receive (diag 't'): enable the AT1846S decoder and drain up to 'max'
 * digits seen within a ~3 s window.  NOTE: the 0x67..0x76 Goertzel coefficients
 * are left at the chip default (12.8/25.6 MHz reference); the HD2 is 26 MHz, so
 * this op is the HW check of whether decode works at the default coefficients.
 */
static void dtmf_rx_read(uint8_t max, char *out, unsigned outsz)
{
    static const char tbl[16] = { '0','1','2','3','4','5','6','7',
                                  '8','9','A','B','C','D','*','#' };
    uint32_t freeze0 = g_rf_freeze; g_rf_freeze = 1;
    hd2_at1846s_write(0x7A, 0x8018u);                  // dtmf_en + detect time

    char digits[33]; unsigned k = 0;
    if(max > 32u) max = 32u;
    for(unsigned t = 0; t < 300u && k < max; ++t)      // ~3 s @10 ms
    {
        uint16_t st = hd2_at1846s_read(0x7E);
        if((st & 0x0010u) != 0u)                        // dtmf_sample ready
        {
            digits[k++] = tbl[st & 0x0F];
            for(unsigned w = 0; w < 30u; ++w)           // wait for ready to clear
            {
                if((hd2_at1846s_read(0x7E) & 0x0010u) == 0u) break;
                Thread::sleep(5);
            }
        }
        Thread::sleep(10);
    }

    hd2_at1846s_write(0x7A, hd2_at1846s_read(0x7A) & ~0x8000u);  // disable
    g_rf_freeze = freeze0;
    digits[k] = 0;
    snprintf(out, outsz, "DTMF rx=%s (%u)\r\n", digits, k);
}

void *diagThreadFunc(void *)
{
    for(;;)
    {
        // Idle: sleep rather than busy-burn until a byte arrives. A 5 ms sleeper
        // survives a PARTIAL hang the same way main_thread does, so the poke
        // interface still answers when the UI thread is wedged. (The getTime()
        // os_timer race that once collapsed sleep() is fixed in 812d5fa3, so a
        // sleeper no longer hangs with the timer -- a yield() spin here would just
        // peg the core and defeat tickless idle.)
        if((UART0_LSR & UART_LSR_DR) == 0u)
        {
            HD2_CRUMB_STAMP(diag);      // hard-lock forensics: diag idle loop alive
            Thread::sleep(5);
            continue;
        }

        uint8_t cmd;
        if(!rxByte(cmd)) continue;
        switch(cmd)
        {
            case 'P':                              // probe -> version line
                txStr(HD2_DIAG_VERSION);
                tx('\n');
                break;
            case 'R':                              // byte read: <addr><size>
            {
                uint32_t addr; uint8_t size;
                if(!rxU32(addr) || !rxByte(size)) break;   // timeout -> resync
                volatile const uint8_t *p =
                    reinterpret_cast<volatile const uint8_t *>(addr);
                for(uint8_t i = 0; i < size; ++i) tx(p[i]);
                break;
            }
            case 'r':                              // word read: <addr>
            {
                uint32_t addr;
                if(!rxU32(addr)) break;
                txU32(*reinterpret_cast<volatile const uint32_t *>(addr));
                break;
            }
            case 'W':                              // word write: <addr><val>
            {
                uint32_t addr, val;
                if(!rxU32(addr) || !rxU32(val)) break;
                *reinterpret_cast<volatile uint32_t *>(addr) = val;
                tx('k');                           // ack
                break;
            }
            case 'N':                              // connect path: <src u8><sink u8> -> u8
            {
                uint8_t src, snk;
                if(!rxByte(src) || !rxByte(snk)) break;
                tx(hd2_router_connect(src, snk) == 0 ? 0x00u : 0xFFu);
                break;
            }
            case 'n':                              // disconnect path: <src u8><sink u8> -> u8
            {
                uint8_t src, snk;
                if(!rxByte(src) || !rxByte(snk)) break;
                tx(hd2_router_disconnect(src, snk) == 0 ? 0x00u : 0xFFu);
                break;
            }
            case 'e':                              // FM extras: <flags u8><vox u8> -> 'k'
            {                                      // flags bit0=1750 burst, bit1=tail elim; vox 0..5
                uint8_t flags, vox;
                if(!rxByte(flags) || !rxByte(vox)) break;
                hd2_rtx_setFmExtras(flags, vox);
                tx('k');
                break;
            }
            case 'T':                              // DTMF tx: <n u8><onMs u16LE><offMs u16LE><n ASCII digits>
            {                                      // TRANSMITS RF.  on/off 0 -> defaults (120/80 ms).
                uint8_t n, ol, oh, fl, fh;
                if(!rxByte(n) || !rxByte(ol) || !rxByte(oh) || !rxByte(fl) || !rxByte(fh)) break;
                if(n > 32u) n = 32u;
                uint16_t onMs  = (uint16_t)(ol | (oh << 8));
                uint16_t offMs = (uint16_t)(fl | (fh << 8));
                if(onMs  == 0u) onMs  = 120u;
                if(offMs == 0u) offMs = 80u;
                char s[32];
                bool ok = true;
                for(uint8_t i = 0; i < n; ++i) { uint8_t b; if(!rxByte(b)) { ok = false; break; } s[i] = (char)b; }
                if(!ok) break;
                char buf[48];
                dtmf_tx_send(s, n, onMs, offMs, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 't':                              // DTMF rx: <max u8> -> "DTMF rx=<digits> (N)"
            {
                uint8_t mx;
                if(!rxByte(mx)) break;
                char buf[48];
                dtmf_rx_read(mx, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'S':                              // set routing target: <target u8><val u32 LE> -> u32 LE
            {
                uint8_t target; uint32_t val;
                if(!rxByte(target) || !rxU32(val)) break;
                txU32(hd2_route_set(target, val));
                break;
            }
            case 'E':                              // arm guarded routing ops (one-shot)
                hd2_route_arm_tx();
                txStr("TX armed\r\n");              // \r\n to match main.c's reply
                break;
            case 'U':                              // routing snapshot -> ascii line(s)
            {
                char buf[320];
                hd2_route_dump(buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'b':                              // speaker tone test: <freq u16 LE> -> 'k'
            {                                      // PWM ch1 -> codec DAC -> lineout -> amp -> speaker;
                uint8_t lo, hi;                    // bypasses AT1846S / modem / FM entirely.
                if(!rxByte(lo) || !rxByte(hi)) break;
                uint16_t f = (uint16_t)((uint16_t)lo | (uint16_t)(hi << 8));
                if(f == 0u) f = 1000u;             // 0 -> 1 kHz default
                platform_beepStart(f);
                Thread::sleep(300);                // ~300 ms tone (timer is healthy post-812d5fa3)
                platform_beepStop();
                tx('k');
                break;
            }
            case 'q':                              // read AT1846S register: <reg u8> -> u16 LE
            {
                uint8_t reg;
                if(!rxByte(reg)) break;
                uint16_t v = hd2_at1846s_read(reg);
                tx((uint8_t)(v & 0xffu));
                tx((uint8_t)((v >> 8) & 0xffu));
                break;
            }
            case 'Q':                              // write AT1846S register: <reg u8> <val u16 LE> -> 'k'
            {
                uint8_t reg, lo, hi;
                if(!rxByte(reg) || !rxByte(lo) || !rxByte(hi)) break;
                hd2_at1846s_write(reg, (uint16_t)((uint16_t)lo | (uint16_t)(hi << 8)));
                tx('k');
                break;
            }
            case 'z':                              // rf_freeze: <on u8> -> "RFFREEZE=N\r\n"
            {                                      // suspend/resume ALL firmware AT1846S I2C +
                uint8_t on;                        // audio-GPIO rewrites (live-experiment guard)
                if(!rxByte(on)) break;
                g_rf_freeze = (on != 0u) ? 1u : 0u;
                txStr(g_rf_freeze ? "RFFREEZE=1\r\n" : "RFFREEZE=0\r\n");
                break;
            }
            case 'g':                              // import factory channels -> OpenRTX codeplug
            {                                      // -> "IMPORT=<n>\r\n"
                int n = hd2_import_vendor_codeplug(nullptr);
                char buf[24];
                snprintf(buf, sizeof buf, "IMPORT=%d\r\n", n);
                txStr(buf);
                break;
            }
            case 'd':                              // flash-write one sector: <addr u32 LE>
            {                                      // <len u16 LE> <data[len]> -> "FWR=ok/err"
                static uint8_t fwbuf[W25Q_HD2_SECTOR_SIZE];
                uint8_t h[6];
                bool ok = true;
                for(int i = 0; i < 6 && ok; ++i) ok = rxByte(h[i]);
                if(!ok) { txStr("FWR=short\r\n"); break; }
                uint32_t addr = (uint32_t)h[0] | ((uint32_t)h[1] << 8) |
                                ((uint32_t)h[2] << 16) | ((uint32_t)h[3] << 24);
                uint32_t len  = (uint32_t)h[4] | ((uint32_t)h[5] << 8);
                if(len > sizeof(fwbuf)) { txStr("FWR=toolong\r\n"); break; }
                for(uint32_t i = 0; i < len && ok; ++i) ok = rxByte(fwbuf[i]);
                if(!ok) { txStr("FWR=short\r\n"); break; }
                if(!w25q_hd2_probe()) { txStr("FWR=noflash\r\n"); break; }
                int e = w25q_hd2_eraseSector(addr);
                int p = (e == 0) ? w25q_hd2_program(addr, fwbuf, len) : -1;
                txStr((e == 0 && p == 0) ? "FWR=ok\r\n" : "FWR=err\r\n");
                break;
            }
            case 'F':                              // radio_enable: bring the radio up on command
            {                                      // (boot-inhibit debug). -> "RADIO=1\r\n"
                g_radio_enabled = 1;
                txStr("RADIO=1\r\n");
                break;
            }
            case 'D':                              // dump vendor data layer (settings +
            {                                      // channel[0] + contact[0]) -> ascii lines
                char buf[200];
                hd2_cps_settings_t st;
                int sres = hd2_cps_settings_load(&st);
                snprintf(buf, sizeof buf,
                    "VSET(%s) sql=%u bl=%u bri=%u mic=%d keybeep=%u step=%u\r\n",
                    sres == 1 ? "dflt" : (sres == 0 ? "ok" : "err"),
                    st.squelch, st.backlight, st.brightness, (int)st.mic_gain,
                    (st.flags4 & HD2_VSET_F4_KEY_BEEP) ? 1u : 0u, st.step);
                txStr(buf);

                hd2_vendor_channel_t ch;
                if(hd2_vendor_channel_read(0, &ch) == 0 && hd2_vendor_channel_present(&ch))
                {
                    char nm[11]; memcpy(nm, ch.name, 10); nm[10] = 0;
                    snprintf(buf, sizeof buf,
                        "CH0 \"%s\" rx=%lu tx=%lu %s %s\r\n", nm,
                        (unsigned long)hd2_bcd4_to_hz(ch.rx_freq),
                        (unsigned long)hd2_bcd4_to_hz(ch.tx_freq),
                        hd2_channel_is_dmr(&ch) ? "DMR" : "FM",
                        hd2_channel_is_wide(&ch) ? "wide" : "narrow");
                }
                else snprintf(buf, sizeof buf, "CH0 empty\r\n");
                txStr(buf);

                hd2_vendor_contact_t ct;
                if(hd2_vendor_contact_read(0, &ct) == 0 && hd2_vendor_contact_present(&ct))
                {
                    char nm[17]; memcpy(nm, ct.name, 16); nm[16] = 0;
                    snprintf(buf, sizeof buf, "CT0 id=%lu type=%u \"%s\"\r\n",
                        (unsigned long)ct.dmr_id, ct.type, nm);
                }
                else snprintf(buf, sizeof buf, "CT0 empty\r\n");
                txStr(buf);
                break;
            }
            case 'H':                              // crumbs: previous-life + live breadcrumbs
            {                                      // (hard-lock forensics; hd2_crumb.h)
                char buf[360];
                snprintf(buf, sizeof buf,
                    "PREV boots=%lu rtx=%lu@%08lx wake=%lu@%08lx ovf=%lu@%08lx diag=%lu@%08lx"
                    " wctl=%lx wld=%08lx wcur=%08lx irq=%08lx%08lx"
                    " disp=%lu@%08lx arm=%lu/%08lx pic=%08lx res=%08lx/%08lx/%08lx%s\r\n"
                    "LIVE boots=%lu rtx=%lu@%08lx wake=%lu@%08lx ovf=%lu@%08lx diag=%lu@%08lx now=%08lx\r\n",
                    (unsigned long)hd2_crumb_prev.boot_count,
                    (unsigned long)hd2_crumb_prev.rtx_hb,  (unsigned long)hd2_crumb_prev.rtx_tick,
                    (unsigned long)hd2_crumb_prev.wake_hb, (unsigned long)hd2_crumb_prev.wake_tick,
                    (unsigned long)hd2_crumb_prev.ovf_hb,  (unsigned long)hd2_crumb_prev.ovf_tick,
                    (unsigned long)hd2_crumb_prev.diag_hb, (unsigned long)hd2_crumb_prev.diag_tick,
                    (unsigned long)hd2_crumb_prev.wake_ctrl, (unsigned long)hd2_crumb_prev.wake_load,
                    (unsigned long)hd2_crumb_prev.wake_cur,
                    (unsigned long)hd2_crumb_prev.irqns_hi, (unsigned long)hd2_crumb_prev.irqns_lo,
                    (unsigned long)hd2_crumb_prev.disp_hb, (unsigned long)hd2_crumb_prev.disp_tick,
                    (unsigned long)hd2_crumb_prev.arm_hb,  (unsigned long)hd2_crumb_prev.arm_rel,
                    (unsigned long)hd2_crumb_prev.pic_snap,
                    (unsigned long)hd2_crumb_prev.res_sp,
                    (unsigned long)hd2_crumb_prev.res_epc,
                    (unsigned long)hd2_crumb_prev.res_epsr,
                    (hd2_crumb_prev.magic == HD2_CRUMB_MAGIC) ? "" : " (INVALID)",
                    (unsigned long)HD2_CRUMB->boot_count,
                    (unsigned long)HD2_CRUMB->rtx_hb,  (unsigned long)HD2_CRUMB->rtx_tick,
                    (unsigned long)HD2_CRUMB->wake_hb, (unsigned long)HD2_CRUMB->wake_tick,
                    (unsigned long)HD2_CRUMB->ovf_hb,  (unsigned long)HD2_CRUMB->ovf_tick,
                    (unsigned long)HD2_CRUMB->diag_hb, (unsigned long)HD2_CRUMB->diag_tick,
                    (unsigned long)HD2_CRUMB_NOW());
                txStr(buf);
                break;
            }
            case 'X':                              // wdt: <mode u8> 0=force reboot now,
            {                                      // 1=auto-WDT on, 2=auto-WDT off
                uint8_t m;
                if(!rxByte(m)) break;
                if(m == 0u)
                {
                    txStr("REBOOT\r\n");
                    Thread::sleep(30);             // let the reply drain the UART
                    /* Restore the IAP-handoff (~84 MHz) clocks BEFORE the reset:
                     * the WDT reset does NOT reset the clock tree, and the IAP
                     * computes its UART divisor assuming the pre-PLL clock --
                     * without this it comes up at 28800 and spews junk (and any
                     * host bytes it mis-frames become menu keystrokes, which
                     * can wedge it into YMODEM-wait).  Same recipe as the 'Z'
                     * jump-to-IAP op (main.c). */
                    clk_restore_prepll();
                    hd2_wdt_force_reset();         // ~15 ms WDT -> chip reset; no return
                }
                else if(m == 1u) { g_wdt_auto = 1u; txStr("WDT=1\r\n"); }
                else             { g_wdt_auto = 0u; hd2_wdt_off(); txStr("WDT=0\r\n"); }
                break;
            }
            case 'c':                              // echo: <val u8> -> "ECHO=NN\r\n"
            {                                      // sequence counter so a reply after a lock can
                uint8_t v;                         // be tied to the exact probe that produced it
                if(!rxByte(v)) break;
                char buf[16];
                snprintf(buf, sizeof buf, "ECHO=%02x\r\n", (unsigned)v);
                txStr(buf);
                break;
            }
            case 'I':                              // at_reinit -> "REINIT freq=<hz>\r\n"
            {                                      // one-shot full AT1846S bring-up; BLOCKS this
                                                   // thread ~700 ms (VCO calibration delays).
                uint32_t f = hd2_rtx_getRxFreq();
                hd2_at1846s_reinit(f);
                char buf[48];
                snprintf(buf, sizeof buf, "REINIT freq=%lu\r\n", (unsigned long)f);
                txStr(buf);
                break;
            }
            case 'o':                              // at_profile: <profile u8> -> 'k'
            {                                      // 0=vendor / 1=GD77 audio-gain regs, live A/B
                uint8_t prof;
                if(!rxByte(prof)) break;
                hd2_at1846s_profile(prof);
                tx('k');
                break;
            }
            case 'm':                              // at_mute: <mute u8> -> u16 LE (resulting reg 0x30)
            {                                      // RMW AT1846S reg 0x30 bit7 (RX AF mute)
                uint8_t mute;
                if(!rxByte(mute)) break;
                uint16_t v = hd2_at1846s_afmute(mute);
                tx((uint8_t)(v & 0xffu));
                tx((uint8_t)((v >> 8) & 0xffu));
                break;
            }
            case 'u':                              // audio_snap -> ascii block (multi-line)
            {                                      // full audio-path state: AT1846S + GPIO +
                char buf[512];                     // diplex + CLK_MGR + codec (hd2_router.c)
                hd2_audio_snap(buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'O':                              // pcm_capture probe: <ms u16 LE><arm u8> -> "CAP pp=.."
            {
                uint8_t mlo, mhi, arm;
                if(!rxByte(mlo) || !rxByte(mhi) || !rxByte(arm)) break;
                char buf[96];
                hd2_pcm_capture((uint16_t)(mlo | (mhi << 8)), arm, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'w':                              // aprs_rx: <frames u8> -> "APRS: <frame>" or status
            {
                uint8_t fr;
                if(!rxByte(fr)) break;
                static char buf[288];              // static: keep the diag stack small
                hd2_aprs_rx((uint16_t)fr, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'B':                              // aprs_tx: <preamble u8><flags u8> -> "APRSTX .."
            {                                      // keys analog-FM TX, beacons 1200-AFSK via
                uint8_t pre, fl;                   // codec playback.  TRANSMITS RF for <~3 s.
                if(!rxByte(pre) || !rxByte(fl)) break;
                char buf[96];
                hd2_aprs_tx(pre, fl, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'p':                              // pcm_tone: <freq u16 LE> <ms u16 LE> <arm u8> -> ascii line
            {                                      // SAHB PCM-window sine stream via vec-0x3b IRQ;
                uint8_t flo, fhi, mlo, mhi, arm;   // reply reports IRQ count + path regs.  Blocks <= 10 s.
                if(!rxByte(flo) || !rxByte(fhi) || !rxByte(mlo) || !rxByte(mhi)
                   || !rxByte(arm)) break;
                char buf[96];
                hd2_pcm_tone((uint16_t)(flo | (fhi << 8)),
                             (uint16_t)(mlo | (mhi << 8)), arm, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'v':                              // stream_tone: <freq u16 LE> <ms u16 LE> -> ascii line
            {                                      // full audioPath/audioStream/outputStream_HD2
                uint8_t flo, fhi, mlo, mhi;        // stack test.  Blocks <= 10 s.
                if(!rxByte(flo) || !rxByte(fhi) || !rxByte(mlo) || !rxByte(mhi)) break;
                char buf[96];
                hd2_stream_tone((uint16_t)(flo | (fhi << 8)),
                                (uint16_t)(mlo | (mhi << 8)), buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'A':                              // adpcm_test: no args -> ascii line
            {                                      // decode embedded "zero" ADPCM clip -> stream
                char buf[80];
                hd2_adpcm_sample_play(buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'V':                              // vp_say: <kind u8> <arg u8> -> 'k'
            {                                      // software voice-prompt test (codec2 -> stream).
                uint8_t kind, arg;                 // kind 0=integer(arg), 1=prompt(arg), 2="OpenRTX"
                if(!rxByte(kind) || !rxByte(arg)) break;
                hd2_vp_say(kind, arg);
                tx('k');
                break;
            }
            case 'M':                              // byte write (st.b): <addr u32><val u8> -> 'k'
            {                                      // for 8-bit blocks: codec @0x160009xx and the
                uint32_t addr; uint8_t val;        // modem TX/RX RAM @0x1600xxxx (word stores only
                if(!rxU32(addr) || !rxByte(val)) break;   // land their low byte there)
                *reinterpret_cast<volatile uint8_t *>(addr) = val;
                tx('k');
                break;
            }
            case 'Y':                              // fm_tx_tone: <secs u8><flags u8> -> ascii line
            {                                      // native FM-TX tone test (see fm_tx_tone_test
                uint8_t secs, flags;               // above).  TRANSMITS RF for <secs> seconds!
                if(!rxByte(secs) || !rxByte(flags)) break;
                if(secs == 0u || secs > 20u) secs = 6u;
                char buf[96];
                fm_tx_tone_test(secs, flags, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'x':                              // vp_fire: <msg_id u8> <mode u8> -> 'k'
            {                                      // voice-prompt doorbell experiment
                uint8_t id, mode;                  // (hd2_router.c, docs/voice_prompt_map.md)
                if(!rxByte(id) || !rxByte(mode)) break;
                hd2_vp_fire(id, mode);
                tx('k');
                break;
            }
            default:
                break;                             // ignore -> resync next cmd
        }
    }
    return nullptr;                                // unreachable
}

} // namespace

// Start the diag thread. Call AFTER platform/UART init (openrtx_init), from the
// HD2 entry (main.cpp). HD2-only -- never linked into OpenRTX core.
extern "C" void hd2_diag_start()
{
    static pthread_t diag_thread;
    pthread_create(&diag_thread, nullptr, diagThreadFunc, nullptr);
}
