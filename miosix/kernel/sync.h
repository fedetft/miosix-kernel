/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011 by Terraneo Federico             *
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
 //Miosix kernel

#ifndef SYNC_H
#define SYNC_H

#include "kernel.h"
#include <vector>

#ifndef PTHREAD_MUTEX_RECURSIVE
/*
 * Small dirty trick.
 * Starting from Miosix 1.58 the upgrade to the new gcc 4.5.2 and the
 * miosix-specific patches becomes mandatory, or the kernel will no longer
 * compile. To make the transition as smooth as possible to those who are
 * using the old arm-miosix-eabi-gcc compiler, we need to print an
 * informative warning telling them they have to upgrade.
 * Now, since the new patches make the #define PTHREAD_MUTEX_RECURSIVE
 * available we can use it to check which compiler is being used.
 */
#warning "You upgraded to gcc 4.5.2 and mandatory newlib patches, did you?"
#warning "If not, see http://www.webalice.it/fede.tft/miosix/gcc-4.5.2.html"
#endif

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
        } else pthread_mutex_init(&impl,NULL);
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
        bool hppw;
        {
            PauseKernelLock dLock;
            hppw=PKunlock(dLock);
        }
        #ifdef SCHED_TYPE_EDF
        if(hppw) Thread::yield();//The other thread might have a closer deadline
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
     * Lock mutex, can be called only with kernel paused one level deep
     * (pauseKernel calls can be nested).<br>
     * \param dLock the PauseKernelLock instance that paused the kernel.
     * \return true if a higher priority thread was woken
     */
    bool PKunlock(PauseKernelLock& dLock);

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
    ConditionVariable();

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
    void wait(FastMutex& m);

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

    /**
     * \internal
     * \struct WaitingData
     * This struct is used to make a list of waiting threads.
     */
    struct WaitingData
    {
        Thread *p;///<\internal Thread that is waiting
        WaitingData *next;///<\internal Next thread in the list
    };

    WaitingData *first;///<Pointer to first element of waiting fifo
    WaitingData *last;///<Pointer to last element of waiting fifo
};

/**
 * A timer that can be used to measure time intervals.<br>Its resolution equals
 * the kernel tick.<br>Maximum interval is 2^31-1 ticks.
 */
class Timer
{
public:
    /**
     * Constructor. Timer is initialized in stopped status.
     */
    Timer();

    /**
     * Start the timer
     */
    void start();

    /**
     * Stop the timer. After stop, Timer can be started and stopped again to
     * count non-contiguous timer intervals.
     */
    void stop();

    /**
     * \return true if timer is running
     */
    bool isRunning() const;

    /**
     * get the interval, in kernel ticks.
     * \return the number of tick between start and stop. Returns -1 if the
     * timer was never started, if interval is called after start but before
     * stop, or if it overflowed.
     *
     * To read the vaue of a timer without stopping it, you can use its copy
     * constructor to create another timer, and stop it while the first timer
     * keeps running.
     */
    int interval() const;

    /**
     * Clear the timer and set it to not running state.
     */
    void clear();

    //Using default copy constructor, operator = and destructor
private:
    //Timer data
    bool first;///< True if start has never been called
    bool running;///< True if timer is running
    long long start_tick;///< The kernel tick when start was called.
    long long tick_count;///< The tick count
};

/**
 * A queue, used to transfer data between TWO threads, or between ONE thread
 * and an IRQ.<br>
 * If you need to tranfer data between more than two threads, you need to
 * use mutexes to ensure that only one thread at a time calls get, and only one
 * thread at a time calls put.<br>
 * This queue is meant to be a static or global class. Dynamically creating a
 * queue with new or on the stack must be done with care, to avoid deleting a
 * queue with a waiting thread, and to avoid situations where a thread tries to
 * access a deleted queue.
 * \tparam T the type of elements in the queue
 * \tparam len the length of the Queue. Value 0 is forbidden
 */
template <typename T, unsigned int len>
class Queue
{
public:
    /**
     * Constructor, create a new empty queue.
     */
    Queue() : waiting(0), num_elem(0), put_pos(0), get_pos(0) {}

    ///Size of the queue
    static const unsigned int queueSize=len;

    /**
     * \return true if the queue is empty
     */
    bool isEmpty() const { return num_elem==0; }

    /**
     * \return true if the queue is full
     */
    bool isFull() const { return num_elem==len; }
	
    /**
     * If a queue is empty, waits until the queue is not empty.
     */
    void waitUntilNotEmpty()
    {
        FastInterruptDisableLock dLock;
        IRQwakeWaitingThread();
        while((put_pos==get_pos)&&(num_elem==0))//while queue empty
        {
            waiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
            IRQwakeWaitingThread();
        }
    }
	
    /**
     * If a queue is full, waits until the queue is not full
     */
    void waitUntilNotFull()
    {
        FastInterruptDisableLock dLock;
        IRQwakeWaitingThread();
        while((put_pos==get_pos)&&(num_elem!=0))//while queue full
        {
            waiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
            IRQwakeWaitingThread();
        }
    }

