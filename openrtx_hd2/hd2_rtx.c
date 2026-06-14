/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * rtx implementation for the HD2 threaded bring-up: FM RX + PTT-keyed FM
 * voice TX (TX added 2026-06-11 -- see docs/audio_paths.md §5).
 *
 * This replaces the rtx_* stubs (formerly in hd2_stubs.c, which just
 * returned a fixed RSSI of -127 -> a static S-meter bar).  It mirrors the
 * RX-relevant half of OpenRTX's openrtx/src/rtx/rtx.cpp plus the idle-RX
 * ("enterRx") path of OpMode_FM.cpp, but WITHOUT the pieces that aren't
 * ported yet on the HD2:
 *   - the OpMode handler objects (drag in OpMode_M17 + codec2),
 *   - the full audioPath_request/release manager (we call the audio_HD2
 *     routing matrix directly -- see the squelch-gated audio below),
 *   - TX (radio_HD2.cpp is an RX-only control path; TX is hard-disabled).
 *
 * What it does: tune the AT1846S to the channel's RX frequency, sit in RX
 * while idle (so the radio is actually *listening*), low-pass-filter the
 * live RSSI from radio_getRssi() (AT1846S reg 0x1B), and surface it via
 * rtx_getRssi() -> state.rssi -> the UI S-meter.
 *
 * Re-tuning: the UI thread (core/threads.c) builds rtx_cfg from
 * state.channel and calls rtx_configure() on every `sync_rtx`, so changing
 * the VFO frequency re-tunes the chip here on the next rtx_task() pass.
 *
 * When the audio path + the real OpMode_FM land, delete this file and link
 * openrtx/src/rtx/{rtx,OpMode_FM}.cpp instead (see hd2_app_stubs.c notes).
 */

#include "interfaces/radio.h"
#include "interfaces/audio.h"
#include "interfaces/delays.h"
#include "interfaces/platform.h"
#include "rtx/rtx.h"
#include "hd2_wdt.h"
#include "hd2_crumb.h"
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* FM-broadcast-active flag (hd2_fm_broadcast.cpp, set by the UI FM screen):
 * while set, the dedicated 0x20 tuner owns the speaker; rtx_task stands down
 * from the audio path (see the gate in rtx_task). */
extern volatile uint32_t g_fm_active;

/* RF-freeze flag (radio_HD2.cpp, loader op 'z'): while set, rtx_task
 * skips ALL chip I/O -- the 33 Hz AT1846S RSSI poll, the squelch-driven
 * audio GPIO gate and any pending reconfigure's radio_* writes -- so a host
 * can poke the AT1846S live without being overwritten.  The thread itself
 * keeps running (it just sleeps), and a queued rtx_configure() is picked up
 * on un-freeze. */
extern volatile uint32_t g_rf_freeze;

/* AT1846S chip access + VOX (radio_HD2.cpp, proven bit-bang path). */
extern void     hd2_at1846s_write(uint8_t reg, uint16_t val);
extern uint16_t hd2_at1846s_read(uint8_t reg);
extern void     hd2_vox_enable(uint8_t thHigh, uint8_t thLow);
extern void     hd2_vox_disable(void);
extern bool     hd2_vox_detected(void);
extern bool     hd2_rx_carrier_detected(void);   /* AT1846S sq_cmp (reg 0x1C[0]) */

static void rtx_updateSquelch(void);             /* sq_cmp -> cached s_sqlOpen */

/* TX-extras timing, in 30 ms rtx_task passes. */
#define RTX_TONE_BURST_PASSES 25u   /* 1750 Hz key-up burst   ~0.75 s */
#define RTX_TAIL_ELIM_PASSES   6u   /* reverse-burst on dekey ~180 ms */
#define RTX_VOX_HANG_PASSES   30u   /* VOX hangtime           ~0.90 s */

/* Map VOX level 1..5 to an AT1846S reg-0x64 threshold field (VOX sheet);
 * higher level = more sensitive = lower threshold.  Returns false if off. */
