/***************************************************************************
 *   Copyright (C) 2025 by Daniele Cattaneo                                *
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

#include "config/miosix_settings.h"

#ifdef WITH_SMP

#include "interfaces/interrupts.h"

namespace miosix {

namespace HwLocks
{
    enum
    {
        RP2040Atomics = KernelMax, // Mutual exclusion of atomic operations
        RP2040InitCoreSync         // Used at the end of SMP setup to synchronize the cores
    };
};

inline void irqDisabledHwIrqLockAcquire(HwLocks::ID i) noexcept
{
    for(;;)
    {
        if(sio_hw->spinlock[i]) break;
        __WFE();
    }
    __DSB();
}

inline void irqDisabledHwIrqLockRelease(HwLocks::ID i) noexcept
{
    hwLockRelease(i);
}

inline void irqDisabledHwLockAcquire(HwLocks::ID i) noexcept
{
    for(;;)
    {
        // If we fail taking the spinlock, and then a preemption happens before
        // the WFE, we might not wake immediately on spinlock release if
        // some other other code elsewhere absorbed the SEV.
        // This corner case looks like this on a timeline:
        //      core 0                             core 1
        // t=0: sio_hw->spinlock[i]==0
        // t=1: context switch to other thread
        // t=2:                                    SEV in hwSpinlockRelease
        // t=3: other thread does a WFE, wakes up
        // t=4: context switch to original thread
        // t=5: WFE in hwSpinlockAcquire should
        //      wake up immediately but doesn't
        // As a workaround we disable interrupts locally (preventing preemption)
        // between the access to the spinlock and the WFE.
        //   Normally, however, this would prevent servicing interrupts
        // while the WFE is waiting. Fortunately ARM provides the SEVONPEND
        // flag, which makes the SCB generate a SEV if interrupts become
        // pending. With this flag set, WFE terminates on interrupts even with
        // interrupts disabled, which allows us to re-enable interrupts
        // temporarily and service them.
        fastDisableIrq();
        if(sio_hw->spinlock[i]) break;
        // This WFE executes with interrupts disabled but will wake up on any
        // pending IRQ because the SEVONPEND SCB flag has been set during boot.
        __WFE();
        // Service pending interrupts (if any) by allowing them briefly.
        fastEnableIrq();
        fastDisableIrq();
    }
    __DSB();
}

inline void irqDisabledHwLockRelease(HwLocks::ID i) noexcept
{
    hwLockRelease(i);
}

inline void hwLockAcquire(HwLocks::ID i) noexcept
{
    fastDisableIrq();
    irqDisabledHwLockAcquire(i);
    fastEnableIrq();
}

// This code works even with interrupts disabled
inline void hwLockRelease(HwLocks::ID i) noexcept
{
    sio_hw->spinlock[i]=1;
    __DSB();
    __SEV();
    // Put a compiler memory barrier after SEV for good measure
    asm volatile("":::"memory");
    // A word of warning. GCC 9.2.0 has been observed reordering
    //   __DSB();
    //   sio_hw->spinlock[i]=1;
    //   __SEV();
    //   fastEnableIrq();
    // into
    //   __SEV();
    //   fastEnableIrq();
    //   sio_hw->spinlock[i]=1;
    //   __DSB();
    // which is weird because both __DSB() and fastEnableIrq() clobber memory
    // and thus should prevent reordering.
}

} // namespace miosix

#endif // WITH_SMP
