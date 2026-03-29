/***************************************************************************
 *   Copyright (C) 2008-2026 by Terraneo Federico                          *
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

/**
 * \addtogroup Interfaces
 * \{
 */

/*
 * In this architecture, registers are saved in the following order:
 * *ctxsave+64 --> cpsr
 * *ctxsave+60 --> pc (return address)
 * *ctxsave+56 --> lr
 * *ctxsave+52 --> sp
 * *ctxsave+48 --> r12
 * *ctxsave+44 --> r11
 * *ctxsave+40 --> r10
 * *ctxsave+36 --> r9
 * *ctxsave+32 --> r8
 * *ctxsave+28 --> r7
 * *ctxsave+24 --> r6
 * *ctxsave+20 --> r5
 * *ctxsave+16 --> r4
 * *ctxsave+12 --> r3
 * *ctxsave+8  --> r2
 * *ctxsave+4  --> r1
 * *ctxsave+0  --> r0
 */

/**
 * \internal
 * \def saveContextFromSwi()
 * Save context from the software interrupt
 * It is used by the kernel, and should not be used by end users.
 */
#define saveContextFromSwi()                                                 \
    asm volatile(/*push lr on stack, to use it as a general purpose reg.*/   \
                 "stmfd  sp!,{lr}        \n\t"                               \
                 /*load ctxsave and dereference the pointer*/                \
                 "ldr    lr,=ctxsave     \n\t"                               \
                 "ldr    lr,[lr]         \n\t"                               \
                 /*save all thread registers except pc*/                     \
                 "stmia  lr,{r0-lr}^     \n\t"                               \
                 /*add a nop as required after stm ^ (ARM reference stm 2)*/ \
                 "nop                    \n\t"                               \
                 /*move lr to r0, restore original lr and save it*/          \
                 "add    r0,lr,#60       \n\t"                               \
                 "ldmfd  sp!,{lr}        \n\t"                               \
                 "stmia  r0!,{lr}        \n\t"                               \
                 /*save spsr on top of ctxsave*/                             \
                 "mrs    r1,spsr         \n\t"                               \
                 "stmia  r0,{r1}         \n\t");

/**
 * \def saveContextFromIrq()
 * Save context from the emulated PendSV
 * It is used by the kernel, and should not be used by end users.
 */
#define saveContextFromIrq()                                                 \
    asm volatile(/*Adjust lr, return address in a ISR has a 4 bytes offset*/ \
                 "sub    lr,lr,#4        \n\t");                             \
    saveContextFromSwi();

/**
 * \def restoreContext()
 * Restore context in an IRQ where saveContextFromIrq() or saveContextFromSwi()
 * is used. Must be the last line of an IRQ where a context switch can happen
 */
#define restoreContext()                                                     \
    asm volatile(/*load ctxsave and dereference the pointer*/                \
                 /*also add 64 to make it point to "top of stack"*/          \
                 "ldr   lr,=ctxsave     \n\t"                                \
                 "ldr   lr,[lr]         \n\t"                                \
                 "add   lr,lr,#64       \n\t"                                \
                 /*restore spsr*/                                            \
                 /*after this instructions, lr points to ctxsave[15]*/       \
                 "ldmda lr!,{r1}        \n\t"                                \
                 "msr   spsr,r1         \n\t"                                \
                 /*restore all thread registers except pc*/                  \
                 "ldmdb lr,{r0-lr}^     \n\t"                                \
                 /*add a nop as required after ldm ^ (ARM reference ldm 2)*/ \
                 "nop                   \n\t"                                \
                 /*lr points to return address, return from interrupt*/      \
                 "ldr   lr,[lr]         \n\t"                                \
                 "movs  pc,lr           \n\t");


namespace miosix {

/**
 * In the chip we support, interrupt number 31 isn't mapped to any hardware
 * peripheral, so we can reuse it as software interrupt for the emulated PendSV.
 * If in the future entry 31 will end up conflicting with some actual peripheral
 * interrupt of some other chip, we'll figure out how to deal with that.
 */
constexpr unsigned int emulatedPendsvNumber=31;

inline void IRQinvokeScheduler() noexcept
{
    //NOTE: before Miosix 3 we used "swi 0" as yield also within the kernel, but
    //now we a software IRQ to emulate the PendSV of newer ARM cores and call
    //the scheduler from there
    VICSoftInt=1<<emulatedPendsvNumber;
    asm volatile("":::"memory");
}

} //namespace miosix

/**
 * \}
 */
