/***************************************************************************
 *   Copyright (C) 2010-2024 by Terraneo Federico                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#pragma once

#include "interfaces/arch_registers.h"

#ifndef __FPU_PRESENT
#define __FPU_PRESENT 0 //__FPU_PRESENT undefined means no FPU
#endif

/**
 * \addtogroup Interfaces
 * \{
 */

#if __FPU_PRESENT==1

/*
 * In this architecture, registers are saved in the following order:
 * *ctxsave+104 --> psplim
 * *ctxsave+100 --> s31
 * ...
 * *ctxsave+40  --> s16
 * *ctxsave+36  --> lr (contains EXC_RETURN whose bit #4 tells if fpu is used)
 * *ctxsave+32  --> r11
 * *ctxsave+28  --> r10
 * *ctxsave+24  --> r9
 * *ctxsave+20  --> r8
 * *ctxsave+16  --> r7
 * *ctxsave+12  --> r6
 * *ctxsave+8   --> r5
 * *ctxsave+4   --> r4
 * *ctxsave+0   --> psp
 */

/**
 * \internal
 * \def saveContext()
 * Save context from an interrupt<br>
 * Must be the first line of an IRQ where a context switch can happen.
 * The IRQ must be "naked" to prevent the compiler from generating context save.
 *
 * A note on the dmb instruction, without it a race condition was observed
 * between pauseKernel() and IRQrunScheduler(). pauseKernel() uses an strex
 * instruction to store a value in the global variable kernel_running which is
 * tested by the context switch code in IRQrunScheduler(). Without the memory
 * barrier IRQrunScheduler() would occasionally read the previous value and
 * perform a context switch while the kernel was paused, leading to deadlock.
 * The failure was only observed within the exception_test() in the testsuite
 * running on the stm32f429zi_stm32f4discovery.
 * TODO: According to ARM's docs, the DMB should probably have been a CLREX.
 * On SMP, the DMB should be a CLREX followed by a DSB, because DMB just
 * ensures no reordering, while DSB also stalls until all writes have been
 * committed. This is important to ensure the state of the thread stays
 * consistent if it's moved to another core.
 */
#define saveContext()                                                         \
    asm volatile("   mrs    r1,  psp            \n"/*get PROCESS stack ptr  */ \
                 "   ldr    r3,  =ctxsave       \n"/*get current context    */ \
                 "   ldr    r0,  [r3]           \n"                            \
                 "   stmia  r0!, {r1,r4-r11,lr} \n"/*save r1(psp),r4-r11,lr */ \
                 "   lsls   r2,  lr,  #27       \n"/*check if bit #4 is set */ \
                 "   bmi    0f                  \n"                            \
                 "   vstmia.32 r0, {s16-s31}    \n"/*save s16-s31 if we need*/ \
                 "0: mov    r4,  r3             \n"/*save for restoreContext*/ \
                 "   dmb                        \n"                            \
                 );

/**
 * \def restoreContext()
 * Restore context in an IRQ where saveContext() is used. Must be the last line
 * of an IRQ where a context switch can happen. The IRQ must be "naked" to
 * prevent the compiler from generating context restore.
 */
#define restoreContext()                                                      \
    asm volatile("   ldr    r0,  [r4]           \n"/*get current context    */ \
                 "   ldmia  r0!, {r1,r4-r11,lr} \n"/*load r1(psp),r4-r11,lr */ \
                 "   lsls   r2,  lr,  #27       \n"/*check if bit #4 is set */ \
                 "   bmi    0f                  \n"                            \
                 "   vldmia.32 r0, {s16-s31}    \n"/*restore s16-s31 if need*/ \
                 "0: ldr    r0,  [r0, #64]      \n"/*get psplim*/              \
                 "   msr    psplim, r0          \n"/*update PROCESS sp limit*/ \
                 "   msr    psp, r1             \n"/*restore PROCESS sp*/      \
                 "   bx     lr                  \n"/*return*/                  \
                 );

#else //__FPU_PRESENT==1