static bool rtx_voxThresh(uint8_t level, uint8_t *th)
{
    static const uint8_t code[5] = { 0x45, 0x48, 0x4C, 0x52, 0x58 };
    if(level == 0u || level > 5u) return false;
    *th = code[level - 1];
    return true;
}

/* 1750 Hz tone-burst on/off via the AT1846S tone1 generator (reg 0x35 freq,
 * 0x3A[14:12] source, 0x79[15:14] output). */
static void rtx_toneBurstStart(void)
{
    hd2_at1846s_write(0x35, 17500u);                                    /* 1750.0 Hz */
    hd2_at1846s_write(0x3A, (hd2_at1846s_read(0x3A) & ~0x7000u) | 0x1000u);
    hd2_at1846s_write(0x79, (hd2_at1846s_read(0x79) & ~0xF000u) | 0xC000u);
}
static void rtx_toneBurstStop(void)
{
    hd2_at1846s_write(0x3A, (hd2_at1846s_read(0x3A) & ~0x7000u) | 0x4000u);  /* mic */
    hd2_at1846s_write(0x79,  hd2_at1846s_read(0x79) & ~0xF000u);             /* off */
}

static pthread_mutex_t   *cfgMutex;     /* config-handoff mutex (from rtx_init) */
static const rtxStatus_t *newCnf;       /* pending config from rtx_configure()  */
static rtxStatus_t        rtxStatus;    /* current rtx driver status            */
static rssi_t             rssi;         /* filtered RSSI, dBm                    */
static bool               reinitFilter; /* seed the RSSI filter next RX pass     */
static bool               enterRx;      /* request (re-)entry into RX            */
static bool               audioOpen;    /* RX->speaker audio path currently routed */
/* Radio boot-inhibit.  0 = radio bring-up deferred (system boots with the
 * radio bus untouched) until the diag 'F' op sets it to 1.  Default 1 = the
 * radio comes up at boot (normal operation).  The 0 default was a diagnostic
 * for the HW-I2C wedge hunt -- root-caused 2026-06-12 (platform_init's
 * IO_DIPLEX0 write stole the pad mux from the I2C1 controller; see
 * platform.c + docs/TASK_hw_i2c_migration.md).  Flip to 0 to get the
 * bring-up-on-command harness back.  See rtx_init / rtx_task /
 * rtx_bringupRadio. */
volatile int              g_radio_enabled = 1;
static bool               radioBroughtUp  = false;

/* Watchdog heartbeat moved to platform/targets/HD2/watchdog_HD2.c (the portable
 * watchdog_kick() hook, fed from rtx_threadFunc) so the feed survives the
 * OpMode_FM convergence (rtx.cpp).  g_wdt_auto + the arm/feed live there now. */

static bool               cfgApplied;   /* first real config from the UI arrived.
                                         * Until then the audio gate stays CLOSED:
                                         * rtx_init's placeholder sqlLevel=1 maps to
                                         * ~-122 dBm, which the ~-94 dBm noise floor
                                         * opens instantly -> boot "kchunk" static
                                         * burst before the codeplug squelch lands. */

/*
 * Open/close the analog RX->speaker audio path, idempotently.  audio_HD2.c's
 * audio_connect(SOURCE_RTX, SINK_SPK) releases the AT1846S chip-side RX mute
 * (reg 0x30 bit7) and pulls the board audio-route (PTB10) + speaker-amp (PTB4)
 * lines low -- the same pure-analog path the broadcast-FM tuner uses.  We drive
 * it directly rather than dragging in the full audioPath manager.
 */
static void rtx_setAudio(bool open)
{
    if(open == audioOpen)
        return;

    if(open)
        audio_connect(SOURCE_RTX, SINK_SPK);
    else
        audio_disconnect(SOURCE_RTX, SINK_SPK);

    audioOpen = open;
}

/* The deferred radio bring-up (AT1846S init + modem boot + board audio).
 * Idempotent; runs once when g_radio_enabled flips on.  This is the exact
 * code that previously ran unconditionally at boot in rtx_init. */
