/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Hard-lock forensics breadcrumbs (2026-06-12, "bug #2" hunt -- see
 * docs/TASK_hw_i2c_migration.md).  A 92-byte block near the top of SRAM
 * (0x4F000; hd2.ld shrinks the ram region so the heap can't reach it, and
 * .bss zeroing doesn't cover it) that key contexts stamp continuously.
 * NOT at the very top: the IAP (which runs on every boot, including WDT
 * resets) uses the top of SRAM as its stack/workspace and scribbles
 * ~0x4FE00..0x50000 (signature-ladder-measured 2026-06-13 across a real
 * WDT crash cycle; everything at/below 0x4FE00 survived bit-perfect --
 * 4 KiB of margin for deeper IAP paths like YMODEM).  Contexts stamped:
 *
 *   - rtx thread        (rtx_task, every ~30 ms pass)
 *   - WAKE_CH timer ISR (every scheduled wakeup, >=33 Hz under normal load)
 *   - TIME_CH overflow ISR (every ~102.26 s -- also probes the wrap theory)
 *   - diag thread       (every serviced op)
 *
 * SRAM survives a watchdog reset, so after the WDT auto-recovery reboots a
 * hard-locked radio, IRQbspInit snapshots the previous life's block before
 * the new one starts stamping; diag op 'H' dumps both.  Reading it answers
 * the two questions a hard lock poses:
 *   1. which context stopped FIRST (lowest last-stamp tick), and
 *   2. did IRQs keep running after threads died (ISR ticks advancing past
 *      thread ticks = scheduler/starvation death; all equal = CPU/AHB death).
 *
 * Stamps are {counter, TIMER-ch0 CURVAL} pairs -- raw 42 MHz hardware ticks,
 * no kernel calls, IRQ-safe.
 */

#pragma once

#include <stdint.h>

#define HD2_CRUMB_BASE   0x0004f000u
#define HD2_CRUMB_MAGIC  0x48443243u            /* "HD2C" */

typedef struct
{
    uint32_t magic;
    uint32_t boot_count;
    uint32_t rtx_hb;       uint32_t rtx_tick;
    uint32_t wake_hb;      uint32_t wake_tick;
    uint32_t ovf_hb;       uint32_t ovf_tick;
    uint32_t diag_hb;      uint32_t diag_tick;
    /* WAKE_CH one-shot arm state, sampled by rtx_task each pass (i.e. the
     * state <=30 ms before a death).  Post-mortem discrimination for the
     * lost-wakeup bug: armed-but-never-fired (CTRL enabled, LOAD/CURVAL sane)
     * = channel/PIC delivery race; never-re-armed (CTRL disabled or irqNs ==
     * infinity marker 0xffffffff) = kernel-side loss.  LIVE-MEASURED healthy
     * base rate (2026-06-13): LOAD is 0x800..0xa400, never 1; both early
     * deaths showed LOAD=1 (the fire-ASAP/past-deadline arm) at the final
     * rtx pass -- the rel=1 state is death-adjacent. */
    uint32_t wake_ctrl;    /* TMR_CTRL(WAKE_CH) */
    uint32_t wake_load;    /* TMR_LOAD(WAKE_CH) */
    uint32_t wake_cur;     /* TMR_CURVAL(WAKE_CH) */
    uint32_t irqns_lo;     /* miosix irqNs low 32 (0xffffffff in both = none) */
    uint32_t irqns_hi;
    /* Dispatch-chain forensics: disp = csky_isr_dispatch ran to completion
     * (its very last statement).  disp_tick OLDER than wake_tick post-mortem
     * = the final wake's dispatch never completed (died in the handler /
     * IRQwakeThreads / scheduler-select / EOI tail).  arm = every
     * IRQosTimerSetInterrupt call, with the rel ticks it armed (arm newer
     * than wake = armed-but-never-fired; wake newer = fired-never-rearmed). */
    uint32_t disp_hb;      uint32_t disp_tick;
    uint32_t arm_hb;       uint32_t arm_rel;
    /* Captured by the NEXT boot's IRQinitIrqTable BEFORE it wipes the PIC:
     * (INT_ST&0xff) | (MASK&0xff)<<8 | (INT_ST1&0xff)<<16 | (MASK1&0xff)<<24.
     * NOTE: measured 2026-06-13 to be POLLUTED by the WDT-reset/IAP path
     * (a clean X0 reboot of a healthy radio also reads 0xfd) -- do not
     * read death-state into it. */
    uint32_t pic_snap;
    /* The resume frame the dispatcher's CTX_RESTORE is about to consume,
     * recorded at the END of every csky_isr_dispatch: the chosen thread's
     * saved SP (ctxsave[0][0]) and that frame's EPC/EPSR words.  Post-mortem
     * (the final dispatch before death) this is the EXACT rte target that
     * never ran another stamp: res_epc names the fatal code location,
     * res_epsr bit6 says whether IE was restored. */
    uint32_t res_sp;
    uint32_t res_epc;
    uint32_t res_epsr;
} hd2_crumb_t;                                   /* 92 bytes (23 x uint32_t) */

#define HD2_CRUMB ((volatile hd2_crumb_t *)HD2_CRUMB_BASE)

#ifdef __cplusplus
extern "C" {
#endif
/* Previous life's block, snapshotted by IRQbspInit (bsp.cpp) before the new
 * life starts stamping.  Diag op 'H' dumps it. */
extern hd2_crumb_t hd2_crumb_prev;
#ifdef __cplusplus
}
#endif

/* TIMER ch0 (free-running timebase) current value -- raw, IRQ-safe. */
#define HD2_CRUMB_NOW() (*(volatile uint32_t *)0x14000004u)

#define HD2_CRUMB_STAMP(field) do {                            \
        HD2_CRUMB->field##_hb++;                               \
        HD2_CRUMB->field##_tick = HD2_CRUMB_NOW();             \
    } while (0)
