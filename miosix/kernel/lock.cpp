/***************************************************************************
 *   Copyright (C) 2008-2024 by Terraneo Federico                          *
 *   Copyright (C) 2025 by Terraneo Federico and Daniele Cattaneo          *
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

#ifdef WITH_SMP
// Initialize as locked because the kernel starts with interrupts disabled
unsigned char GlobalIrqLock::holdingCore=0;
#endif

// Initialize as locked because the kernel starts with interrupts disabled
unsigned char FastPauseKernelLock::holdingCore=0;
bool FastPauseKernelLock::pendingWakeup=false;

void PauseKernelLock::lock()
{
    #ifdef WITH_SMP
    // Save and restore interrupt enabling state to allow use during boot
    bool ie=areInterruptsEnabled();
    fastDisableIrq();
    FastPauseKernelLock::irqDisabledFastLock();
    if(ie) fastEnableIrq();
    #else
    FastPauseKernelLock::lock();
    #endif
}

void PauseKernelLock::unlock()
{
    #ifdef WITH_SMP
    bool ie=areInterruptsEnabled();
    fastDisableIrq();
    FastPauseKernelLock::irqDisabledFastUnlock();
    if(ie) fastEnableIrq();
    
    // See comments in FastPauseKernelLock::unlock()
    if(FastPauseKernelLock::pendingWakeup)
    {
        FastPauseKernelLock::pendingWakeup=false;
        Thread::yield();
    }
    #else
    FastPauseKernelLock::unlock();
    #endif
}

bool PauseKernelLock::pushLock()
{
    // We should never take the PK lock while we're holding the GIL, as this
    // can cause deadlocks in SMP architectures. The only exception is early
    // at boot, before the kernel is started where there's only one core
    // running and it's running code as-if both the PK and GIL are taken.
    // If we're compiling with extra checks, verify this property using an
    // edge detection method. If the kernel isn't paused (this excludes the
    // early at boot case) and interrupts are disabled, then it's a bug.
    if(extraChecks==ExtraChecks::Kernel)
        if(isKernelPaused()==false && areInterruptsEnabled()==false)
            errorHandler(Error::UNEXPECTED);

    #ifdef WITH_SMP
    // Checking getCurrentCoreId() with interrupts potentially enabled is
    // unsafe due to thread migrations, so disable interrupts while we
    // check.
    bool ie=areInterruptsEnabled();
    fastDisableIrq();
    #endif
    bool res;
    if(FastPauseKernelLock::holdingCore==getCurrentCoreId())
    {
        // We are holding the lock, nothing to do
        res=true;
    } else {
        // Take the lock
        FastPauseKernelLock::irqDisabledFastLock();
        res=false;
    }
    #ifdef WITH_SMP
    if(ie) fastEnableIrq();
    #endif
    return res;
}

#ifdef WITH_DEEP_SLEEP
/// This variable is used to keep count of how many peripherals are actually used.
/// If it 0 then the system can enter the deep sleep state. Shared with thread.cpp
int deepSleepCounter=0;

void deepSleepLock() noexcept
{
    atomicAdd(&deepSleepCounter,1);
}

void deepSleepUnlock() noexcept
{
    atomicAdd(&deepSleepCounter,-1);
}
#endif //WITH_DEEP_SLEEP

} //namespace miosix
