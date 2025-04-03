/***************************************************************************
 *   Copyright (C) 2025 by Federico Terraneo and Daniele Cattaneo          *
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

#include "interfaces-impl/lock_impl.h"
#include "interfaces/arch_registers.h"

namespace miosix {

static inline void IRQhwSpinlockAcquire(unsigned char i) noexcept
{
    for(;;)
    {
        if(sio_hw->spinlock[i]) break;
        __WFE();
    }
    __DSB();
}

static inline void IRQhwSpinlockRelease(unsigned char i) noexcept
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

// Definition of statically allocated spinlocks
struct RP2040HwSpinlocks
{
    enum
    {
        GIL = 0,        // Global interrupt lock
        PK,             // Pause kernel lock
        Atomics,        // Mutual exclusion of atomic operations
        InitCoreSync,   // Used at the end of SMP setup to synchronize the cores
        FirstFree,
        Last = 32
    };
};

template <unsigned char Id>
class FastHwSpinLock
{
public:
    FastHwSpinLock()
    {
        prevState=areInterruptsEnabled();
        fastDisableIrq();
        IRQhwSpinlockAcquire(Id);
    }

    ~FastHwSpinLock()
    {
        IRQhwSpinlockRelease(Id);
        if(prevState) fastEnableIrq();
    }

    FastHwSpinLock(const FastHwSpinLock&)=delete;
    FastHwSpinLock& operator= (const FastHwSpinLock&)=delete;

private:
    bool prevState;
};

} //namespace miosix
