/***************************************************************************
 *   Copyright (C) 2008-2025 by Terraneo Federico                          *
 *   Copyright (C) 2023 by Daniele Cattaneo                                *
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

#include "lock.h"
#include "intrusive.h"
#include "kernel/scheduler/sched_types.h"
#include "kernel/sched_data_structures.h"
#include <vector>

namespace miosix {

/**
 * \addtogroup Sync
 * \{
 */

//Forwrd declaration
class Thread;
class ConditionVariable;
class FastPauseKernelLock;
enum class TimedWaitResult;

/**
 * Mutex options, passed to the constructor to set additional options.
 */
enum MutexOptions
{
    DEFAULT,    ///< Default non-recursive mutex
    RECURSIVE   ///< Mutex is recursive
};

/**
 * Fast mutex without support for priority inheritance
 */
class FastMutex
{
public:
    /**
     * Constructor, initializes the mutex.
     */
    FastMutex(MutexOptions opt=MutexOptions::DEFAULT) : owner(nullptr),
        recursiveDepth(opt==MutexOptions::RECURSIVE ? 0 : -1) {}

    /**
     * Locks the critical section. If the critical section is already locked,
     * the thread will be queued in a wait list.
     * \return always returns 0, to allow tail call optimization when used to
     * implement pthread_mutex_t
     */
    int lock();

    /**
     * Acquires the lock only if the critical section is not already locked by
     * other threads. Attempting to lock again a recursive mutex will fail, and
     * the mutex' lock count will not be incremented.
     * \return true if the lock was acquired
     */
    bool tryLock();

    /**
     * Unlocks the critical section.
     * \return always returns 0, to allow tail call optimization when used to
     * implement pthread_mutex_t
     */
    int unlock();

    /**
     * \internal
     * \return true if mutex is locked
     */
    bool isLocked() const { return owner!=nullptr; }

    //Unwanted methods
    FastMutex(const FastMutex&) = delete;
    FastMutex& operator= (const FastMutex&) = delete;

private:
    /**
     * Implementation code to lock a mutex to a specified depth level.
     * Must be called with the kernel paused. If the mutex is not recursive the
     * mutex is locked only one level deep regardless of the depth value.
     * \param dLock The instance of FastPauseKernelLock
     * \param depth recursive depth at which the mutex will be locked. Zero
     * means the mutex is locked one level deep (as if lock() was called once),
     * one means two levels deep, etc.
     */
    inline void PKlockToDepth(FastPauseKernelLock& dLock, unsigned int depth);

    /**
     * Implementation code to unlock all depth levels of a mutex.
     * Must be called with the kernel paused
     * \param mutex mutex to unlock
     * \return the mutex recursive depth (how many times it was locked by the
     * owner). Zero means the mutex is locked one level deep (lock() was called
     * once), one means two levels deep, etc.
     */
    inline unsigned int PKunlockAllDepthLevels();

    /// Thread currently inside critical section, if nullptr the critical section
    /// is free
    Thread *owner;

    /// Used to hold nesting depth for recursive mutexes, -1 if not recursive
    int recursiveDepth;

    WaitQueue waitQueue; ///< Holds waiting threads, handles prioritization

    //Friends
    friend class ConditionVariable;
};

/**
 * A mutex class with support for priority inheritance. If a thread tries to
 * enter a critical section which is not free, it will be put to sleep and
 * added to a queue of sleeping threads, ordered by priority. The thread that
 * is into the critical section inherits the highest priority among the threads
 * that are waiting if it is higher than its original priority.<br>
 * This mutex is meant to be a static or global class. Dynamically creating a
 * mutex with new or on the stack must be done with care, to avoid deleting a
 * locked mutex, and to avoid situations where a thread tries to lock a
 * deleted mutex.<br>
 */
class Mutex
{
public:
    /**
     * Constructor, initializes the mutex.
     */
    Mutex(MutexOptions opt=MutexOptions::DEFAULT) : owner(nullptr),
        recursiveDepth(opt==MutexOptions::RECURSIVE ? 0 : -1), next(nullptr) {}

    /**
     * Locks the critical section. If the critical section is already locked,
     * the thread will be queued in a wait list.
     * \return always returns 0, to allow tail call optimization when used to
     * implement pthread_mutex_t
     */
    int lock();

    /**
     * Acquires the lock only if the critical section is not already locked by
     * other threads. Attempting to lock again a recursive mutex will fail, and
     * the mutex' lock count will not be incremented.
     * \return true if the lock was acquired
     */
    bool tryLock();