static void rtx_bringupRadio(void)
{
    if(radioBroughtUp)
        return;

    radio_init(&rtxStatus);
    radio_updateConfiguration();
    audio_init();

    rssi           = radio_getRssi();
    reinitFilter   = true;
    enterRx        = true;
    radioBroughtUp = true;
}

void rtx_init(pthread_mutex_t *m)
{
    cfgMutex = m;
    newCnf   = NULL;

    /* Default config: analog FM, RX-only.  The UI sends the real VFO
     * frequency via rtx_configure() shortly after boot (first sync_rtx). */
    memset(&rtxStatus, 0, sizeof(rtxStatus));
    rtxStatus.opMode      = OPMODE_FM;
    rtxStatus.bandwidth   = BW_25;
    rtxStatus.opStatus    = OFF;
    rtxStatus.rxFrequency = 430000000;
    rtxStatus.txFrequency = 430000000;
    rtxStatus.sqlLevel    = 1;
    rtxStatus.txDisable   = 1;          /* RX-only build */

    /* Boot-inhibit harness (g_radio_enabled, declared above with default 1 =
     * radio up at boot): when flipped to 0, the whole radio bring-up (AT1846S
     * init + modem boot + audio init) is DEFERRED so the system boots clean
     * (UI + diag bridge fully usable) with the radio bus untouched; the diag
     * 'F' op then brings the radio up ON COMMAND.  Kept as a debugging tool --
     * it is how the HW-I2C bring-up wedge was isolated without boot-time
     * power-cycle loops (docs/TASK_hw_i2c_migration.md). */
    rssi         = -127;
    reinitFilter = true;
    enterRx      = true;
    audioOpen    = false;
    cfgApplied   = false;

    if(g_radio_enabled)
        rtx_bringupRadio();
}

void rtx_terminate(void)
{
    rtx_setAudio(false);
    rtxStatus.opStatus = OFF;
    radio_terminate();
}

void rtx_configure(const rtxStatus_t *cfg)
{
    pthread_mutex_lock(cfgMutex);
    newCnf = cfg;
    pthread_mutex_unlock(cfgMutex);
}

rtxStatus_t rtx_getCurrentStatus(void)
{
    return rtxStatus;
}

