/***************************************************************************
 *   Copyright (C) 2008-2023 by Terraneo Federico                          *
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

#include "kernel.h"
#include "intrusive.h"
#include <vector>

namespace miosix {

/**
 * \addtogroup Sync
 * \{
 */

/**
 * Fast mutex without support for priority inheritance
 */
class FastMutex
{
public:
    /**
     * Mutex options, passed to the constructor to set additional options.<br>
     * The DEFAULT option indicates the default Mutex type.
     */
    enum Options
    {
        DEFAULT,    ///< Default mutex
        RECURSIVE   ///< Mutex is recursive
    };

    /**
     * Constructor, initializes the mutex.
     */
    FastMutex(Options opt=DEFAULT)
    {
        if(opt==RECURSIVE)
        {
            pthread_mutexattr_t temp;
            pthread_mutexattr_init(&temp);
            pthread_mutexattr_settype(&temp,PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&impl,&temp);
            pthread_mutexattr_destroy(&temp);
        } else pthread_mutex_init(&impl,nullptr);
    }

    /**
     * Locks the critical section. If the critical section is already locked,
     * the thread will be queued in a wait list.
     */
    void lock()
    {
        pthread_mutex_lock(&impl);
    }

    /**
     * Acquires the lock only if the critical section is not already locked by
     * other threads. Attempting to lock again a recursive mutex will fail, and
     * the mutex' lock count will not be incremented.
     * \return true if the lock was acquired
     */
    bool tryLock()
    {
        return pthread_mutex_trylock(&impl)==0;
    }

    /**
     * Unlocks the critical section.
     */
    void unlock()
    {
        pthread_mutex_unlock(&impl);
    }

    /**
     * \internal
     * \return the FastMutex implementation defined mutex type
     */
    pthread_mutex_t *get()
    {
        return &impl;
    }

    /**
     * Destructor
     */
    ~FastMutex()
    {
        pthread_mutex_destroy(&impl);
    }

private:
    FastMutex(const FastMutex&);
    FastMutex& operator= (const FastMutex&);

    pthread_mutex_t impl;
};

//Forward declaration
class ConditionVariable;

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
     * Mutex options, passed to the constructor to set additional options.<br>
     * The DEFAULT option indicates the default Mutex type.
     */
    enum Options
    {
        DEFAULT,    ///< Default mutex
        RECURSIVE   ///< Mutex is recursive
    };

    /**
     * Constructor, initializes the mutex.
     */
    Mutex(Options opt=DEFAULT);

    /**
     * Locks the critical section. If the critical section is already locked,
     * the thread will be queued in a wait list.
     */
    void lock()
    {
        PauseKernelLock dLock;
        PKlock(dLock);
    }
	
    /**
     * Acquires the lock only if the critical section is not already locked by
     * other threads. Attempting to lock again a recursive mutex will fail, and
     * the mutex' lock count will not be incremented.
     * \return true if the lock was acquired
     */
    bool tryLock()
    {
        PauseKernelLock dLock;
        return PKtryLock(dLock);
    }
    
    /**
     * Unlocks the critical section.
     */
    void unlock()
    {
        #ifdef SCHED_TYPE_EDF
        bool hppw;
        {
            PauseKernelLock dLock;
            hppw=PKunlock(dLock);
        }
        if(hppw) Thread::yield();//The other thread might have a closer deadline
        #else
        {
            PauseKernelLock dLock;
            PKunlock(dLock);
        }
        #endif //SCHED_TYPE_EDF
    }
	