    /**
     * Unlocks the critical section.
     * \return always returns 0, to allow tail call optimization when used to
     * implement pthread_mutex_t
     */
    int unlock();

    /**
     * \internal
     * \return true if mutex is locked
     */
    bool isLocked() const { return owner!=nullptr; }

    //Unwanted methods
    Mutex(const Mutex& s) = delete;
    Mutex& operator= (const Mutex& s) = delete;

private:
    /**
     * Lock mutex to a given depth, can be called only with kernel paused one
     * level deep (pauseKernel calls can be nested). If another thread holds the
     * mutex, this call will restart the kernel and wait (that's why the kernel
     * must be paused one level deep).<br>
     * If the mutex is not recursive the mutex is locked only one level deep
     * regardless of the depth value.
     * \param dLock the PauseKernelLock instance that paused the kernel.
     * \param depth recursive depth at which the mutex will be locked. Zero
     * means the mutex is locked one level deep (as if lock() was called once),
     * one means two levels deep, etc. 
     */
    void PKlockToDepth(PauseKernelLock& dLock, unsigned int depth);
    
    /**
     * Unlock all levels of a recursive mutex, can be called only with
     * kernel paused one level deep (pauseKernel calls can be nested).<br>
     * \return the mutex recursive depth (how many times it was locked by the
     * owner). Zero means the mutex is locked one level deep (lock() was called
     * once), one means two levels deep, etc. 
     */
    unsigned int PKunlockAllDepthLevels();

    /**
     * First part of unlocking a mutex. Remove the mutex from the owner's list
     * of locked mutexes and reduce priority if the current priority was due to
     * having locked this mutex
     */
    inline void deInheritPriority();

    /**
     * Second part of unlocking a mutex. Switch the current mutex owner with
     * the highest priority waiting thread, or leave the mutex without owner
     * if there are no more waiting threads
     */
    inline void chooseNextOwner();

    /**
     * Inherit the given priority towards the thread that is the owner of the
     * locked mutex we're about to lock. Additionally, recursively check if the
     * mutex owner is locked on another mutex and propagate the inheritance
     * as needed
     * \param prio priority to propagate
     */
    void inheritPriorityTowardsMutexOwner(Priority prio);

    /**
     * \param t a thread
     * \return true if the thread's list of locked mutexes with priority
     * inheritance is empty
     */
    static inline bool lockedListEmpty(Thread *t);

    /**
     * Add this mutex to the list of locked mutex with priority inheritance
     * \param t thread to which the current mutex will be added
     */
    inline void addToLockedList(Thread *t);

    /**
     * Remove this mutex from the list of locked mutex with priority inheritance
     * \param t thread from which the current mutex will be removed
     */
    inline void removeFromLockedList(Thread *t);

    /**
     * Check the list of all mutexes with priority inheritance a thread is
     * locking and update the given priority to be the maximum between the
     * initial value and the one of the highest priority thread waiting on
     * said mutexes
     * \param t thread whose list of locked mutex with priority inheritance
     * needs to be checked
     * \param pr initial priority to boost with priority inheritance
     * \return the boosted priority
     */
    static Priority inheritPriorityFromLockedList(Thread *t, Priority pr);

    /// Thread currently inside critical section, if nullptr the critical section
    /// is free
    Thread *owner;

    /// Used to hold nesting depth for recursive mutexes, -1 if not recursive
    int recursiveDepth;

    /// If this mutex is locked, it is added to a list of mutexes held by the
    /// thread that owns this mutex. This field is necessary to make the list.
    Mutex *next;

    /// Waiting thread are stored in this min-heap, sorted by priority
    std::vector<Thread *> waiting;

    //Friends
    friend class ConditionVariable;
    friend class Thread;
};

#ifdef KERNEL_MUTEX_WITH_PRIORITY_INHERITANCE
using KernelMutex = Mutex;
#else //KERNEL_MUTEX_WITH_PRIORITY_INHERITANCE
using KernelMutex = FastMutex;
#endif //KERNEL_MUTEX_WITH_PRIORITY_INHERITANCE

/**
 * Very simple RAII style class to lock a mutex in an exception-safe way.
 * Mutex is acquired by the constructor and released by the destructor.
 */
template<typename T>
class Lock
{
public:
    /**
     * Constructor: locks the mutex
     * \param m mutex to lock
     */
    explicit Lock(T& m): mutex(m)
    {
        mutex.lock();
    }

    /**
     * Destructor: unlocks the mutex
     */
    ~Lock()
    {
        mutex.unlock();
    }

    /**
     * \return the locked mutex
     */
    T& get()
    {
        return mutex;
    }