void rtx_task(void)
{
    /* Watchdog is now fed by watchdog_kick() from rtx_threadFunc (see
     * watchdog_HD2.c) -- no longer this task's job. */
    HD2_CRUMB_STAMP(rtx);          /* hard-lock forensics (hd2_crumb.h) */
    {                              /* + WAKE_CH arm-state snapshot (<=30 ms pre-death) */
        extern void hd2_crumb_wake_state(void);
        hd2_crumb_wake_state();
    }

    /* Radio boot-inhibit: idle until enabled (diag 'F'), then bring the radio
     * up here, in the rtx-thread context, ONCE.  Lets us trigger the radio
     * init on command and observe where it wedges. */
    if(g_radio_enabled == 0)
    {
        sleepFor(0u, 30u);
        return;
    }
    if(!radioBroughtUp)
        rtx_bringupRadio();

    /* RF freeze: skip every chip access (AT1846S bit-bang reads/writes,
     * audio GPIO RMW) but keep the thread alive and pacing.  A pending
     * newCnf stays queued in rtx_configure()'s slot until un-frozen. */
    if(g_rf_freeze != 0u)
    {
        sleepFor(0u, 30u);
        return;
    }

    /* FM-broadcast mode (UI FM screen, hd2_fm_broadcast.cpp): the dedicated 0x20
     * tuner owns the speaker route (PTB10/PTB4/PTB17) while active.  Stand
     * down from the audio path: close the 2-way RX->speaker gate if it was
     * open (mutes the AT1846S + re-parks the GPIOs; the FM worker re-asserts
     * its own routing every 250 ms pass, so a transient clobber self-heals)
     * and skip the squelch gating below -- otherwise a squelch flicker on the
     * parked VFO splats 2-way static over the broadcast audio and the
     * follow-up disconnect kills the broadcast route (the "static on all
     * frequencies" bug, 2026-06-13). */
    if(g_fm_active != 0u)
    {
        rtx_setAudio(false);
        sleepFor(0u, 30u);
        return;
    }

    /* Pick up a pending configuration, preserving the live opStatus. */
    bool reconfigure = false;
    if(pthread_mutex_trylock(cfgMutex) == 0)
    {
        if(newCnf != NULL)
        {
            uint8_t tmp = rtxStatus.opStatus;
            memcpy(&rtxStatus, newCnf, sizeof(rtxStatus_t));
            rtxStatus.opStatus  = tmp;
            reconfigure = true;
            cfgApplied  = true;         /* real squelch level is in -> audio gate may open */
            newCnf = NULL;
        }
        pthread_mutex_unlock(cfgMutex);
    }

    if(reconfigure)
    {
        /* Mute before retuning: don't leave the old channel's audio routed to
         * the speaker while the VCO moves. */
        rtx_setAudio(false);

        /* Apply the new mode/band/frequency, then drop to OFF and request a
         * fresh RX entry so we re-tune the VCO to the new RX frequency. */
        radio_setOpmode((enum opmode)rtxStatus.opMode);
        radio_updateConfiguration();
        radio_disableRtx();
        rtxStatus.opStatus = OFF;
        enterRx            = true;
        reinitFilter       = true;      /* AT1846S returns full-scale just after a retune */
    }

    /*
     * PTT -> FM voice TX (HW-verified 2026-06-11; chip+modem keying lives in
     * radio_enableTx / radio_disableRtx).  PTT is GPIOA.11 active-low via
     * platform_getPttStatus().  Hard 60 s TX timeout (2000 passes @30 ms).
     * While transmitting: speaker muted, RSSI/squelch skipped, red LED on.
     */
    static bool     txActive;
    static uint32_t txTicks;
    static bool     txIsVox;          /* current key started by VOX, not PTT  */
    static uint32_t voxHang;          /* VOX hangtime countdown (passes)      */
    static bool     toneBurstOn;      /* 1750 Hz key-up burst sounding        */
    static uint32_t toneBurstTicks;
    static bool     tailHolding;      /* reverse-burst hold before dekey      */
    static uint32_t tailTicks;

    bool ptt = platform_getPttStatus();

    /*
     * Anti-false-key debounce: the PTT pad (GPIOA.11) is shared with
     * UART2 RXD, so a powered GPS module's NMEA produces sub-ms low
     * pulses.  Primary defence is platform_init parking the GPS power
     * rail (GPIOB.15) off; this 2-consecutive-pass check (60 ms apart)
     * is belt-and-braces.  Deliberately NO inner delay loop: both a
     * delayUs busy-scan and 1 ms kernel sleeps here wedged the rtx/diag
     * threads (os_timer fragility) -- 2026-06-11.  Release needs no
     * debounce: a held button holds the pad solid low.
     */
    static uint8_t pttRun;
    if(ptt && !txActive)
    {
        if(pttRun < 2u) { pttRun++; ptt = false; }
    }
    else if(!ptt)
    {
        pttRun = 0;
    }

    /* 1750 Hz tone-burst tick-down (sounds for the first ~0.75 s of a key). */
    if(toneBurstOn && (++toneBurstTicks >= RTX_TONE_BURST_PASSES))
    {
        rtx_toneBurstStop();
        toneBurstOn = false;
    }

    if(txActive)
    {
        txTicks++;

        /* A hardware PTT press during a VOX key converts it to a held PTT key. */
        if(txIsVox && ptt) txIsVox = false;

        /* VOX-keyed: hold while speech continues, then run the hangtime. */
        if(txIsVox)
        {
            if(hd2_vox_detected()) voxHang = RTX_VOX_HANG_PASSES;
            else if(voxHang > 0u)  voxHang--;
        }

        bool drop = txIsVox ? (voxHang == 0u) : (!ptt);
        if(drop || (txTicks > 2000u))
        {
            /* CTCSS/DCS tail elimination: hold the keyed carrier with the
             * reverse-burst (reg 0x30[11]) for ~180 ms before the real dekey,
             * so the far-end decoder drops carrier without a squelch tail. */
            if(rtxStatus.tailElim && rtxStatus.txToneEn && !tailHolding)
            {
                hd2_at1846s_write(0x30, (uint16_t)(hd2_at1846s_read(0x30) | 0x0800u));
                tailHolding = true;
                tailTicks   = 0;
            }
            if(tailHolding && (++tailTicks < RTX_TAIL_ELIM_PASSES))
            {
                sleepFor(0u, 30u);
                return;                  /* keep holding the reverse burst   */
            }

            if(toneBurstOn) { rtx_toneBurstStop(); toneBurstOn = false; }
            radio_disableRtx();          /* dekey (0x30=0x4006 clears bit11) */
            platform_ledOff(RED);
            txActive    = false;
            txIsVox     = false;
            tailHolding = false;
            tailTicks   = 0;
            rtxStatus.opStatus = OFF;
            enterRx            = true;   /* retune + re-enter RX next pass   */
            reinitFilter       = true;
        }
        sleepFor(0u, 30u);
        return;
    }

    /* Key entry: hardware PTT always wins; VOX only when PTT is up, we are
     * listening, and the squelch is closed (don't key over an incoming
     * signal or self-trigger off speaker audio). */
    bool keyByPtt = (ptt && cfgApplied && (rtxStatus.txDisable == 0u));
    bool keyByVox = false;
    if(!keyByPtt && cfgApplied && (rtxStatus.txDisable == 0u)
       && (rtxStatus.vox != 0u) && (rtxStatus.opStatus == RX)
       && !rtx_rxSquelchOpen())
        keyByVox = hd2_vox_detected();

    if(keyByPtt || keyByVox)
    {
        rtx_setAudio(false);             /* speaker off while keyed         */
        platform_ledOff(GREEN);          /* RX-signal LED off while keying  */
        radio_enableTx();                /* refuses non-FM / out-of-band    */
        if(radio_getStatus() == TX)
        {
            txActive = true;
            txIsVox  = (keyByVox && !keyByPtt);
            txTicks  = 0;
            voxHang  = RTX_VOX_HANG_PASSES;
            if(rtxStatus.toneBurst1750)
            {
                rtx_toneBurstStart();
                toneBurstOn    = true;
                toneBurstTicks = 0;
            }
            rtxStatus.opStatus = TX;
            platform_ledOn(RED);
            sleepFor(0u, 30u);
            return;
        }
    }

    /* Idle: keep the radio listening on the configured RX frequency. */
    if((rtxStatus.opStatus == OFF) && enterRx)
    {
        radio_enableRx();
        rtxStatus.opStatus = RX;
        enterRx            = false;

        /* (Re-)arm the VOX detector -- the RX entry's setFuncMode cleared
         * vox_on (reg 0x30 bit4). */
        uint8_t vth;
        if((rtxStatus.txDisable == 0u) && rtx_voxThresh(rtxStatus.vox, &vth))
            hd2_vox_enable(vth, vth);
        else
            hd2_vox_disable();
    }

    /* RSSI low-pass filter (15.16 fixed point), only while receiving.
     * Equivalent float: rssi = 0.74*radio_getRssi() + 0.26*rssi.
     * Skip a step right after a retune (reinitFilter) to avoid latching the
     * AT1846S's momentary full-scale reading. */
    if(rtxStatus.opStatus == RX)
    {
        bool justReinit = reinitFilter;

        if(!reinitFilter)
        {
            /* Peak-hold (instant rise, slow fall) de-jitters the S-meter: with
             * NO carrier the chip refreshes reg-0x1B rssi_db only periodically
             * and reads near-zero in between (HW 2026-06-14), so a plain filter
             * flickers the bar to 0.  Peak-hold latches the periodic true value
             * and ignores the stale dips; a real signal drop still falls (2 dB/
             * tick @ 33 Hz).  With a carrier rssi_db is already stable. */
            rssi_t raw = radio_getRssi();
            if(raw >= rssi)              rssi = raw;          /* instant rise   */
            else if((rssi - raw) > 2)    rssi = (rssi_t)(rssi - 2); /* slow fall */
            else                         rssi = raw;
        }
        else
        {
            rssi         = radio_getRssi();
            reinitFilter = false;
        }

        /* Update the cached squelch state from the AT1846S's own comparator
         * (sq_cmp) once per tick -- see rtx_rxSquelchOpen().  justReinit skips
         * the post-retune settling tick (the chip momentarily reads full-scale).
         *
         * Gate the RX->speaker audio on the RF squelch: route the AT1846S demod
         * audio while sq_cmp is set, mute when it drops.  For tone-coded channels
         * (rxToneEn) additionally require the CTCSS/DCS sub-audio match, so an
         * unkeyed-tone carrier reads signal yet stays muted (true tone squelch). */
        if(!justReinit) rtx_updateSquelch();
        bool sqlOpen = rtx_rxSquelchOpen();
        if(sqlOpen && rtxStatus.rxToneEn)
            sqlOpen = radio_checkRxDigitalSquelch();
        bool audioOn = cfgApplied && !justReinit && sqlOpen;
        rtx_setAudio(audioOn);
        /* Green LED = RX signal present (complements the red TX LED). */
        if(audioOn) platform_ledOn(GREEN); else platform_ledOff(GREEN);
    }
    else
    {
        rtx_setAudio(false);
        platform_ledOff(GREEN);
    }

    sleepFor(0u, 30u);                  /* 33 Hz update rate */
}

