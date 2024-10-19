/***************************************************************************
 *   Copyright (C) 2010-2021 by Terraneo Federico                          *
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
#include "kernel/kernel.h"
#include "kernel/error.h"
#include "interfaces_private/bsp.h"
#include "kernel/scheduler/scheduler.h"
#include <algorithm>

namespace miosix {

void IRQsystemReboot()
{
    NVIC_SystemReset();
}

void initCtxsave(unsigned int *ctxsave, unsigned int *sp,
                 void (*pc)(void *(*)(void*),void*),
                 void *(*arg0)(void*), void *arg1)
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
}

void IRQportableStartKernel()
{
    // FIXME: on M0 we cannot disable interrupt preemption: all IRQs must
    // have the same priority, otherwise everything breaks!
    //   As a convention we choose a priority of 3, which is the *lowest* one.
    // This counterintuitive setting allows for higher-priority handlers as long
    // as they do NOT perform context switches and are fully reentrant.
    //   However such an interrupt handler is in practice impossible for Miosix,
    // as at the moment IRQ contexts are assumed to be in mutual exclusion.
    //   This will be fixable once we funnel all context switches to PendSV and
    // after we introduce SMP support.
    NVIC_SetPriority(SVCall_IRQn,3); // highest priority=0, lowest=3

    //create a temporary space to save current registers. This data is useless
    //since there's no way to stop the sheduler, but we need to save it anyway.
    unsigned int s_ctxsave[miosix::CTXSAVE_SIZE];
    ctxsave=s_ctxsave;//make global ctxsave point to it
    //Note, we can't use enableInterrupts() now since the call is not matched
    //by a call to disableInterrupts()
    //FIXME: there's no __enable_fault_irq(); is this right?
    __enable_irq();
    miosix::Thread::yield();
    //Never reaches here
}

} //namespace miosix