    //Unwanted methods
    Lock(const Lock& l) = delete;
    Lock& operator= (const Lock& l) = delete;

private:
    T& mutex;///< Reference to locked mutex
};

/**
 * This class allows to temporarily re-unlock a mutex in a scope where
 * it is locked
 *
 * \warning This class can only unlock a recursive mutex that has been locked
 * ONE level deep, so be careful when using it.
 *
 * Example:
 * \code
 * Mutex m;
 *
 * //Mutex unlocked
 * {
 *     Lock<Mutex> dLock(m);
 *
 *     //Now mutex locked
 *
 *     {
 *         Unlock<Mutex> eLock(dLock);
 *
 *         //Now mutex back unlocked
 *     }
 *
 *     //Now mutex again locked
 * }
 * //Finally mutex unlocked
 * \endcode
 */
template<typename T>
class Unlock
{
public:
    /**
     * Constructor, unlock mutex.
     * \param l the Lock that locked the mutex.
     */
    explicit Unlock(Lock<T>& l): mutex(l.get())
    {
        mutex.unlock();
    }

    /**
     * Constructor, unlock mutex.
     * \param m a locked mutex.
     */
    Unlock(T& m): mutex(m)
    {
        mutex.unlock();
    }

    /**
     * Destructor.
     * Disable back interrupts.
     */
    ~Unlock()
    {
        mutex.lock();
    }

    /**
     * \return the unlocked mutex
     */
    T& get()
    {
        return mutex;
    }

    //Unwanted methods
    Unlock(const Unlock& l) = delete;
    Unlock& operator= (const Unlock& l) = delete;

private:
    T& mutex;///< Reference to locked mutex
};

/**
 * A condition variable class for thread synchronization, available from
 * Miosix 1.53.<br>
 * One or more threads can wait on the condition variable, and the signal()
 * and broadcast() methods allow to wake one or all the waiting threads.<br>
 * This class is meant to be a static or global class. Dynamically creating a
 * ConditionVariable with new or on the stack must be done with care, to avoid
 * deleting a ConditionVariable while some threads are waiting, and to avoid
 * situations where a thread tries to wait on a deleted ConditionVariable.<br>
 */
class ConditionVariable
{
public:
    /**
     * Constructor, initializes the ConditionVariable.
     */
    ConditionVariable() {}

    /**
     * Unlock the mutex and wait.
     * If more threads call wait() they must do so specifying the same mutex,
     * otherwise the behaviour is undefined.
     * \param l A Lock instance that locked a Mutex
     */
    template<typename T>
    void wait(Lock<T>& l)
    {
        wait(l.get());
    }

    /**
     * Unlock the mutex and wait until woken up or timeout occurs.
     * If more threads call wait() they must do so specifying the same mutex,
     * otherwise the behaviour is undefined.
     * \param l A Lock instance that locked a Mutex
     * \param absTime absolute timeout time in nanoseconds
     * \return whether the return was due to a timout or wakeup
     */
    template<typename T>
    TimedWaitResult timedWait(Lock<T>& l, long long absTime)
    {
        return timedWait(l.get(),absTime);
    }

    /**
     * Unlock the Mutex and wait.
     * If more threads call wait() they must do so specifying the same mutex,
     * otherwise the behaviour is undefined.
     * \param m a locked Mutex
     */
    void wait(Mutex& m);

    /**
     * Unlock the FastMutex and wait.
     * If more threads call wait() they must do so specifying the same mutex,
     * otherwise the behaviour is undefined.
     * \param m a locked FastMutex
     */
    void wait(FastMutex& m);

    /**
     * Unlock the Mutex and wait until woken up or timeout occurs.
     * If more threads call wait() they must do so specifying the same mutex,
     * otherwise the behaviour is undefined.
     * \param m a locked Mutex
     * \param absTime absolute timeout time in nanoseconds
     * \return whether the return was due to a timeout or wakeup
     */
    TimedWaitResult timedWait(Mutex& m, long long absTime);

    /**
     * Unlock the FastMutex and wait until woken up or timeout occurs.
     * If more threads call wait() they must do so specifying the same mutex,
     * otherwise the behaviour is undefined.
     * \param m a locked FastMutex
     * \param absTime absolute timeout time in nanoseconds
     * \return whether the return was due to a timeout or wakeup
     */
    TimedWaitResult timedWait(FastMutex& m, long long absTime);

    /**
     * Wakeup one waiting thread, chosen based on a wakeup policy that can be
     * chosen at compile time in miosix_settings.h
     */
    void signal();

    /**
     * Wakeup all waiting threads.
     */
    void broadcast();

