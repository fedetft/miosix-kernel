/***************************************************************************
 *   CK803S (C-SKY V2) / HR_C7000 register definitions for modern Miosix.  *
 *   Named MMIO + control-register accessors (no magic numbers). Addresses  *
 *   verified in-tree (irq_test.c / irq_entry.S / docs/irq_handlers.md) and *
 *   reused from the validated tmp/miosix_phase1_draft scaffold.            *
 *   GPL v2+ with the Miosix linking exception.                            *
 ***************************************************************************/

#pragma once

#include <stdint.h>

/* ---- DW_apb_timers @ 0x14000000, channel stride 0x14 --------------------
 * ch0/Timer1 = free-running getTick/delay timebase. ch1/Timer2 = OS tick.   */
#define HD2_TIMER_BASE   0x14000000u
#define HD2_TMR(ch,off)  (*(volatile uint32_t*)(HD2_TIMER_BASE+(ch)*0x14u+(off)))
#define HD2_T1_LOAD      HD2_TMR(0u,0x00u)
#define HD2_T1_CURVAL    HD2_TMR(0u,0x04u)
#define HD2_T1_CTRL      HD2_TMR(0u,0x08u)
#define HD2_T2_LOAD      HD2_TMR(1u,0x00u)
#define HD2_T2_CURVAL    HD2_TMR(1u,0x04u)
#define HD2_T2_CTRL      HD2_TMR(1u,0x08u)   /* b0 en, b1 reload, b2 imask */
#define HD2_T2_EOI       HD2_TMR(1u,0x0cu)   /* READ clears Timer2 IRQ */
#define HD2_TIMER_HZ     42000000u           /* measured (project_hd2_timebase) */

/* ---- PIC interrupt controller @ 0x17000000 ------------------------------ */
#define HD2_PIC_BASE     0x17000000u
#define HD2_PIC_MODE     (*(volatile uint32_t*)(HD2_PIC_BASE+0x00u))
#define HD2_PIC_PO       (*(volatile uint32_t*)(HD2_PIC_BASE+0x04u))
#define HD2_PIC_MASK     (*(volatile uint32_t*)(HD2_PIC_BASE+0x08u)) /* bit=0 ENABLES */
#define HD2_PIC_COW1     (*(volatile uint32_t*)(HD2_PIC_BASE+0x10u)) /* int-end: write */
#define HD2_PIC_COW1_EOI 0x4u                                        /* bit2 = eoi */
#define HD2_PIC_INT_ST   (*(volatile uint32_t*)(HD2_PIC_BASE+0x44u))
#define HD2_PIC_INT_ST1  (*(volatile uint32_t*)(HD2_PIC_BASE+0x48u))
#define HD2_PIC_MODE1    (*(volatile uint32_t*)(HD2_PIC_BASE+0x60u))
#define HD2_PIC_PO1      (*(volatile uint32_t*)(HD2_PIC_BASE+0x64u))
#define HD2_PIC_MASK1    (*(volatile uint32_t*)(HD2_PIC_BASE+0x68u)) /* srcs 32-63 */

/* PIC autovector base: VEC for source x = x + HD2_PIC_VECTOR0 (PIC_VECTOR=0x0c,
 * reset default 0x20). So PIC sources occupy VBR vectors 32..95. */
#define HD2_PIC_VECTOR0  32u
#define HD2_VBR_NVEC     128u   /* CK803S vector space 0..127 (PIC valid 32..112) */

/* Timer2 = PIC source 2 -> autovector 32+2 = 34. trap0 -> CPU-exc vector 16. */
#define HD2_TIMER2_SRC   2u
#define HD2_TIMER2_VEC   34u
#define HD2_YIELD_VEC    16u    /* VBR[16] = trap0 (scheduler-invoke) handler */

/* PSR bit positions (standard CK803S; verify on silicon). */
#define HD2_PSR_IE_BIT   6u
#define HD2_PSR_EE_BIT   8u

/* BOOTROM reset entry (project_hd2_bootrom_reset): jump to 0x4, not 0x03000000. */
#define HD2_BOOTROM_RESET 0x00000004u

static inline unsigned int csky_get_psr(void)
{ unsigned int v; asm volatile("mfcr %0, psr":"=r"(v)); return v; }
static inline void csky_set_psr(unsigned int v)
{ asm volatile("mtcr %0, psr"::"r"(v)); }
static inline void csky_set_vbr(const void *table)
{ asm volatile("mtcr %0, cr<1, 0>"::"r"(table)); }   /* VBR = cr<1,0> */