    /**
     * Get an element from the queue. If the queue is empty, then sleep until
     * an element becomes available.
     * \param elem an element from the queue
     */
    void get(T& elem)
    {
        FastInterruptDisableLock dLock;
        IRQwakeWaitingThread();
        while((put_pos==get_pos)&&(num_elem==0))//while queue empty
        {
            waiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
            IRQwakeWaitingThread();
        }
        num_elem--;
        elem=buffer[get_pos];
        if(++get_pos==len) get_pos=0;
    }

    /**
     * Put an element to the queue. If the queue is full, then sleep until a
     * place becomes available.
     * \param elem element to add to the queue
     */
    void put(T elem)
    {
        FastInterruptDisableLock dLock;
        IRQwakeWaitingThread();
        while((put_pos==get_pos)&&(num_elem!=0))//while queue full
        {
            waiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
            IRQwakeWaitingThread();
        }
        num_elem++;
        buffer[put_pos]=elem;
        if(++put_pos==len) put_pos=0;
    }

    /**
     * Get an element from the queue, only if the queue is not empty.<br>
     * Can ONLY be used inside an IRQ, or when interrupts are disabled.
     * \param elem an element from the queue. The element is valid only if the
     * return value is true
     * \return true if the queue was not empty
     */
    bool IRQget(T& elem)
    {
        IRQwakeWaitingThread();
        if((put_pos==get_pos)&&(num_elem==0))//If queue empty
        {
            return false;
        }
        num_elem--;
        elem=buffer[get_pos];
        if(++get_pos==len) get_pos=0;
        return true;
    }

    /**
     * Get an element from the queue, only if the queue is not empty.<br>
     * Can ONLY be used inside an IRQ, or when interrupts are disabled.
     * \param elem an element from the queue. The element is valid only if the
     * return value is true
     * \param hppw is not modified if no thread is woken or if the woken thread
     * has a lower or equal priority than the currently running thread, else is
     * set to true
     * \return true if the queue was not empty
     */
    bool IRQget(T& elem, bool& hppw)
    {
        if(waiting->IRQgetPriority() > Thread::IRQgetCurrentThread()->
                IRQgetPriority())
            hppw=true;
        IRQwakeWaitingThread();
        if((put_pos==get_pos)&&(num_elem==0))//If queue empty
        {
            return false;
        }
        num_elem--;
        elem=buffer[get_pos];
        if(++get_pos==len) get_pos=0;
        return true;
    }

    /**
     * Put an element to the queue, only if th queue is not full.<br>
     * Can ONLY be used inside an IRQ, or when interrupts are disabled.
     * \param elem element to add. The element has been added only if the
     * return value is true
     * \return true if the queue was not full.
     */
    bool IRQput(T elem)
    {
        IRQwakeWaitingThread();
        if((put_pos==get_pos)&&(num_elem!=0))//If queue full
        {
            return false;
        }
        num_elem++;
        buffer[put_pos]=elem;
        if(++put_pos==len) put_pos=0;
        return true;
    }

    /**
     * Put an element to the queue, only if th queue is not full.<br>
     * Can ONLY be used inside an IRQ, or when interrupts are disabled.
     * \param elem element to add. The element has been added only if the
     * return value is true
     * \param hppw is not modified if no thread is woken or if the woken thread
     * has a lower or equal priority than the currently running thread, else is
     * set to true
     * \return true if the queue was not full.
     */
    bool IRQput(T elem, bool& hppw)
    {
        if(waiting->IRQgetPriority() > Thread::IRQgetCurrentThread()->
                IRQgetPriority())
            hppw=true;
        IRQwakeWaitingThread();
        if((put_pos==get_pos)&&(num_elem!=0))//If queue full
        {
            return false;
        }
        num_elem++;
        buffer[put_pos]=elem;
        if(++put_pos==len) put_pos=0;
        return true;
    }

    /**
     * Clear all items in the queue.<br>
     * Cannot be used inside an IRQ
     */
    void reset()
    {
        FastInterruptDisableLock lock;
        IRQwakeWaitingThread();
        put_pos=get_pos=num_elem=0;
    }
    
    /**
     * Same as reset(), but to be used only inside IRQs or when interrupts are
     * disabled.
     */
    void IRQreset()
    {
        IRQwakeWaitingThread();
        put_pos=get_pos=num_elem=0;
    }
	
private:
    //Unwanted methods
    Queue(const Queue& s);///< No public copy constructor
    Queue& operator = (const Queue& s);///< No publc operator =

    /**
     * Wake an eventual waiting thread.
     * Must be called when interrupts are disabled
     */
    void IRQwakeWaitingThread()
    {
        if(waiting!=NULL)
        {
            waiting->IRQwakeup();//Wakeup eventual waiting thread
            waiting=NULL;
        }
    }

    //Queue data
    T buffer[len];///< queued elements are put here. Used as a ring buffer
    Thread *waiting;///< If not null holds the thread waiting
    volatile unsigned int num_elem;///< nuber of elements in the queue
    volatile unsigned int put_pos;///< index of buffer where to get next element
    volatile unsigned int get_pos; ///< index of buffer where to put next element
};

//This partial specialization is meant to to produce compiler errors in case an
//attempt is made to instantiate a Queue with zero size, as it is forbidden
template<typename T> class Queue<T,0> {};

/**
 * \}
 */

}; //namespace miosix

#endif //SYNC_H
