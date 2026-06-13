/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 FM broadcast receive worker thread, gated by the UI FM screen.
 *
 * Drives the DEDICATED broadcast-FM tuner chip (I2C 0x20 on the GPIOA bit-bang
 * bus) via the fm_broadcast_HD2 driver -- NOT the AT1846S transceiver and NOT
 * the HR_C7000 codec.  Broadcast audio is analog out of the 0x20 chip and gated
 * to the speaker by GPIO (PTB10 route + PTB20 enable), so there is no codec /
 * socsys audio path to bring up here.  (Earlier versions of this worker used
 * hd2_radio_selftest = AT1846S + codec, which is why FM was silent -- wrong
 * subsystem.  See scripts/labels/fm_broadcast.py + the live GPIO/init-block
 * verification in the fm-broadcast-0x20-tuner-driver task.)
 *
 * The OpenRTX UI's FM_RADIO screen (entered with SK2 / KEY_F3) sets g_fm_active.
 * While active, this thread tunes g_fm_test_freq and keeps FM audio on; when the
 * user leaves the screen (g_fm_active=0) it powers the tuner down. g_fm_test_freq
 * (Hz) is set by the UI (UP/DN) or pokeable; g_fm_rssi reports the chip status.
 */

#include <miosix.h>
#include <pthread.h>
#include <cstdint>
#include "hd2_regs.h"          /* GPIOB_DR, SPKR_AMP_BIT */
#include "drivers/baseband/fm_broadcast_HD2.h"

using namespace miosix;

extern "C" volatile uint32_t g_fm_test_freq;   /* tune target, Hz (default 103.6M) */

/* AT1846S chip-side RX AF mute (reg 0x30 bit7 RMW, radio_test_HD2.cpp).  The
 * 2-way AFOUT and the broadcast tuner output share the analog node into the
 * speaker amp; the boot config leaves the AT1846S UNMUTED (LISTEN), which in
 * 2-way mode is hidden by the closed amp -- but broadcast mode opens the amp
 * and the AT1846S demod noise mixes in (live-isolated 2026-06-13: muting the
 * chip cleared the static, FM quieting confirmed the source).  Returns the
 * resulting reg 0x30. */
extern "C" uint16_t hd2_at1846s_afmute(uint32_t mute);

/* RF-freeze flag (radio_test_HD2.cpp, loader op 'z').  The broadcast tuner
 * shares the GPIOA bit-bang I2C bus with the AT1846S and drives PTB10/PTB20,
 * so while frozen this worker must not touch the chip at all -- the thread
 * keeps looping (heartbeat advances) but skips ALL tuner I/O, including the
 * 250 ms RSSI poll and any powerup/powerdown transition. */
extern "C" volatile uint32_t g_rf_freeze;

extern "C" {
volatile uint32_t g_fm_active      = 0;        /* set by the UI FM screen (SK2)   */
volatile uint32_t g_fm_loops       = 0;        /* thread heartbeat                */
volatile uint32_t g_fm_applied_freq= 0;        /* last freq actually tuned        */
volatile uint32_t g_fm_rssi        = 0;        /* tuner status: bit8=tuned, [7:0] not used */
volatile uint32_t g_fm_mode        = 0;        /* read-back channel from the tuner          */
}

static void *fmThread(void *)
{
    Thread::sleep(2500);            /* let boot/UI settle before any heavy init */

    bool     running  = false;
    uint32_t lastFreq = 0;

    for(;;)
    {
        if(g_rf_freeze)
        {
            /* rf_freeze: no tuner I2C / GPIO; keep the heartbeat alive.  The
             * running/lastFreq state is preserved, so on un-freeze the worker
             * resumes exactly where it was. */
            g_fm_loops++;
            Thread::sleep(250);
            continue;
        }

        if(g_fm_active)
        {
            uint32_t f = g_fm_test_freq;            /* Hz */
            if(!running)
            {
                /* Power up the 0x20 tuner (init burst + band/strobe) and route
                 * its analog audio to the speaker.  ~ms, not the AT1846S ~700ms.
                 * Mute the AT1846S AFOUT (shared analog node, see above) --
                 * left muted on exit; the 2-way squelch gate (audio_connect)
                 * unmutes it again on the next signal. */
                fm_broadcast_powerup();
                fm_broadcast_route_speaker();
                (void)hd2_at1846s_afmute(1);
                running  = true;
                lastFreq = 0;                       /* force a tune below */
            }
            if(f != lastFreq)
            {
                fm_broadcast_tune(f / 1000u);       /* driver takes kHz */
                lastFreq = f;
                g_fm_applied_freq = f;
            }

            /* Re-assert the speaker route every pass (idempotent GPIO sets):
             * the rtx thread stands down while g_fm_active (hd2_rtx.c), but a
             * transient clobber from its stand-down disconnect -- or any other
             * audio-path writer -- self-heals within one 250 ms loop.  Without
             * this, one 2-way squelch flicker before the stand-down landed
             * could leave the broadcast route parked off. */
            fm_broadcast_route_speaker();

            /* RDA5802E status: real RSSI (reg 0x0B[15:9]) + STC/READCHAN. Pack
             * RSSI into the low byte and the tune-complete flag into bit8 so the
             * UI shows a moving level when locked. */
            bool tuned = false; uint16_t chan = 0;
            uint8_t rssi = fm_broadcast_rssi();
            fm_broadcast_status(&tuned, &chan);
            g_fm_rssi = (uint32_t)rssi | (tuned ? 0x100u : 0u);
            g_fm_mode = chan;
        }
        else if(running)
        {
            fm_broadcast_powerdown();               /* PTB10/PTB20 high: standby */
            running = false;
        }

        g_fm_loops++;
        Thread::sleep(250);
    }
    return nullptr;
}

/* Start the FM worker thread. Call from the HD2 entry (main.cpp) after
 * openrtx_init (platform/radio/i2c up). HD2-only. */
extern "C" void hd2_fm_probe_start()
{
    static pthread_t fm_thread;
    pthread_create(&fm_thread, nullptr, fmThread, nullptr);
}
