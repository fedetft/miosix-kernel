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

#include "interfaces_private/cpu.h"
#include "interfaces_private/userspace.h"
#include "interfaces/arch_registers.h"

namespace miosix {

void initKernelThreadCtxsave(unsigned int *ctxsave, void (*pc)(void *(*)(void*),void*),
                             unsigned int *sp, unsigned int *spLimit,
                             void *(*arg0)(void*), void *arg1) noexcept
{
    unsigned int *stackPtr=sp;
    //Stack is full descending, so decrement first
    stackPtr--; *stackPtr=0x01000000;                                 //--> xPSR
    stackPtr--; *stackPtr=reinterpret_cast<unsigned int>(pc);         //--> pc
    stackPtr--; *stackPtr=0xffffffff;                                 //--> lr
    stackPtr--; *stackPtr=0;                                          //--> r12
    stackPtr--; *stackPtr=0;                                          //--> r3
    stackPtr--; *stackPtr=0;                                          //--> r2
    stackPtr--; *stackPtr=reinterpret_cast<unsigned int>(arg1);       //--> r1
    stackPtr--; *stackPtr=reinterpret_cast<unsigned int>(arg0);       //--> r0

    ctxsave[0]=reinterpret_cast<unsigned int>(stackPtr);              //--> psp
    //leaving the content of r4-r11 uninitialized
    #if __FPU_PRESENT==1
    //NOTE: only armv7m with fpu has lr in ctxsave
    ctxsave[9]=0xfffffffd; //EXC_RETURN=thread mode, use psp, no floating ops
    //leaving the content of s16-s31 uninitialized
    #endif //__FPU_PRESENT==1

    // ARMv8-M stack pointer limit register
    #if __FPU_PRESENT==1
    ctxsave[26]=reinterpret_cast<unsigned int>(spLimit);
    #else  //__FPU_PRESENT==1
    ctxsave[9]=reinterpret_cast<unsigned int>(spLimit);
    #endif //__FPU_PRESENT==1
}

void IRQportableStartKernel() noexcept
{
    //Enable fault handlers
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk
            | SCB_SHCSR_MEMFAULTENA_Msk;
    //Enable traps for division by zero. Trap for unaligned memory access
    //was removed as gcc starting from 4.7.2 generates unaligned accesses by
    //default (https://www.gnu.org/software/gcc/gcc-4.7/changes.html)
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;

    //create a temporary space to save current registers. This data is useless
    //since there's no way to stop the sheduler, but we need to save it anyway.
    unsigned int s_ctxsave[miosix::CTXSAVE_SIZE];
    ctxsave[getCurrentCoreId()]=s_ctxsave;//make global ctxsave point to it
    IRQinvokeScheduler();
    __enable_fault_irq();
    __enable_irq();
    //Never reaches here
}

} //namespace miosix
