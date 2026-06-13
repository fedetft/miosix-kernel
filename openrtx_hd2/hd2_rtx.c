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

/* Watchdog heartbeat (hard-lock forensics, 2026-06-12): armed ~10 s on the
 * first rtx_task pass and fed every pass.  A system lock stops the feeds ->
 * full-chip WDT reset -> the radio reboots on its own instead of needing the
 * power switch (which doesn't actually cycle the board with the serial cable
 * attached).  Diag op 'X' toggles it (1=on / 2=off) or forces a reboot (0). */
volatile uint32_t         g_wdt_auto = 1;

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
    /* Watchdog heartbeat -- see g_wdt_auto above.  Armed lazily so the boot
     * path (codeplug enumeration etc.) can take as long as it likes. */
    if(g_wdt_auto != 0u)
    {
        static bool wdtArmed;
        if(!wdtArmed) { hd2_wdt_arm(10u); wdtArmed = true; }
        hd2_wdt_feed();
    }
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

    if(txActive)
    {
        txTicks++;
        if(!ptt || (txTicks > 2000u))
        {
            radio_disableRtx();          /* dekey + FM_PTT off + chip OFF   */
            platform_ledOff(RED);
            txActive           = false;
            rtxStatus.opStatus = OFF;
            enterRx            = true;   /* retune + re-enter RX next pass  */
            reinitFilter       = true;
        }
        sleepFor(0u, 30u);
        return;
    }

    if(ptt && cfgApplied && (rtxStatus.txDisable == 0u))
    {
        rtx_setAudio(false);             /* speaker off while keyed         */
        radio_enableTx();                /* refuses non-FM / out-of-band    */
        if(radio_getStatus() == TX)
        {
            txActive           = true;
            txTicks            = 0;
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
            int32_t filt = radio_getRssi() * 0xBD70   /* 0.74 */
                         + rssi            * 0x428F;   /* 0.26 */
            rssi = (filt + 32768) >> 16;              /* round to nearest */
        }
        else
        {
            rssi         = radio_getRssi();
            reinitFilter = false;
        }

        /* Gate the RX->speaker audio on the RF squelch: route the AT1846S
         * demod audio while a signal is over threshold, mute when it drops.
         * Skip the post-retune settling tick (justReinit), where the AT1846S
         * momentarily reads full-scale and would briefly false-open the path.
         *
         * For tone-coded channels (rxToneEn) additionally require the CTCSS/DCS
         * sub-audio match: RSSI still drives the S-meter/LED via
         * rtx_rxSquelchOpen(), but the speaker only opens when the programmed
         * tone is detected -- so an unkeyed-tone carrier reads signal yet stays
         * muted (true tone squelch). */
        bool sqlOpen = rtx_rxSquelchOpen();
        if(sqlOpen && rtxStatus.rxToneEn)
            sqlOpen = radio_checkRxDigitalSquelch();
        rtx_setAudio(cfgApplied && !justReinit && sqlOpen);
    }
    else
    {
        rtx_setAudio(false);
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

bool rtx_rxSquelchOpen(void)
{
    /* RF squelch: map sqlLevel (0..15) to -127..-61 dBm, compare to RSSI.
     * Drives both the UI/LED and (via rtx_task) the RX->speaker audio gate.
     *
     * ±4 dBm hysteresis (8 dBm window), matching the 2026-06-10 cable-noise
     * lockup fix in OpMode_FM.cpp -- which this build does NOT link, so the
     * fix never applied here and the ±1 window kept the "RSSI flutter ->
     * squelch dither -> EVENT_STATUS standby/backlight/redraw thrash ->
     * lockup" path alive (rf_freeze A/B 2026-06-11: freezing this poll stops
     * the lockups).  USB-serial ground-loop/RF-pickup flutters the RSSI a few
     * dB; 8 dBm absorbs it. */
    static bool open = false;
    rssi_t squelch = -127 + (rtxStatus.sqlLevel * 66) / 15;
    if(!open && (rssi > squelch + 4)) open = true;
    else if(open && (rssi < squelch - 4)) open = false;
    return open;
}
