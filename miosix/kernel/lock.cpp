/***************************************************************************
 *   Copyright (C) 2008-2025 by Terraneo Federico                          *
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

#include "lock.h"
#include "error.h"
#include "thread.h"
#include "interfaces/atomic_ops.h"

namespace miosix {

///\internal !=0 after pauseKernel(), ==0 after restartKernel()
volatile int pauseKernelNesting=0;

/// This is used by globalIrqLock() and globalIrqUnlock() to allow nested
/// calls to these functions.
unsigned char globalLockNesting=0;

#ifdef WITH_SMP
unsigned char globalIntrNestLockHoldingCore=0xff;
unsigned char globalPkNestLockHoldingCore=0xff;
#endif //WITH_SMP

///\internal true if a thread wakeup occurs while the kernel is paused
volatile bool pendingWakeup=false;

#ifdef WITH_DEEP_SLEEP
///  This variable is used to keep count of how many peripherals are actually used.
/// If it 0 then the system can enter the deep sleep state
static int deepSleepCounter=0;
#endif //WITH_DEEP_SLEEP

//Variables shared with thread.cpp for performance and encapsulation reasons
extern bool kernelStarted;

void globalIrqLock() noexcept
{
    #ifdef WITH_SMP
    // Is the current core the one holding the global lock?
    if(globalIntrNestLockHoldingCore==getCurrentCoreId())
    {
        // Yes. If we arrive here we are sure we already have interrupts
        // disabled and the GIL taken, just increase nesting level.
        // We cannot be rescheduled to another core because interrupts are
        // disabled.
        if(globalLockNesting==0xff) errorHandler(NESTING_OVERFLOW);
        globalLockNesting++;
    } else {
        // No we are not holding the GIL. Try to take it then!
        // Note that even if we are rescheduled to a different core right here
        // the condition state==getCurrentCoreId() will continue to be false,
        // and we correctly deduce we don't have the GIL (which is what
        // matters).
        // The following are the important cases:
        // - Another thread has the GIL and won't release it before we attempt
        //   to take it
        //   => That thread won't change core because it has interrupts
        //      disabled, so we cannot be scheduled to that core, but that's
        //      the only way to make the condition true
        // - Another thread held the GIL but has just released it now after the
        //   check
        //   => When the other thread has released the GIL, they always set
        //      globalIntrNestLockHoldingCore global to 0xFF, which ensures the
        //      condition of the `if' is false for all cores.
        // - Another thread is holding the GIL in a non-recursive way
        //   => Same case as above but because globalIntrNestLockHoldingCore is
        //      0xFF even with the lock taken
        // - Another thread has taken the GIL since the check
        //   => If the GIL is taken by another thread, that thread MUST be
        //      running. But if we are running simultaneously to that other
        //      thread, we must be on a different core, and the condition is
        //      still false.
        fastGlobalIrqLock();
        globalIntrNestLockHoldingCore=getCurrentCoreId();
        if(globalLockNesting!=0) errorHandler(GLOBAL_LOCK_NESTING);
        globalLockNesting=1;
    }
    #else //WITH_SMP
    fastGlobalIrqLock();
    if(globalLockNesting==0xff) errorHandler(NESTING_OVERFLOW);
    globalLockNesting++;
    #endif //WITH_SMP
}

void globalIrqUnlock() noexcept
{
    if(globalLockNesting==0)
    {
        //Bad, globalIrqUnlock was called one time more than globalIrqLock
        errorHandler(GLOBAL_LOCK_NESTING);
    }
    globalLockNesting--;
    if(globalLockNesting==0)
    {
        #ifdef WITH_SMP
        globalIntrNestLockHoldingCore=0xff;
        #endif //WITH_SMP
        // This function must be safe to call even at the early boot stage
        // before the kernel is fully initialized. Thus, code will take the
        // lock and release it, but we do not want to enable interrupts
        if(kernelStarted==true) fastGlobalIrqUnlock();
        else {
            //We must not enable interrupts since we're in the boot stage where
            //interrupts should be disabled, but we need to release the spinlock
            //and this can be done with the irq-context unlock call
            fastGlobalUnlockFromIrq();
        }
    }
}

void pauseKernel() noexcept
{
    #ifdef WITH_SMP
    if(!kernelStarted)
    {
        // If we are here and the kernel is not yet started, then we are
        // presumably in IRQbspInit() or in the malloc that allocates heap
        // space for the first thread, before SMP is even set up.
        //   We cannot enable interrupts here, but we are also sure that we
        // are executing on core 0 with interrupts disabled. So we really
        // don't need to do anything. We could exit here but for the sake of
        // robustness let's update the lock state anyway.
        //   This could be implemented by using globalIrqLock/Unlock elsewhere
        // but that completely tanks performance and makes pauseKernel slower
        // than a non-recursive conventional mutex.
        if(globalPkNestLockHoldingCore!=0)
        {
            globalPkNestLockHoldingCore=0;
            pauseKernelNesting=1;
        } else {
            // If the kernel is not yet fully booted, the serial driver may not
            // be completely initialized yet. Due to this, errorHandler() would
            // fail causing a null pointer dereference and thus a HardFault,
            // which in itself would try to print to the serial again...
            // In other words calling errorHandler() here makes no sense.
            // Should we even bother checking pauseKernelNesting then?
            //if(pauseKernelNesting==0xff) errorHandler(NESTING_OVERFLOW);
            pauseKernelNesting++;
        }
        return;
    }
    // The logic mirrors the one in globalIrqLock/Unlock, see the comments
    // there for an explanation.
    if(globalPkNestLockHoldingCore==getCurrentCoreId())
    {
        if(pauseKernelNesting==0xff) errorHandler(NESTING_OVERFLOW);
        pauseKernelNesting++;
    } else {
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
        FastGlobalIrqLock lock;
        for(;;)
        {
            if(globalPkNestLockHoldingCore!=0xff)
            {
                FastGlobalIrqUnlock unlock(lock);
                __WFE(); // TODO: arch-specific
                continue;
            }
            // If we get here, we have decided we can take the lock.
            globalPkNestLockHoldingCore=getCurrentCoreId();
            if(pauseKernelNesting!=0) errorHandler(PAUSE_KERNEL_NESTING);
            pauseKernelNesting=1;
            break;
        }
    }
    #else //WITH_SMP
    int old=atomicAddExchange(&kernelRunning,1);
    if(old>=0xff) errorHandler(NESTING_OVERFLOW);
    #endif //WITH_SMP
}

void restartKernel() noexcept
{
    #ifdef WITH_SMP
    int old=pauseKernelNesting--;
    if(pauseKernelNesting==0)
    {
        globalPkNestLockHoldingCore=0xff;
        __DSB(); //TODO: arch-specific, need portable wrapper
        __SEV();
    }
    #else //WITH_SMP
    int old=atomicAddExchange(&pauseKernelNesting,-1);
    #endif //WITH_SMP
    if(old<=0) errorHandler(PAUSE_KERNEL_NESTING);
    
    //TODO is it still needed?
    //Check globalLockNesting to allow pauseKernel() while interrupts
    //are disabled with a GlobalIrqLock
    if(globalLockNesting==0)
    {
        //If we missed a preemption yield immediately. This mechanism works the
        //same way as the hardware implementation of interrupts that remain
        //pending if they occur while interrupts are disabled.
        //This is important to make sure context switches to a higher priority
        //thread happen in a timely fashion.
        //It is important that pendingWakeup is set to true any time the
        //scheduler is called but it could not run due to the kernel being
        //paused regardless of whether the scheduler has been called by the
        //timer irq or any peripheral irq.
        //With the tickless kernel, this is also important to prevent deadlocks
        //as the idle thread is no longer periodically interrupted by timer
        //ticks and it does pause the kernel. If the interrupt that wakes up
        //a thread fails to call the scheduler since the idle thread paused the
        //kernel and pendingWakeup is not set, this could cause a deadlock.
        if(old==1 && pendingWakeup)
        { 
            pendingWakeup=false;
            Thread::yield();
        }
    }
}

bool isKernelRunning() noexcept
{
    return (pauseKernelNesting==0) && kernelStarted;
}

void deepSleepLock() noexcept
{
    #ifdef WITH_DEEP_SLEEP
    atomicAdd(&deepSleepCounter,1);
    #endif //WITH_DEEP_SLEEP
}

void deepSleepUnlock() noexcept
{
    #ifdef WITH_DEEP_SLEEP
    atomicAdd(&deepSleepCounter,-1);
    #endif //WITH_DEEP_SLEEP
}

} //namespace miosix