/*
 * In this architecture, registers are saved in the following order:
 * *ctxsave+36 --> psplim
 * *ctxsave+32 --> r11
 * *ctxsave+28 --> r10
 * *ctxsave+24 --> r9
 * *ctxsave+20 --> r8
 * *ctxsave+16 --> r7
 * *ctxsave+12 --> r6
 * *ctxsave+8  --> r5
 * *ctxsave+4  --> r4
 * *ctxsave+0  --> psp
 *
 * NOTE: we could save the lr (EXC_RETURN) in ctxsave instead of the IRQ stack
 * to save one instruction (the stmdb sp!, {lr} in saveContext), but that would
 * increase every ctxsave array by 4 bytes and would actually be slower on MCUs
 * with external RAM, as in that case the IRQ stack is faster
 */

/**
 * \internal
 * \def saveContext()
 * Save context from an interrupt<br>
 * Must be the first line of an IRQ where a context switch can happen.
 * The IRQ must be "naked" to prevent the compiler from generating context save.
 * 
 * A note on the dmb instruction, without it a race condition was observed
 * between pauseKernel() and IRQrunScheduler(). pauseKernel() uses an strex
 * instruction to store a value in the global variable kernel_running which is
 * tested by the context switch code in IRQrunScheduler(). Without the memory
 * barrier IRQrunScheduler() would occasionally read the previous value and
 * perform a context switch while the kernel was paused, leading to deadlock.
 * The failure was only observed within the exception_test() in the testsuite
 * running on the stm32f429zi_stm32f4discovery.
 */
#define saveContext()                                                        \
    asm volatile("stmdb sp!, {lr}        \n\t" /*save lr on MAIN stack*/      \
                 "mrs   r1,  psp         \n\t" /*get PROCESS stack pointer*/  \
                 "ldr   r3,  =ctxsave    \n\t" /*get current context*/        \
                 "ldr   r0,  [r3]        \n\t"                                \
                 "stmia r0,  {r1,r4-r11} \n\t" /*save PROCESS sp + r4-r11*/   \
                 "mov   r4,  r3          \n\t" /*save for restoreContext*/    \
                 "dmb                    \n\t"                                \
                 );

/**
 * \def restoreContext()
 * Restore context in an IRQ where saveContext() is used. Must be the last line
 * of an IRQ where a context switch can happen. The IRQ must be "naked" to
 * prevent the compiler from generating context restore.
 */
#define restoreContext()                                                     \
    asm volatile("ldr   r0,  [r4]        \n\t" /*get current context*/        \
                 "ldmia r0!, {r1,r4-r11} \n\t" /*restore r4-r11 + r1=psp*/    \
                 "ldr   r0,  [r0]        \n\t" /*get psplim*/                 \
                 "msr   psplim, r0       \n\t" /*update PROCESS sp limit*/    \
                 "msr   psp, r1          \n\t" /*restore PROCESS sp*/         \
                 "ldmia sp!, {pc}        \n\t" /*return*/                     \
                 );

#endif //__FPU_PRESENT==1

namespace miosix {

inline void IRQinvokeScheduler() noexcept
{
    //NOTE: before Miosix 3 we used "svc 0" as yield also within the kernel, but
    //now we have the dedicated PendSV IRQ to call the scheduler, so use that.
    //Can't use NVIC_SetPendingIRQ as PendSV is an exception, not an IRQ
    SCB->ICSR=SCB_ICSR_PENDSVSET_Msk;
    //NOTE: due to the write buffer while doing the store to the SCB->ICSR,
    //the CPU could execute ahead of the yield. Use dsb to prevent
    //NOTE: a dmb is NOT enough, as the code pattern using this function can be:
    // str     r2, [r3, #4] //SCB->ICSR=SCB_ICSR_PENDSVSET_Msk;
    // dmb     sy
    // cpsie   i
    // cpsid   i
    // and it's important that the store to set PENDSVSET is seen in the cycle
    // when interrupts are enabled. However since cpsi* instruction do not read
    // from memory a dmb can still allow the store to be delayed. This issue
    // was first seen in CortexM33
    asm volatile("dsb":::"memory");
}

} //namespace miosix

/**
 * \}
 */