rssi_t rtx_getRssi(void)
{
    return rssi;
}

/* Current RX frequency (Hz) for the diag at_reinit op ('I'): lets the
 * one-shot AT1846S re-init (hd2_at1846s_reinit) re-apply the frequency the
 * rtx thread last configured, without dragging rtxStatus_t into the diag. */
uint32_t hd2_rtx_getRxFreq(void)
{
    return (uint32_t)rtxStatus.rxFrequency;
}

/* Bench/diag activation of the FM TX-extras (diag op 'e').  No settings/UI
 * toggle exists yet, so this forces the flags live until the next reconfigure.
 *   flags bit0 = 1750 Hz key-up tone burst, bit1 = CTCSS/DCS tail elimination.
 *   vox = VOX level 0..5 (0 = off).  VOX is (re)armed on the next RX entry. */
void hd2_rtx_setFmExtras(uint8_t flags, uint8_t vox)
{
    rtxStatus.toneBurst1750 = (flags & 0x01u) ? 1u : 0u;
    rtxStatus.tailElim      = (flags & 0x02u) ? 1u : 0u;
    rtxStatus.vox           = (vox > 5u) ? 5u : vox;
}

/* Cached squelch state, updated once per RX tick by rtx_updateSquelch() (rtx
 * thread only).  rtx_rxSquelchOpen() returns this -- it is also called from the
 * UI thread (ui.c), so it must NOT touch the I2C bus itself. */
