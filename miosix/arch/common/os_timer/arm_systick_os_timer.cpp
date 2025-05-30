/***************************************************************************
 *   Copyright (C) 2025 by Terraneo Federico                               *
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

#include "interfaces/arch_registers.h"
#include "interfaces_private/os_timer.h"

/*
 * Miosix 3 can use the SysTick as a per-core preemption timer in the separate
 * timer model. However, we can't implement the entire OS timing subsystem using
 * SysTick only. Even in the separate timer model we need an additional timer
 * to handle high resolution timekeeping without clock skew and thread sleeps.
 */

#ifndef OS_TIMER_MODEL_UNIFIED

namespace miosix {

static CoarseTimeConversion preemptionTimeConversion;

void IRQinitCoreLocalPreemptionTimer()
{
    preemptionTimeConversion=CoarseTimeConversion(SystemCoreClock);
    NVIC_EnableIRQ(SysTick_IRQn);
}

void IRQosTimerSetPreemption(unsigned int ns) noexcept
{
    SysTick->LOAD=preemptionTimeConversion.ns2tick(ns);
    // SysTick is weird. Writing any value to VAL causes LOAD to be stored in
    // VAL. Couldn't it be implemented so that writing to VAL writes to VAL?
    SysTick->VAL=1;
    // Set TICKINT so that when timer reaches zero, interrupt is fired. Also
    // set the other bits to avoid read-modify-write.
    // NOTE: This register is also written in the ISR in cortexMx_interrupts.cpp
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk
                  | SysTick_CTRL_ENABLE_Msk;
    // Immediately after starting the timer, change LOAD so that when the timer
    // reaches zero, it restarts counting from the highest possible value to
    // measure thread actual burst length
    SysTick->LOAD=0x00ffffff;
}

// This may become part of the implementation of the to-be-defined API to get
// thread actual burst lengths for the control scheduler
// unsigned int getBurstErrorTicks()
// {
//     SysTick->CTRL=SysTick_CTRL_CLKSOURCE_Msk; //Stop timer
//     int result=SysTick->VAL;
//     // If timer overflowed, it kept counting so we can return negative value
//     if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) result-=0x01000000;
//     return result;
// }

} //namespace miosix

#endif //OS_TIMER_MODEL_UNIFIED
