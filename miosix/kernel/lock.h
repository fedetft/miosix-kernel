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

#pragma once

#include "miosix_settings.h"
#include "interfaces/interrupts.h"
#include "interfaces/cpu_const.h"
#include "interfaces_private/smp_locks.h"
#include "thread.h"
#include "error.h"

namespace miosix {

//Forward declarations
inline bool isKernelPaused() noexcept;

/**
 * \defgroup lock Low-level locking
 * \brief Global Irq Lock, Pause Kernel lock
 * 
 * These classes define two global locks used for synchronization in the Miosix
 * kernel, the Global Irq Lock (GIL) and the Pause Kernel lock.
 * 
 * The GIL is used to ensure mutual exclusion between threads and interrupt
 * handlers. It is also used by the kernel to protect its data structures.
 * In single core platforms it is implemented by disabling and enabling
 * interrupts, while in multicore (SMP) platforms it is implemented as a
 * hardware spinlock or sleep lock.
 * Taking this core also has the effect of disabling preemption, therefore
 * you should try to keep the critical sections that hold this lock as short as
 * possible.
 * Holding this lock grants the capability to call kernel functions whose name
 * starts with the IRQ prefix.
 * 
 * The Pause Kernel lock is a global lock in the sense that only one thread on
 * one core can take the lock.
 * On the core which takes the lock preemption is disabled, but interrupts will
 * continue to occur. Attempting to take the pauseKernel lock from another core
 * will block until the lock is released.
 * Calling blocking functions (printf/fopen/...) including device drivers
 * implemented in terms of IRQglobalIrqUnlockAndWait() or sleeping while holding
 * this lock will cause deadlock.
 * The main purpose of this lock is for the kernel to implement mutexes.
 * 
 * Since these locks are both global, they have a high rate of contention, but
 * by converse they are very fast to take and release.
 */

/**
 * \private Base class used for implementing RAII-based locking.
 */
template<class T>
class FastLockMixin
{
public:
    /**
     * Constructor, acquire the lock
     */
    FastLockMixin() { T::lock(); }

    /**
     * Destructor, release the lock
     */
    ~FastLockMixin() { T::unlock(); }

    FastLockMixin(const FastLockMixin&)=delete;
    FastLockMixin& operator= (const FastLockMixin&)=delete;
};

/**
 * \private Base class used for implementing RAII-based recursive locking.
 */
template<class T>
class LockMixin
{
public:
    /**
     * Constructor, acquire the lock if the current thread is not already
     * holding it
     */
    LockMixin()
    {
        wasLocked=T::pushLock();
    }

    /**
     * Destructor, release the lock if this object was the one acquiring it
     * in the first place
     */
    ~LockMixin()
    {
        if(!wasLocked) T::unlock();
    }

    LockMixin(const LockMixin&)=delete;
    LockMixin& operator= (const LockMixin&)=delete;
private:
    bool wasLocked;
};

/**
 * \private Base class used for implementing RAII-based unlock.
 */
template<class T>
class UnlockBase
{
public:
    /**
     * Constructor, release the lock
     * \param l the object that was used to acquire the lock
     */
    UnlockBase(T& l) { (void)l; T::unlock(); }

    /**
     * Destructor.
     * Acquire back the lock.
     */
    ~UnlockBase() { T::lock(); }

    UnlockBase(const UnlockBase&)=delete;
    UnlockBase& operator= (const UnlockBase&)=delete;
};

/**
 * \name Global Interrupt Lock
 * \{
 */

/**
 * Class for acquiring/releasing the global lock from interrupt context.
 * Can only be used inside an interrupt service routine.
 * The static methods of the class can be used to take/release the lock
 * manually.
 * Alternatively, it can be instantiated, to act as a RAII lock which releases
 * the lock automatically when it goes out of scope.
 *
 * NOTE: FastGlobalLockFromIrq cannot be nested and cannot be used before the
 * kernel is started (well, interrupts cannot occur before the kernel is started
 * so this can't happen unless you do the mistake of using this lock outside of
 * interrupt context...) Attempting to do so will lead to undefined behavior.
 * There is no nestable version of this lock. Interrupt code should be kept as
 * simple as possible, so you don't need it.
 *
 * On single core architectures, this lock becomes a no-operation as there is
 * no need for it, while on multi core architectures an implementation-defined
 * mechanism is used to guarantee that only one core at a time can hold the
 * global lock even from interrupt context, hence the global name.
 */
class FastGlobalLockFromIrq: public FastLockMixin<FastGlobalLockFromIrq>
{
public:
    /**
     * Acquire the global lock from an IRQ context.
     */
    static inline void lock()
    {
        #ifdef WITH_SMP
        irqDisabledHwIrqLockAcquire(HwLocks::GIL);
        #endif
    }

