/***************************************************************************
 *   Copyright (C) 2010 - 2024 by Terraneo Federico                        *
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

/*
 * This pointer is used by the kernel, and should not be used by end users.
 * this is a pointer to a location where to store the thread's registers during
 * context switch. It requires C linkage to be used inside asm statement.
 * Registers are saved in the following order:
 * *ctxsave+32 --> r11
 * *ctxsave+28 --> r10
 * *ctxsave+24 --> r9
 * *ctxsave+20 --> r8
 * *ctxsave+16 --> r7
 * *ctxsave+12 --> r6
 * *ctxsave+8  --> r5
 * *ctxsave+4  --> r4
 * *ctxsave+0  --> psp
 */
extern "C" {
extern volatile unsigned int *ctxsave;
}
const int stackPtrOffsetInCtxsave=0; ///< Allows to locate the stack pointer

inline void doYield()
{
    SCB->ICSR=SCB_ICSR_PENDSVSET_Msk;
    asm volatile("dmb":::"memory");
    //NVIC_SetPendingIRQ(PendSV_IRQn);
//     asm volatile("movs r3, #0\n\t"
//                  "svc  0"
//                  :::"r3");
}

inline void doDisableInterrupts()
{
    // Documentation says __disable_irq() disables all interrupts with
    // configurable priority, so also SysTick and SVC.
    // No need to disable faults with __disable_fault_irq()
    __disable_irq();
    //The new fastDisableInterrupts/fastEnableInterrupts are inline, so there's
    //the need for a memory barrier to avoid aggressive reordering
    asm volatile("":::"memory");
}

inline void doEnableInterrupts()
{
    __enable_irq();
    //The new fastDisableInterrupts/fastEnableInterrupts are inline, so there's
    //the need for a memory barrier to avoid aggressive reordering
    asm volatile("":::"memory");
}

inline bool checkAreInterruptsEnabled()
{
    register int i;
    asm volatile("mrs   %0, primask    \n\t":"=r"(i));
    if(i!=0) return false;
    return true;
}

namespace fault {
/**
 * Possible kind of faults that the Cortex-M3 can report.
 * They are used to print debug information if a process causes a fault.
 * This is a regular enum enclosed in a namespace instead of an enum class
 * as due to the need to loosely couple fault types for different architectures
 * the arch-independent code uses int to store generic fault types.
 */
enum FaultType
{
    MP=1,            //Process attempted data access outside its memory
    MP_NOADDR=2,     //Process attempted data access outside its memory (missing addr)
    MP_XN=3,         //Process attempted code access outside its memory
    UF_DIVZERO=4,    //Process attempted to divide by zero
    UF_UNALIGNED=5,  //Process attempted unaligned memory access
    UF_COPROC=6,     //Process attempted a coprocessor access
    UF_EXCRET=7,     //Process attempted an exception return
    UF_EPSR=8,       //Process attempted to access the EPSR
    UF_UNDEF=9,      //Process attempted to execute an invalid instruction
    UF_UNEXP=10,     //Unexpected usage fault
    HARDFAULT=11,    //Hardfault (for example process executed a BKPT instruction)
    BF=12,           //Busfault
    BF_NOADDR=13,    //Busfault (missing addr)
    STACKOVERFLOW=14 //Stack overflow
};

} //namespace fault