    /**
     * \internal
     * \return true if no thread is waiting on the condition variable
     */
    bool empty() const;

    //Unwanted methods
    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator= (const ConditionVariable&) = delete;

private:
    WaitQueue waitQueue; ///< Holds waiting threads, handles prioritization
};

/**
 * Semaphore primitive for syncronization between multiple threads and
 * optionally an interrupt handler.
 * 
 * A semaphore is an integer counter that represents the availability of one
 * or more items of a resource. A producer thread can signal the semaphore to
 * increment the counter, making more items available. A consumer thread can
 * wait for the availability of at least one item (i.e. for the counter to be
 * positive) by performing a `wait' on the semaphore. If the counter is already
 * positive, wait decrements the counter and terminates immediately. Otherwise
 * it waits for a `signal' to increment the counter first.
 * 
 * It is possible to use Semaphores to orchestrate communication between IRQ
 * handlers and the main driver code by using the APIs prefixed by `IRQ'. 
 * 
 * \note As with all other synchronization primitives, Semaphores are inherently
 * shared between multiple threads, therefore special care must be taken in
 * managing their lifetime and ownership.
 *
 * \warning Although multiple threads can wait on the same semaphore, they are
 * awakened in fifo order, thus using only a Sempahore in device driver can
 * cause priority inversion. It is suggested for device drivers which are
 * expected to be called concurrently by multiple threads to first lock a
 * KernelMutex to handle thread synchronization and allow kernel builds with
 * full priority inheritance, and then use the Semaphore only to synchronize the
 * single thread that locked the mutex with interrupts.
 * \since Miosix 2.5
 */
class Semaphore
{
public:
    /**
     * Initialize a new semaphore.
     * \param initialCount The initial value of the counter.
     */
    Semaphore(unsigned int initialCount=0) : count(initialCount) {}

    /**
     * Increment the semaphore counter, putting threads out of sleep without
     * triggering a reschedule.
     * \param hppw is set to `true' if a higher priority thread is woken up.
     * Otherwise it is not modified.
     */
    void IRQsignal(bool& hppw);

    /**
     * Increment the semaphore counter, waking up at most one waiting thread.
     */
    void IRQsignal();

    /**
     * Increment the semaphore counter, waking up at most one waiting thread.
     */
    void signal();

    /**
     * Wait for the semaphore counter to be positive, and then decrement it.
     */
    void wait();

    /**
     * Wait up to a given timeout for the semaphore counter to be positive,
     * and then decrement it.
     * \param absTime absolute timeout time in nanoseconds
     * \return whether the return was due to a timeout or wakeup
     */
    TimedWaitResult timedWait(long long absTime);

    /**
     * Decrement the counter only if it is positive. Only for use in IRQ
     * handlers or with interrupts disabled.
     * \return true if the counter was positive.
     */
    inline bool IRQtryWait()
    {
        // Check if the counter is positive
        if(count>0)
        {
            // The wait "succeeded"
            count--;
            return true;
        }
        return false;
    }

    /**
     * Decrement the counter only if it is positive.
     * \return true if the counter was positive.
     */
    bool tryWait()
    {
        // Global interrupt lock because Semaphore is IRQ-safe
        FastGlobalIrqLock dLock;
        return IRQtryWait();
    }

    /**
     * Resets the counter to zero, and returns the old count. Only for use in
     * IRQ handlers or with interrupts disabled.
     * \return The previous counter value.
     */
    inline int IRQreset()
    {
        int old=count;
        count=0;
        return old;
    }

    /**
     * Resets the counter to zero, and returns the old count.
     * \return The previous counter value.
     */
    int reset()
    {
        // Global interrupt lock because Semaphore is IRQ-safe
        FastGlobalIrqLock dLock;
        return IRQreset();
    }

    /**
     * \return the current semaphore counter.
     */
    unsigned int getCount() { return count; }

    // Disallow copies
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator= (const Semaphore&) = delete;

private:
    /**
     * \internal Element of a thread waiting list
     */
    class WaitToken : public IntrusiveListItem
    {
    public:
        WaitToken(Thread *thread) : thread(thread) {}
        Thread *thread; ///<\internal Waiting thread and spurious wakeup token
    };

    /**
     * \internal
     * Internal method that signals the semaphore without triggering a
     * rescheduling for prioritizing newly-woken threads.
     */
    inline Thread *IRQsignalImpl();

    volatile unsigned int count; ///< Counter of the semaphore
    IntrusiveList<WaitToken> fifo; ///< List of waiting threads
};

/**
 * \}
 */

} //namespace miosix