    /**
     * Release the global lock from an IRQ context.
     */
    static inline void unlock()
    {
        #ifdef WITH_SMP
        irqDisabledHwIrqLockRelease(HwLocks::GIL);
        #endif
    }
};

/**
 * This class allows to temporarily release the global lock in a scope it was
 * held using a FastGlobalLockFromIrq.
 *
 * Example:
 * \code
 * {
 *     FastGlobalIrqLock dLock;
 *     //Now holding the lock
 *     {
 *         FastGlobalIrqUnlock eLock(dLock);
 *         //Now lock released
 *     }
 *     //Now holding again the lock
 * }
 * //Finally lock released
 * \endcode
 */
using FastGlobalUnlockFromIrq = UnlockBase<FastGlobalLockFromIrq>;


/**
 * Class for acquiring/releasing the global lock.
 * The static methods of the class can be used to take/release the lock
 * manually.
 * Alternatively, it can be instantiated, to act as a RAII lock which releases
 * the lock automatically when it goes out of scope.
 *
 * You should try to keep the critical sections that hold this lock as short
 * as possible. Holding this lock grants you the capability to call kernel
 * function whose function name starts with the IRQ prefix.
 *
 * NOTE: FastGlobalIrqLock cannot be nested and cannot be used before the
 * kernel is started! Attempting to do so will lead to undefined behavior.
 * For such cases, use GlobalIrqLock instead.
 * 
 * On single core architectures, this lock is implemented by disabling
 * interrupts, while on multi core architectures interrupts on the core that
 * acquired the lock are disabled and an implementation-defined mechanism is
 * used to guarantee that only one core at a time can hold the global lock,
 * hence the global name.
 *
 * \note This class replaces the FastInterruptDisableLock class in Miosix v2.x
 */
class FastGlobalIrqLock: public FastLockMixin<FastGlobalIrqLock>
{
public:
    /**
     * Acquire the global lock.
     * \note This method replaces fastDisableInterrupts() from Miosix v2.x
     */
    static inline void lock() noexcept
    {
        // If we're compiling with extra checks, verify that we also never use
        // FastGlobalIrqLock early at boot or in a context where the global lock
        // is already taken.
        if(extraChecks==ExtraChecks::Kernel && areInterruptsEnabled()==false)
            errorHandler(Error::UNEXPECTED);

        fastDisableIrq();
        FastGlobalLockFromIrq::lock();
    }

    /**
     * Release the global lock.
     * \note This method replaces fastEnableInterrupts() from Miosix v2.x
     */
    static inline void unlock() noexcept
    {
        FastGlobalLockFromIrq::unlock();
        fastEnableIrq();
    }
};

/**
 * This class allows to temporarily release the global lock in a scope it was
 * held using a FastGlobalIrqLock.
 *
 * Example:
 * \code
 * {
 *     FastGlobalIrqLock dLock;
 *     //Now holding the lock
 *     {
 *         FastGlobalIrqUnlock eLock(dLock);
 *         //Now lock released
 *     }
 *     //Now holding again the lock
 * }
 * //Finally lock released
 * \endcode
 *
 * \note This class replaces the FastInterruptEnableLock class in Miosix v2.x
 */
using FastGlobalIrqUnlock = UnlockBase<FastGlobalIrqLock>;


/**
 * RAII lock for recursively acquiring/releasing the global lock.
 * 
 * You should try to keep the critical sections that hold this lock as short
 * as possible. Holding this lock grants you the capability to call kernel
 * function whose function name starts with the IRQ prefix.
 * 
 * GlobalIrqLock can be nested, like recursive mutexes. 
 * It is also safe for use before the kernel is started, and in this case it
 * does nothing, since interrupts aren't yet enabled and only one core is
 * running.
 *
 * On single core architectures, the global lock is implemented by disabling
 * interrupts, while on multi core architectures interrupts on the core that
 * acquired the lock are disabled and an implementation-defined mechanism is
 * used to guarantee that only one core at a time can hold the global lock,
 * hence the global name.
 *
 * \note This class replaces the InterruptDisableLock class in Miosix v2.x
 * There are no replacements for the disableInterrupts() and enableInterrupts()
 * functions in Miosix v2.x, use the class GlobalIrqLock as a RAII lock
 * instead.
 */
class GlobalIrqLock: public LockMixin<GlobalIrqLock>
{
private:
    /**
     * \private Take the lock
     */
    static inline void lock() noexcept
    {
        FastGlobalIrqLock::lock();
        #ifdef WITH_SMP
        holdingCore=getCurrentCoreId();
        #endif
    }

