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
volatile int kernelRunning=0;

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
    unsigned char state=globalIntrNestLockHoldingCore;
    if(state==getCurrentCoreId())
    {
        if(globalLockNesting==0xff) errorHandler(NESTING_OVERFLOW);
        globalLockNesting++;
    } else {
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
    fastDisableIrq();
    unsigned char state=globalPkNestLockHoldingCore;
    if(state==getCurrentCoreId())
    {
        if(kernelRunning==0xff) errorHandler(NESTING_OVERFLOW);
        kernelRunning++;
    } else {
        //BUG: here we may wait for a long time and with interrupts disabled!!
        IRQhwSpinlockAcquire(RP2040HwSpinlocks::PK); //TODO: need generic API
        globalPkNestLockHoldingCore=getCurrentCoreId();
        if(kernelRunning!=0) errorHandler(PAUSE_KERNEL_NESTING);
        kernelRunning=1;
    }
    fastEnableIrq();
    #else //WITH_SMP
    int old=atomicAddExchange(&kernelRunning,1);
    if(old>=0xff) errorHandler(NESTING_OVERFLOW);
    #endif //WITH_SMP
}

void restartKernel() noexcept
{
    #ifdef WITH_SMP
    fastDisableIrq();
    int old=kernelRunning--;
    if(kernelRunning==0)
    {
        globalPkNestLockHoldingCore=0xff;
        IRQhwSpinlockRelease(RP2040HwSpinlocks::PK); //TODO: need generic API
    }
    fastEnableIrq();
    #else //WITH_SMP
    int old=atomicAddExchange(&kernelRunning,-1);
    #endif //WITH_SMP
    if(old<=0) errorHandler(PAUSE_KERNEL_NESTING);
    
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
    return (kernelRunning==0) && kernelStarted;
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
