/***************************************************************************
 *   Copyright (C) 2013-2024 by Terraneo Federico                          *
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

#include "miosix_settings.h"

#ifdef WITH_SMP

#include "kernel/lock.h"

namespace miosix {

class AtomicsLock
{
public:
    AtomicsLock()
    {
        prevState=areInterruptsEnabled();
        fastDisableIrq();
        irqDisabledHwIrqLockAcquire(HwLocks::RP2040Atomics);
    }

    ~AtomicsLock()
    {
        irqDisabledHwIrqLockRelease(HwLocks::RP2040Atomics);
        if(prevState) fastEnableIrq();
    }

    AtomicsLock(const AtomicsLock&)=delete;
    AtomicsLock& operator= (const AtomicsLock&)=delete;

private:
    bool prevState;
};

int atomicSwapImpl(volatile int *p, int v)
{
    int result;
    {
        AtomicsLock lock;
        result = *p;
        *p = v;
    }
    asm volatile("":::"memory");
    return result;
}

void atomicAddImpl(volatile int *p, int incr)
{
    {
        AtomicsLock lock;
        *p += incr;
    }
    asm volatile("":::"memory");
}

int atomicAddExchangeImpl(volatile int *p, int incr)
{
    int result;
    {
        AtomicsLock lock;
        result = *p;
        *p += incr;
    }
    asm volatile("":::"memory");
    return result;
}

int atomicCompareAndSwapImpl(volatile int *p, int prev, int next)
{
    int result;
    {
        AtomicsLock lock;
        result = *p;
        if(*p == prev) *p = next;
    }
    asm volatile("":::"memory");
    return result;
}

void *atomicFetchAndIncrementImpl(void * const volatile * p, int offset, int incr)
{
    void *result;
    {
        AtomicsLock lock;
        result = *p;
        if(result == 0) return 0;
        auto pt = reinterpret_cast<volatile unsigned int*>(result) + offset;
        *pt += incr;
    }
    asm volatile("":::"memory");
    return result;
}

} //namespace miosix

#endif //WITH_SMP