    /**
     * \private Returns if we are within a critical section protected by this
     * lock. This only returns true for the thread that is currently holding
     * the lock. Requires interrupts to be disabled.
     * Note that this only makes sense in SMP platforms because in non-SMP
     * platforms having interrupts disabled is equivalent to taking the
     * GlobalIrqLock.
     */
    static inline bool irqDisabledInLockedSection() noexcept
    {
        #ifdef WITH_SMP
        return holdingCore==getCurrentCoreId();
        #else
        return true;
        #endif
    }

    /**
     * \private Take the lock if it's not already taken. Returns true only
     * if we already were in a locked section, otherwise false.
     * This primitive is used to implement recursion (see the LockMixin<> class)
     */
    static inline bool pushLock() noexcept
    {
        #ifdef WITH_SMP
        // Checking getCurrentCoreId() with interrupts potentially enabled is
        // unsafe due to thread migrations, so disable interrupts while we
        // check.
        bool ie=areInterruptsEnabled();
        fastDisableIrq();
        if(holdingCore==getCurrentCoreId())
        {
            // We are holding the lock, nothing to do
            if(ie) fastEnableIrq();
            return true;
        }
        // Take the lock
        FastGlobalLockFromIrq::lock();
        holdingCore=getCurrentCoreId();
        return false;
        #else
        // If interrupts are not enabled, we already have the lock taken
        if(!areInterruptsEnabled()) return true;
        // Otherwise take it
        FastGlobalIrqLock::lock();
        return false;
        #endif
    }

    /**
     * \private Release the lock
     */
    static void unlock() noexcept
    {
        #ifdef WITH_SMP
        holdingCore=0xFF;
        #endif
        FastGlobalIrqLock::unlock();
    }

    /// These kernel classes and functions need to access lock/unlock
    friend class LockMixin<GlobalIrqLock>;
    friend class UnlockBase<GlobalIrqLock>;
    friend class Thread;
    friend class LibAtomicQuickLock;
    /// The lock is actually initialized in IRQstartKernel(); when the boot
    /// process is complete
    friend void IRQstartKernel();
    #ifdef WITH_SMP
    /// The core currently holding the global lock
    static unsigned char holdingCore;
    #endif
};

/**
 * This class allows to temporarily release the global lock in a scope it was
 * held using a GlobalIrqLock
 *
 * Example:
 * \code
 * {
 *     GlobalIrqLock dLock;
 *     //Now holding the lock
 *     {
 *         GlobalIrqUnlock eLock(dLock);
 *         //Now lock released
 *     }
 *     //Now holding again the lock
 * }
 * //Finally lock released
 * \endcode
 *
 * \note This class replaces the InterruptEnableLock class in Miosix v2.x
 */
using GlobalIrqUnlock = UnlockBase<GlobalIrqLock>;

/**
 * \}
 */


/**
 * \name Pause Kernel Lock
 * \{
 */

/**
 * RAII lock for acquiring/releasing the pause kernel lock.
 * It is also safe for use before the kernel is started, and in this case it
 * does nothing, since interrupts aren't yet enabled and only one core is
 * running.
 * 
 * NOTE: FastPauseKernelLock cannot be nested and cannot be used before the
 * kernel is started! Attempting to do so will lead to undefined behavior.
 * For such cases, use PauseKernelLock instead.
 */
class FastPauseKernelLock: public FastLockMixin<FastPauseKernelLock>
{
public:
    /**
     * Take the lock.
     * \note This method is not a replacement for pauseKernel() because it
     * does not allow recursive locking. Use PauseKernelLock instead.
     */
    static inline void lock() noexcept
    {
        // We should never take the PK lock while we're holding the GIL, as this
        // can cause deadlocks in SMP architectures. The only exception is early
        // at boot, before the kernel is started where there's only one core
        // running and it's running code as-if both the PK and GIL are taken.
        // If we're compiling with extra checks, verify this property using an
        // edge detection method. If the kernel isn't paused (this excludes the
        // early at boot case) and interrupts are disabled, then it's a bug.
        // Additionally, we also should never use FastPauseKernelLock early at
        // boot or in a context where the kernel is already paused, so if the
        // kernel is already paused fail too.
        if(extraChecks==ExtraChecks::Kernel)
        {
            if(isKernelPaused()) errorHandler(Error::UNEXPECTED);
            else if(areInterruptsEnabled()==false) errorHandler(Error::UNEXPECTED);
        }

        #ifdef WITH_SMP
        // The SMP implementation needs to disable local interrupts when
        // changing the lock's state to prevent the scheduler running in between
        // taking/releasing the HW lock and setting the holdingCore variable.
        //   If it did, and it put us back in ready state, anyone else trying to
        // take the lock would have to wait in the spinlock even if the lock
        // isn't really taken.
        //   However, this means it cannot run if the kernel is not yet started.
        // So we also check for that and bail out early if so.
        //   These comments also apply to unlock().
        fastDisableIrq();
        irqDisabledFastLock();
        fastEnableIrq();
        #else
        holdingCore=0;
        asm volatile("":::"memory");
        #endif
    }

