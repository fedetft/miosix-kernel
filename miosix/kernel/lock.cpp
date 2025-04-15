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
#include "lock_private.h"
#include "error.h"
#include "thread.h"
#include "interfaces/atomic_ops.h"

namespace miosix {

unsigned char GlobalIrqLock::holdingCore=0xff;

///\internal !=0 after pauseKernel(), ==0 after restartKernel()
int pauseKernelNesting=0;
#ifdef WITH_SMP
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

// TODO: rename to preemptionLock/preemptionUnlock? and change PK prefix to PL,
// globalPlNestLockHoldingCore, preemptionLockNesting
void pauseKernel() noexcept
{
    #ifdef WITH_SMP
    // NOTE: the code below would disable and then enable interrupts, so isn't
    // safe to be called before the kernel is started. However, ther's no need
    // for locking before the kernel is started, so do nothing
    if(kernelStarted==false) return;
    if(globalPkNestLockHoldingCore==getCurrentCoreId())
    {
        if(pauseKernelNesting==0xff) errorHandler(NESTING_OVERFLOW);
        pauseKernelNesting++;
    } else {
        pauseKernelForceToDepth(1);
    }
    #else //WITH_SMP
    int old=atomicAddExchange(&pauseKernelNesting,1);
    if(old>=0xff) errorHandler(NESTING_OVERFLOW);
    #endif //WITH_SMP
}

void restartKernel() noexcept
{
    #ifdef WITH_SMP
    // NOTE: the code below would disable and then enable interrupts, so isn't
    // safe to be called before the kernel is started. However, ther's no need
    // for locking before the kernel is started, so do nothing
    if(kernelStarted==false) return;
    int old=pauseKernelNesting--;
    if(pauseKernelNesting==0)
    {
        fastDisableIrq();
        globalPkNestLockHoldingCore=0xff;
        // We disable local interrupts because if the scheduler decides to
        // reschedule us here, any other thread that tries to take the PK lock
        // will have to wait until we are rescheduled to release the spinlock,
        // and this is clearly undesirable.
        irqDisabledHwLockRelease(HwLocks::PK);
        fastEnableIrq();
    }
    #else //WITH_SMP
    int old=atomicAddExchange(&pauseKernelNesting,-1);
    #endif //WITH_SMP
    if(old<=0) errorHandler(PAUSE_KERNEL_NESTING);

    // If we missed a preemption yield immediately. This mechanism works the
    // same way as the hardware implementation of interrupts that remain
    // pending if they occur while interrupts are disabled.
    // This is important to make sure context switches to a higher priority
    // thread happen in a timely fashion.
    // It is important that pendingWakeup is set to true any time the
    // scheduler is called but it could not run due to the kernel being
    // paused regardless of whether the scheduler has been called by the
    // timer irq or any peripheral irq.
    // With the tickless kernel, this is also important to prevent deadlocks
    // as the idle thread is no longer periodically interrupted by timer
    // ticks and it does pause the kernel. If the interrupt that wakes up
    // a thread fails to call the scheduler since the idle thread paused the
    // kernel and pendingWakeup is not set, this could cause a deadlock.
    // This code cannot trigger a yield before the kernel is started since only
    // the scheduler sets pendingWakeup
    if(old==1 && pendingWakeup)
    { 
        pendingWakeup=false;
        Thread::yield();
    }
}

bool isKernelPaused() noexcept
{
    #ifdef WITH_SMP
    // In a multi core environment pauseKernel disables preemption only on
    // the core that paused the kernel, all other cores can preempt just
    // fine. Of course, only one core at a time can call pauseKernel, if
    // another core attempts it, then it just waits to protect shared
    // variables modified within a pause kernel lock.
    return globalPkNestLockHoldingCore==getCurrentCoreId();
    #else //WITH_SMP
    return pauseKernelNesting!=0;
    #endif //WITH_SMP
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