private:
    //Unwanted methods
    Mutex(const Mutex& s);///< No public copy constructor
    Mutex& operator = (const Mutex& s);///< No publc operator =
    //Uses default destructor

    /**
     * Lock mutex, can be called only with kernel paused one level deep
     * (pauseKernel calls can be nested). If another thread holds the mutex,
     * this call will restart the kernel and wait (that's why the kernel must
     * be paused one level deep).<br>
     * \param dLock the PauseKernelLock instance that paused the kernel.
     */
    void PKlock(PauseKernelLock& dLock);
    
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
     * Acquires the lock only if the critical section is not already locked by
     * other threads. Attempting to lock again a recursive mutex will fail, and
     * the mutex' lock count will not be incremented.<br>
     * Can be called only with kernel paused one level deep.
     * (pauseKernel calls can be nested).
     * \param dLock the PauseKernelLock instance that paused the kernel.
     * \return true if the lock was acquired
     */
    bool PKtryLock(PauseKernelLock& dLock);

    /**
     * Unlock mutex, can be called only with kernel paused one level deep
     * (pauseKernel calls can be nested).<br>
     * \param dLock the PauseKernelLock instance that paused the kernel.
     * \return true if a higher priority thread was woken
     */
    bool PKunlock(PauseKernelLock& dLock);
    
    /**
     * Unlock mutex all levels of a recursive mutex, can be called only with
     * kernel paused one level deep (pauseKernel calls can be nested).<br>
     * \param dLock the PauseKernelLock instance that paused the kernel.
     * \return the mutex recursive depth (how many times it was locked by the
     * owner). Zero means the mutex is locked one level deep (lock() was called
     * once), one means two levels deep, etc. 
     */
    unsigned int PKunlockAllDepthLevels(PauseKernelLock& dLock);

    /// Thread currently inside critical section, if NULL the critical section
    /// is free
    Thread *owner;

    /// If this mutex is locked, it is added to a list of mutexes held by the
    /// thread that owns this mutex. This field is necessary to make the list.
    Mutex *next;

    /// Waiting thread are stored in this min-heap, sorted by priority
    std::vector<Thread *> waiting;

    /// Used to hold nesting depth for recursive mutexes, -1 if not recursive
    int recursiveDepth;

    //Friends
    friend class ConditionVariable;
    friend class Thread;
};

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

private:
    //Unwanted methods
    Lock(const Lock& l);///< No public copy constructor
    Lock& operator = (const Lock& l);///< No publc operator =

    T& mutex;///< Reference to locked mutex
};

/**
 * This class allows to temporarily re unlock a mutex in a scope where
 * it is locked <br>
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

private:
    //Unwanted methods
    Unlock(const Unlock& l);
    Unlock& operator= (const Unlock& l);

    T& mutex;///< Reference to locked mutex
};

/**
 * \internal
 * This class is used to make a list of threads that are waiting on a condition
 * variable. It is used by the kernel, and should not be used by end users.
 */
class CondData : public IntrusiveListItem
{
public:
    CondData(Thread *thread) : thread(thread) {}
    Thread *thread; ///<\internal Thread that is waiting
};

/**
 * Possible return values of timedWait
 */
enum class TimedWaitResult
{
    NoTimeout,
    Timeout
};

/**
 * A condition variable class for thread synchronization, available from
 * Miosix 1.53.<br>
 * One or more threads can wait on the condition variable, and the signal()
 * and broadcast() allow to wake ne or all the waiting threads.<br>
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
    TimedWaitResult wait(Lock<T>& l, long long absTime)
    {
        return timedWait(l.get());
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
     * \param m a locked Mutex
     */
    void wait(FastMutex& m)
    {
        //Memory layout of ConditionVariable is the same of pthread_cond_t and
        //the algorithm would be exactly the same
        pthread_cond_wait(reinterpret_cast<pthread_cond_t*>(this),m.get());
    }

    /**
     * Unlock the Mutex and wait until woken up or timeout occurs.
     * If more threads call wait() they must do so specifying the same mutex,
     * otherwise the behaviour is undefined.
     * \param m a locked Mutex
     * \param absTime absolute timeout time in nanoseconds
     * \return whether the return was due to a timout or wakeup
     */
    TimedWaitResult timedWait(Mutex& m, long long absTime);

    /**
     * Unlock the FastMutex and wait until woken up or timeout occurs.
     * If more threads call wait() they must do so specifying the same mutex,
     * otherwise the behaviour is undefined.
     * \param m a locked Mutex
     * \param absTime absolute timeout time in nanoseconds
     * \return whether the return was due to a timout or wakeup
     */
    TimedWaitResult timedWait(FastMutex& m, long long absTime);

    /**
     * Wakeup one waiting thread.
     * Currently implemented policy is fifo.
     */
    void signal();

    /**
     * Wakeup all waiting threads.
     */
    void broadcast();

private:
    //Unwanted methods
    ConditionVariable(const ConditionVariable& );
    ConditionVariable& operator= (const ConditionVariable& );

    //Memory layout must be kept in sync with pthread_cond, see wait(FastMutex& m)
    IntrusiveList<CondData> condList;
};

/**
 * \}
 */

} //namespace miosix