    /**
     * Release the lock.
     * \note This method is not a replacement for restartKernel() because it
     * does not allow recursive locking. Use PauseKernelLock instead.
     */
    static inline void unlock() noexcept
    {
        #ifdef WITH_SMP
        // See comments in lock().
        fastDisableIrq();
        irqDisabledFastUnlock();
        fastEnableIrq();
        #else //WITH_SMP
        holdingCore=0xff;
        asm volatile("":::"memory");
        #endif //WITH_SMP
    
        // If we missed a preemption, yield. This mechanism works the
        // same way as the hardware implementation of interrupts that remain
        // pending if they occur while interrupts are disabled.
        // The scheduler sets pendingWakeup to true any time it is called but it
        // could not run due to the lock being taken.
        // With the tickless kernel, this is also important to prevent deadlocks
        // as the idle thread is no longer periodically interrupted by timer
        // ticks and it does pause the kernel. If the interrupt that wakes up
        // a thread fails to call the scheduler since the idle thread paused the
        // kernel and pendingWakeup is not set, this could cause a deadlock.
        if(pendingWakeup)
        {
            pendingWakeup=false;
            Thread::yield();
        }
    }

private:
    /**
     * \private Lock the Pause Kernel lock when interrupts are disabled.
     * This is used when the thread holding the lock is exiting from a wait.
     */
    static inline void irqDisabledFastLock() noexcept
    {
        #ifdef WITH_SMP
        irqDisabledHwLockAcquire(HwLocks::PK);
        holdingCore=getCurrentCoreId();
        #else
        holdingCore=0;
        #endif
    }

    /**
     * \private Take the lock if it's not already taken. Returns true only
     * if we already were in a locked section, otherwise false.
     * This primitive is used to implement recursion (see the LockMixin<> class)
     */
    static inline bool pushLock() noexcept
    {
        #ifdef WITH_SMP
        // Checking getCurrentCoreId() with interrupts potentially enabled is
        // unsafe due to thread migrations, so disable interrupts while we
        // check.
        fastDisableIrq();
        #endif
        bool res;
        if(holdingCore==getCurrentCoreId())
        {
            // We are holding the lock, nothing to do
            res=true;
        } else {
            // Take the lock
            irqDisabledFastLock();
            res=false;
        }
        #ifdef WITH_SMP
        fastEnableIrq();
        #endif
        return res;
    }

    /**
     * \private Lock the Pause Kernel lock when interrupts are disabled.
     * This is used when the thread holding the lock is entering a wait.
     */
    static inline void irqDisabledFastUnlock() noexcept
    {
        holdingCore=0xff;
        #ifdef WITH_SMP
        irqDisabledHwLockRelease(HwLocks::PK);
        #endif
    }

