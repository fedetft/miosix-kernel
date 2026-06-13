/***************************************************************************
 *   CK803S (C-SKY V2) atomic ops for modern Miosix.                        *
 *   CK803S exposes no LL/SC here, so atomics disable interrupts briefly    *
 *   (same approach as the armv6m port). GPL v2+ + Miosix linking exception.*
 ***************************************************************************/

#pragma once

#include "interfaces/arch_registers.h"
#include "interfaces/interrupts.h"

namespace miosix {

class AtomicsLock
{
public:
    AtomicsLock()
    {
        oldInterruptsEnabled=areInterruptsEnabled();
        fastDisableIrq();
    }

    ~AtomicsLock()
    {
        if(oldInterruptsEnabled) fastEnableIrq();
    }

private:
    bool oldInterruptsEnabled;

    AtomicsLock(const AtomicsLock&);
    AtomicsLock& operator= (const AtomicsLock&);
};

inline int atomicSwap(volatile int *p, int v)
{
    int result;
    { AtomicsLock lock; result = *p; *p = v; }
    asm volatile("":::"memory");
    return result;
}

inline void atomicAdd(volatile int *p, int incr)
{
    { AtomicsLock lock; *p += incr; }
    asm volatile("":::"memory");
}

inline int atomicAddExchange(volatile int *p, int incr)
{
    int result;
    { AtomicsLock lock; result = *p; *p += incr; }
    asm volatile("":::"memory");
    return result;
}

inline int atomicCompareAndSwap(volatile int *p, int prev, int next)
{
    int result;
    { AtomicsLock lock; result = *p; if(*p == prev) *p = next; }
    asm volatile("":::"memory");
    return result;
}

inline void *atomicFetchAndIncrement(void * const volatile * p, int offset,
        int incr)
{
    void *result;
    {
        AtomicsLock lock;
        result = const_cast<void*>(*p);
        if(result == 0) return 0;
        volatile uint32_t *pt = reinterpret_cast<uint32_t*>(result) + offset;
        *pt += incr;
    }
    asm volatile("":::"memory");
    return result;
}

} //namespace miosix