static volatile bool s_sqlOpen = false;

/* Update the squelch decision from the AT1846S's OWN comparator, reg 0x1C[0]
 * sq_cmp (RSSI + noise vs the 0x48/0x49 thresholds, with the chip's built-in
 * hi/lo hysteresis -- set from sqlLevel by radio_config's setSquelchLevel).
 *
 * WHY not threshold our raw RSSI like before: reg 0x1B reads ~40 dB low on
 * ~10-20% of I2C transfers (HW-confirmed 2026-06-14), which the old ±4 dBm
 * window could not absorb -> the audio gate fluttered closed mid-signal and
 * chopped the demod audio (broke APRS RX capture; flickered the S-meter to 0).
 * sq_cmp is the chip's debounced decision and is far steadier; a 3-tick (~90 ms)
 * close debounce here absorbs the rare corrupt 0x1C read.  Matches the vendor
 * (rx_squelch_monitor_tick reads 0x1C bit0). */
static void rtx_updateSquelch(void)
{
    static uint8_t closeCnt = 0u;
    if(hd2_rx_carrier_detected())       { s_sqlOpen = true;  closeCnt = 0u; }
    else if(s_sqlOpen && ++closeCnt >= 3u) { s_sqlOpen = false; closeCnt = 0u; }
}

bool rtx_rxSquelchOpen(void)
{
    return s_sqlOpen;
}