    /// The scheduler needs to set pendingWakeup if it is called within a
    /// pauseKernel.
    template<typename T> friend class basic_scheduler;
    /// Thread wait primitives use irqDisabledFastLock() and
    /// irqDisabledFastUnlock().
    friend class Thread;
    friend class PauseKernelLock;
    /// The lock is actually initialized in IRQstartKernel(); when the boot
    /// process is complete
    friend void IRQstartKernel();
    /// The core holding the lock or 0xff.
    static unsigned char holdingCore;
    /// Whether the scheduler was invoked while the lock is taken.
    static bool pendingWakeup;
};

/**
 * RAII lock for recursively acquiring/releasing the pause kernel lock.
 * PauseKernelLock can be nested, like recursive mutexes.
 * It is also safe for use before the kernel is started, and in this case it
 * does nothing, since interrupts aren't yet enabled and only one core is
 * running.
 * 
 * \note There are no replacements for the pauseKernel() and enableKernel()
 * functions in Miosix v2.x, use the class PauseKernelLock instead.
 */
class PauseKernelLock: public LockMixin<PauseKernelLock>
{
public:
    /**
     * Return true if the pause kernel lock is currently taken, false otherwise.
     * Note: this function can be called during the boot process.
     *
     * \note Acquiring the global lock or disabling interrupts does not affect
     * the result returned by this function.
     * This function replaces isKernelPaused() from Miosix v2.x.
     * \return true if preemption is disabled on the current core.
     */
    static inline bool inLockedSection()
    {
        #ifdef WITH_SMP
        // Checking getCurrentCoreId() with interrupts potentially enabled is
        // unsafe due to thread migrations
        bool ie=areInterruptsEnabled();
        fastDisableIrq();
        bool result=FastPauseKernelLock::holdingCore==getCurrentCoreId();
        if(ie) fastEnableIrq();
        return result;
        #else //WITH_SMP
        return FastPauseKernelLock::holdingCore==getCurrentCoreId();
        #endif //WITH_SMP
    }

private:
    /**
     * \private Take the lock.
     */
    static void lock();

    /**
     * \private Release the lock.
     */
    static void unlock();

    /**
     * \private Take the lock if it's not already taken. Returns true only
     * if we already were in a locked section, otherwise false.
     */
    static bool pushLock();

    /// These classes need to access lock/unlock
    friend class LockMixin<PauseKernelLock>;
    friend class UnlockBase<PauseKernelLock>;
    /// These functions need pushLock and unlock
    friend void mallocLockImpl();
    friend void mallocUnlockImpl();
};

/**
 * Return true if the pause kernel lock is currently taken, false otherwise.
 *
 * \note Acquiring the global lock or disabling interrupts does not affect the
 * result returned by this function.
 * \return true if preemption is disabled on the current core.
 */
inline bool isKernelPaused() noexcept
{
    return PauseKernelLock::inLockedSection();
}

/**
 * This class allows to temporarily restart kernel in a scope where it is
 * paused with a PauseKernelLock.
 *
 * Example:
 * \code
 *
 * //Kernel running
 * {
 *     PauseKernelLock dLock;
 *     //Kernel paused
 *     {
 *         PauseKernelUnlock eLock(dLock);
 *         //Kernel running again
 *     }
 *     //Kernel back to paused
 * }
 * //Kernel running
 * \endcode
 *
 * \note This class replaces the RestartKernelLock class in Miosix v2.x
 */
using PauseKernelUnlock = UnlockBase<PauseKernelLock>;

/**
 * \}
 */

/**
 * \name Deep Sleep Lock
 * \{
 */

/**
 * Prevent the microcontroller from entering a deep sleep state. Most commonly
 * used by device drivers requiring clocks or power rails that would be disabled
 * when entering deep sleep to perform blocking operations while informing the
 * scheduler that deep sleep is currently not possible.
 * Can be nested multiple times and called by different device drivers
 * simultaneously. If N calls to deepSleepLock() are made, then N calls to
 * deepSleepUnlock() need to be made before deep sleep is enabled back.
 */
#ifdef WITH_DEEP_SLEEP
void deepSleepLock() noexcept;
#else //WITH_DEEP_SLEEP
inline void deepSleepLock() noexcept {} //Nothing to do
#endif //WITH_DEEP_SLEEP

/**
 * Used to signal the scheduler that a critical section where deep sleep should
 * not be entered has completed. If N calls to deepSleepLock() are made, then N
 * calls to deepSleepUnlock() need to be made before deep sleep is enabled back.
 */
#ifdef WITH_DEEP_SLEEP
void deepSleepUnlock() noexcept;
#else //WITH_DEEP_SLEEP
inline void deepSleepUnlock() noexcept {} //Nothing to do
#endif //WITH_DEEP_SLEEP

/**
 * This class is a RAII lock for temporarily prevent entering deep sleep.
 * This call avoids the error of not reenabling deep sleep capability since it
 * is done automatically.
 */
class DeepSleepLock
{
public:
    DeepSleepLock() { deepSleepLock(); }

    ~DeepSleepLock() { deepSleepUnlock(); }

    DeepSleepLock(const DeepSleepLock&)=delete;
    DeepSleepLock& operator= (const DeepSleepLock&)=delete;
};

/**
 * \}
 */

} //namespace miosix
