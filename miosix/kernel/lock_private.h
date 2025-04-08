/***************************************************************************
 *   Copyright (C) 2008-2025 by Terraneo Federico                          *
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

#ifndef COMPILING_MIOSIX
#error "This is header is private, it can't be used outside Miosix itself."
#error "If your code depends on a private header, it IS broken."
#endif //COMPILING_MIOSIX

#include "config/miosix_settings.h"
#include "interfaces/cpu_const.h"
#include "interfaces/atomic_ops.h"
#include "interfaces_private/smp.h"
#include "lock.h"
#include "error.h"

namespace miosix {

extern int pauseKernelNesting;
#ifdef WITH_SMP
extern unsigned char globalPkNestLockHoldingCore;
#endif //WITH_SMP

extern unsigned char globalLockNesting;
#ifdef WITH_SMP
extern unsigned char globalIntrNestLockHoldingCore;
#endif //WITH_SMP

extern volatile bool pendingWakeup;

/**
 * \internal Lock the Global Irq Lock to a specified recursion depth.
 * Must be called with the lock not yet taken by the current thread.
 * 
 * \param savedNesting The lock recursion depth to set.
 */
inline void globalIrqForceLockToDepth(unsigned char savedNesting)
{
    fastGlobalIrqLock();
    #ifdef WITH_SMP
    globalIntrNestLockHoldingCore=getCurrentCoreId();
    #endif
    if(globalLockNesting!=0) errorHandler(GLOBAL_LOCK_NESTING);
    globalLockNesting=savedNesting;
}

/**
 * \internal Fully unlock the Global Irq Lock and return the previous recursion
 * level of the lock.
 * Must be called with the lock currently being taken by the current thread,
 * either through the fast primitives or not.
 */
inline unsigned char globalIrqForceUnlock()
{
    unsigned char savedNesting=globalLockNesting;
    globalLockNesting=0;
    #ifdef WITH_SMP
    globalIntrNestLockHoldingCore=0xff;
    #endif
    fastGlobalIrqUnlock();
    return savedNesting;
}

/**
 * \internal Lock the pause kernel lock to the specified nesting level.
 * Must be called with the pause kernel lock not taken by the current thread,
 * and local interrupts disabled.
 * The GIL spinlock must *not* be taken, this is why this function
 * does not have the IRQ prefix even though it appears it should.
 * 
 * \param The lock recursion depth to set.
 */
inline void irqDisabledPauseKernelForceToDepth(unsigned char savedNesting)
{
    #ifdef WITH_SMP
    // An important difference from globalIrqLock/Unlock is that instead of
    // disabling interrupts for ensuring no preemption occurs, we are
    // setting globalPkNestLockHoldingCore.
    //   This MUST HAPPEN ATOMICALLY from the point of view of the scheduler
    // though, so we must use atomics or the GIL to do it.
    //   Of course we can't just disable interrupts locally because another
    // core might be trying to acquire the pause kernel lock concurrently.
    //   We cannot even use atomics unfortunately. I.e. if we did a
    // atomicExchange(&globalPkNestLockHoldingCore,getCurrentCoreId())
    // the scheduler might still move us from one core to another between
    // the call to getCurrentCoreId() and the (atomic) setting of the
    // variable.
    irqDisabledHwLockAcquire(HwLocks::PK);
    globalPkNestLockHoldingCore=getCurrentCoreId();
    #endif
    // NOTE: in the single core case we could do a compare and swap but since
    // we already disabled interrupts it's not necessary
    if(pauseKernelNesting!=0) errorHandler(UNEXPECTED);
    pauseKernelNesting=savedNesting;
}

/**
 * \internal Lock the pause kernel lock to the specified nesting level.
 * Must be called with the pause kernel lock not taken by the current thread.
 * 
 * \param The lock recursion depth to set.
 */
inline void pauseKernelForceToDepth(unsigned char savedNesting)
{
    #ifdef WITH_SMP
    fastDisableIrq();
    irqDisabledPauseKernelForceToDepth(savedNesting);
    fastEnableIrq();
    #else
    int old=atomicCompareAndSwap(&pauseKernelNesting,0,savedNesting);
    if(old!=0) errorHandler(UNEXPECTED);
    #endif
}

/**
 * \internal Fully unlock the pause kernel lock and return the previous 
 * recursion level of the lock.
 * Must be called with the pause kernel lock currently being taken by the
 * current thread.
 */
inline int irqDisabledRestartKernelForce()
{
    #ifdef WITH_SMP
    auto savedPauseKernelNesting=pauseKernelNesting;
    pauseKernelNesting=0;
    globalPkNestLockHoldingCore=0xff;
    irqDisabledHwLockRelease(HwLocks::PK);
    return savedPauseKernelNesting;
    #else
    auto savedPauseKernelNesting=pauseKernelNesting;
    pauseKernelNesting=0;
    #endif
    if(savedPauseKernelNesting==0) errorHandler(PAUSE_KERNEL_NESTING);
    return savedPauseKernelNesting;
}

} //namespace miosix
