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

#pragma once

#include "config/miosix_settings.h"
#include "kernel/scheduler/sched_types.h"
#include "stdlib_integration/libstdcpp_integration.h"
#include "intrusive.h"
#include "cpu_time_counter_types.h"
#include "interfaces/cpu_const.h"

/**
 * \namespace miosix
 * All user available kernel functions, classes are inside this namespace.
 */
namespace miosix {
    
/**
 * \addtogroup Kernel
 * \{
 */

/**
 * Returns OS time, which is a monotonic clock started when the OS booted.<br>
 * Warning! This function replaces the getTick() in previous versions of the
 * kernel, but unlike getTick(), getTime() cannot be called with interrupts
 * disabled. For that, you need to call IRQgeTime().
 * \return current time in nanoseconds
 */
long long getTime() noexcept;

/**
 * Returns OS time, which is a monotonic clock started when the OS booted.<br>
 * Must be called with interrupts disabled, or within an interrupt.
 * \return current time in nanoseconds
 */
long long IRQgetTime() noexcept;

/**
 * Possible return values of timedWait
 */
enum class TimedWaitResult
{
    NoTimeout,
    Timeout
};

/**
 * \internal
 * Data structure holding the information of a thread waiting on a
 * synchronization primitive such as a Mutex or ConditionVariable
 *
 * NOTE: since this class is only meant to implement synchronization primitives
 * and in Miosix from version 3 onwards synchronization primitives are
 * implemented using PauseKernelLock, this class uses PK prefixed member
 * functions to access thread properties, and assumes the class is only accessed
 * while holding the PauseKernelLock
 */
class WaitToken : public IntrusiveListItem
{
public:
    /**
     * Constructor
     * \param t thread about to block waiting on a synchronization primitive
     */
    WaitToken(Thread *t) : t(t) {}

    #ifdef SCHED_TYPE_EDF
    /**
     * \return the thread deadline, used for sorting waiting threads by
     * earliest deadline first
     *
     *  NOTE: we can just call PKgetPriority() and not bother with savedPriority
     * as the case we care about is when a thread has locked a single mutex and
     * that's the one that was atomically unlocked as part of the wait(). Since
     * we get here after the mutex has been unlocked, the priority or better,
     * deadline, has already been de-inherited, if at all. Even if some weird
     * code did the antipattern of locking some more mutex and thus waiting with
     * some mutex locked and a priority inheritance occurs while waiting, since
     * we have only one list no memory corruption is possible in
     * removeFromWaitQueue
     */
    long long getTime() { return t->PKgetPriority().get(); }
    #endif

    Thread *t; ///<\internal Waiting thread
};

//Forwrd declaration
class MemoryProfiling;
class Mutex;
enum class PriorityPolicy;
template<PriorityPolicy pp> class WaitQueue;
class GlobalIrqLock;
class FastGlobalIrqLock;
class PauseKernelLock;
class FastPauseKernelLock;
#ifdef WITH_PROCESSES
class ProcessBase;
class Process;
class FaultData;
class SyscallParameters;
#endif //WITH_PROCESSES

/**
 * This class represents a thread. It has methods for creating, deleting and
 * handling threads.<br>It has private constructor and destructor, since memory
 * for a thread is handled by the kernel.<br>To create a thread use the static
 * producer method create().<br>
 * Methods that have an effect on the current thread, that is, the thread that
 * is calling the method are static.<br>
 * Calls to non static methods must be done with care, because a thread can
 * terminate at any time. For example, if you call wakeup() on a terminated
 * thread, the behavior is undefined.
 */
class Thread : public IntrusiveListItem
{
public:

    /**
     * Thread options, can be passed to Thread::create to set additional options
     * of the thread.
     * More options can be specified simultaneously by ORing them together.
     * The DEFAULT option indicates the default thread creation.
     */
    enum Options
    {
        DEFAULT=0,    ///< Default thread options (JOINABLE thread)
        JOINABLE=0,   ///< Thread is created joinable
        DETACHED=1<<0 ///< Thread is detached instead of joinable
    };

    /**
     * Producer method, creates a new thread.
     * \param startfunc the entry point function for the thread
     * \param stacksize size of thread stack, its minimum is the constant
     * STACK_MIN.
     * The size of the stack must be divisible by 4, otherwise it will be
     * rounded to a number divisible by 4.
     * \param priority the thread's priority, whose range depends on the
     * selected scheduler, see miosix_settings.h and constant DEFAULT_PRIORITY
     * \param argv a void* pointer that is passed as pararmeter to the entry
     * point function
     * \param options thread options, such ad Thread::DETACHED
     * \return a reference to the thread created, that can be used, for example,
     * to delete it, or nullptr in case of errors.
     *
     * NOTE: starting from Miosix 3 threads are created JOINABLE by default.
     * To createa detached thread, pass as options Thread::DETACHED
     */
    static Thread *create(void *(*startfunc)(void *), unsigned int stacksize,
                            Priority priority=DEFAULT_PRIORITY, void *argv=nullptr,
                            Options options=DEFAULT);

    /**
     * Same as create(void (*startfunc)(void *), unsigned int stacksize,
     * Priority priority=1, void *argv=nullptr)
     * but in this case the entry point of the thread returns a void*
     * \param startfunc the entry point function for the thread
     * \param stacksize size of thread stack, its minimum is the constant
     * STACK_MIN.
     * The size of the stack must be divisible by 4, otherwise it will be
     * rounded to a number divisible by 4.
     * \param priority the thread's priority, whose range depends on the
     * selected scheduler, see miosix_settings.h and constant DEFAULT_PRIORITY
     * \param argv a void* pointer that is passed as pararmeter to the entry
     * point function
     * \param options thread options, such ad Thread::DETACHED
     * \return a reference to the thread created, that can be used, for example,
     * to delete it, or nullptr in case of errors.
     *
     * NOTE: starting from Miosix 3 threads are created JOINABLE by default.
     * To createa detached thread, pass as options Thread::DETACHED
     */
    static Thread *create(void (*startfunc)(void *), unsigned int stacksize,
                            Priority priority=DEFAULT_PRIORITY, void *argv=nullptr,
                            Options options=DEFAULT)
    {
        //Just call the other version with a cast.
        return Thread::create(reinterpret_cast<void *(*)(void*)>(startfunc),
            stacksize,priority,argv,options);
    }

    /**
     * When called, suggests the kernel to pause the current thread, and run
     * another one.
     * <br>CANNOT be called when the kernel is paused.
     */
    static void yield();

    /**
     * Put the thread to sleep for a number of milliseconds.
     * <br>The actual precision depends on the underlying hardware timer.
     * \param ms the number of milliseconds. If it is ==0 this method will
     * return immediately
     *
     * CANNOT be called when the kernel is paused.
     */
    static void sleep(unsigned int ms);

    /**
     * Put the thread to sleep for a number of nanoseconds.
     * <br>The actual precision depends on the underlying hardware timer.
     * \param ns the number of nanoseconds. If it is <=0 this method will
     * return immediately
     *
     * CANNOT be called when the kernel is paused.
     */
    static void nanoSleep(long long ns);
    
    /**
     * Put the thread to sleep until the specified absolute time is reached.
     * If the time is in the past, returns immediately.
     * To make a periodic thread, this is the recomended way
     * \code
     * void periodicThread()
     * {
     *     const long long period=90000000; //Run every 90 milliseconds
     *     long long time=getTime();
     *     for(;;)
     *     {
     *         //Do work
     *         time+=period;
     *         Thread::nanoSleepUntil(time);
     *     }
     * }
     * \endcode
     * \param absoluteTime when to wake up, in nanoseconds
     *
     * CANNOT be called when the kernel is paused.
     */
    static void nanoSleepUntil(long long absoluteTimeNs);

    /**
     * This method stops the thread until wakeup() is called.
     * Ths method is useful to implement any kind of blocking primitive,
     * including device drivers.
     *
     * CANNOT be called when the kernel is paused.
     */
    static void wait();

    /**
     * This method stops the thread until wakeup() is called.
     * Ths method is useful to implement any kind of blocking primitive,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where the kernel is paused (preemption disabled).
     * Preemption will be enabled during the waiting period, and disabled back
     * before this method returns.
     *
     * \param dLock the PauseKernelLock object that was used to disable
     * preemption in the current context.
     */
    static void PKrestartKernelAndWait(PauseKernelLock& dLock)
    {
        (void)dLock; //Common implementation doesn't need it
        return PKrestartKernelAndWaitImpl();
    }

    /**
     * This method stops the thread until wakeup() is called.
     * Ths method is useful to implement any kind of blocking primitive,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where the kernel is paused (preemption disabled).
     * Preemption will be enabled during the waiting period, and disabled back
     * before this method returns.
     *
     * \param dLock the FastPauseKernelLock object that was used to disable
     * preemption in the current context.
     */
    static void PKrestartKernelAndWait(FastPauseKernelLock& dLock)
    {
        (void)dLock; //Common implementation doesn't need it
        return PKrestartKernelAndWaitImpl();
    }

    /**
     * This method stops the thread until wakeup() is called.
     * Ths method is useful to implement any kind of blocking primitive,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where interrupts are disbled, interrupts will be enabled
     * during the waiting period, and disabled back before this method returns.
     * NOTE: it's not possible to firt take a PauseKernelLock, then take a
     * GlobalIrqLock and call this function. When this function is called,
     * no PauseKernelLock should be taken, cosider using PKrestartKernelAndWait
     * without taking the GlobalIrqLock instead.
     *
     * \param dLock the GlobalIrqLock object that was used to disable
     * interrupts in the current context.
     *
     * \note This member function replaces IRQenableIrqAndWait() in Miosix v2.x,
     * which in turn replaces IRQwait in older code
     */
    static void IRQglobalIrqUnlockAndWait(GlobalIrqLock& dLock)
    {
        (void)dLock; //Common implementation doesn't need it
        return IRQglobalIrqUnlockAndWaitImpl();
    }

    /**
     * This method stops the thread until wakeup() is called.
     * Ths method is useful to implement any kind of blocking primitive,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where interrupts are disbled, interrupts will be enabled
     * during the waiting period, and disabled back before this method returns.
     * NOTE: it's not possible to firt take a PauseKernelLock, then take a
     * FastGlobalIrqLock and call this function. When this function is called,
     * no PauseKernelLock should be taken, cosider using PKrestartKernelAndWait
     * without taking the FastGlobalIrqLock instead.
     *
     * \param dLock the FastGlobalIrqLock object that was used to disable
     * interrupts in the current context.
     *
     * \note This member function replaces IRQenableIrqAndWait() in Miosix v2.x
     * which in turn replaces IRQwait in older code
     */
    static void IRQglobalIrqUnlockAndWait(FastGlobalIrqLock& dLock)
    {
        (void)dLock; //Common implementation doesn't need it
        return IRQglobalIrqUnlockAndWaitImpl();
    }

    /**
     * This method stops the thread until wakeup() is called or the specified
     * absolute time in nanoseconds is reached.
     * Ths method is thus a combined IRQwait() and absoluteSleep(), and is
     * useful to implement any kind of blocking primitive with timeout,
     * including device drivers.
     *
     * \param absoluteTimeoutNs absolute time after which the wait times out
     * \return TimedWaitResult::Timeout if the wait timed out
     */
    static TimedWaitResult timedWait(long long absoluteTimeNs);

    /**
     * This method stops the thread until wakeup() is called or the specified
     * absolute time in nanoseconds is reached.
     * Ths method is thus a combined IRQwait() and absoluteSleep(), and is
     * useful to implement any kind of blocking primitive with timeout,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where the kernel is paused (preemption disabled).
     * Preemption will be enabled during the waiting period, and disabled back
     * before this method returns.
     *
     * \param dLock the PauseKernelLock object that was used to disable
     * preemption in the current context.
     * \param absoluteTimeoutNs absolute time after which the wait times out
     * \return TimedWaitResult::Timeout if the wait timed out
     */
    static TimedWaitResult PKrestartKernelAndTimedWait(PauseKernelLock& dLock,
            long long absoluteTimeNs)
    {
        (void)dLock; //Common implementation doesn't need it
        return PKrestartKernelAndTimedWaitImpl(absoluteTimeNs);
    }

    /**
     * This method stops the thread until wakeup() is called or the specified
     * absolute time in nanoseconds is reached.
     * Ths method is thus a combined IRQwait() and absoluteSleep(), and is
     * useful to implement any kind of blocking primitive with timeout,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where the kernel is paused (preemption disabled).
     * Preemption will be enabled during the waiting period, and disabled back
     * before this method returns.
     *
     * \param dLock the FastPauseKernelLock object that was used to disable
     * preemption in the current context.
     * \param absoluteTimeoutNs absolute time after which the wait times out
     * \return TimedWaitResult::Timeout if the wait timed out
     */
    static TimedWaitResult PKrestartKernelAndTimedWait(FastPauseKernelLock& dLock,
            long long absoluteTimeNs)
    {
        (void)dLock; //Common implementation doesn't need it
        return PKrestartKernelAndTimedWaitImpl(absoluteTimeNs);
    }

    /**
     * This method stops the thread until wakeup() is called or the specified
     * absolute time in nanoseconds is reached.
     * Ths method is thus a combined IRQwait() and absoluteSleep(), and is
     * useful to implement any kind of blocking primitive with timeout,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where interrupts are disbled, interrupts will be enabled
     * during the waiting period, and disabled back before this method returns.
     * NOTE: it's not possible to firt take a PauseKernelLock, then take a
     * GlobalIrqLock and call this function. When this function is called,
     * no PauseKernelLock should be taken, cosider using PKrestartKernelAndTimedWait
     * without taking the GlobalIrqLock instead.
     *
     * \param dLock the GlobalIrqLock object that was used to disable
     * interrupts in the current context.
     * \param absoluteTimeoutNs absolute time after which the wait times out
     * \return TimedWaitResult::Timeout if the wait timed out
     *
     * \note This member function replaces IRQenableIrqAndTimedWait() in Miosix v2.x
     */
    static TimedWaitResult IRQglobalIrqUnlockAndTimedWait(GlobalIrqLock& dLock,
            long long absoluteTimeNs)
    {
        (void)dLock; //Common implementation doesn't need it
        return IRQglobalIrqUnlockAndTimedWaitImpl(absoluteTimeNs);
    }

    /**
     * This method stops the thread until wakeup() is called or the specified
     * absolute time in nanoseconds is reached.
     * Ths method is thus a combined IRQwait() and absoluteSleep(), and is
     * useful to implement any kind of blocking primitive with timeout,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where interrupts are disbled, interrupts will be enabled
     * during the waiting period, and disabled back before this method returns.
     * NOTE: it's not possible to firt take a PauseKernelLock, then take a
     * FastGlobalIrqLock and call this function. When this function is called,
     * no PauseKernelLock should be taken, cosider using PKrestartKernelAndTimedWait
     * without taking the FastGlobalIrqLock instead.
     *
     * \param dLock the FastGlobalIrqLock object that was used to disable
     * interrupts in the current context.
     * \param absoluteTimeoutNs absolute time after which the wait times out
     * \return TimedWaitResult::Timeout if the wait timed out
     *
     * \note This member function replaces IRQenableIrqAndTimedWait() in Miosix v2.x
     */
    static TimedWaitResult IRQglobalIrqUnlockAndTimedWait(FastGlobalIrqLock& dLock,
            long long absoluteTimeNs)
    {
        (void)dLock; //Common implementation doesn't need it
        return IRQglobalIrqUnlockAndTimedWaitImpl(absoluteTimeNs);
    }

    /**
     * Wakeup a thread.
     * This function causes a context switch if the woken thread priorirty is
     * higher than the one that is currently running one on at least one core.
     * <br>CANNOT be called when the kernel is paused.
     */
    void wakeup();

    /**
     * Wakeup a thread.
     * Starting from Miosix 3 this function handles internally the check if the
     * woken thread priority is higher than the one running on at least one core.
     * If a core is found which is running a lower priority thread, then if
     * this core is the same as the one that took the pauseKernel, no preemption
     * is caused but pendingWakeup is set to a context switch will occur
     * immediately when releasing the pauseKernel lock. This is the only case
     * that can happen on a single core CPU. On a multi core CPU, however, the
     * core running a lower priority thread may be another core. In this case,
     * a preemption is caused immediately as the pauseKernel lock only disables
     * preemption on the core that took the lock.
     * 
     * Can only be called when the kernel is paused.
     */
    void PKwakeup();

    /**
     * Wakeup a thread.
     * Starting from Miosix 3 this function causes the scheduler interrupt to
     * become pending if the woken thread priorirty is higher than the one that
     * is currently running on at least one core. A context switch will thus
     * occur as soon as interrupts are enabled again.
     * 
     * Can only be called inside an IRQ or when interrupts are disabled.
     */
    void IRQwakeup();
    
    /**
     * \return a pointer to the current thread.
     *
     * Returns a valid pointer also if called before the kernel is started.
     */
    static Thread *getCurrentThread()
    {
        //Safe to call without disabling IRQ, see implementation
        return IRQgetCurrentThread();
    }

    /**
     * \return a pointer to the current thread.
     *
     * Returns a valid pointer also if called before the kernel is started.
     */
    static Thread *PKgetCurrentThread()
    {
        //Safe to call without disabling IRQ, see implementation
        return IRQgetCurrentThread();
    }

    /**
     * \return a pointer to the current thread.
     *
     * Returns a valid pointer also if called before the kernel is started.
     */
    static Thread *IRQgetCurrentThread();

    /**
     * Check if a thread exists
     * \param p thread to check
     * \return true if thread exists, false if does not exist or has been
     * deleted. A joinable thread is considered existing until it has been
     * joined, even if it returns from its entry point (unless it is detached
     * and terminates).
     *
     * Can be called when the kernel is paused.
     */
    static bool exists(Thread *p);

    /**
     * Returns the priority of a thread.<br>
     * To get the priority of the current thread use:
     * \code Thread::getCurrentThread()->getPriority(); \endcode
     * If the thread is currently locking one or more mutexes, this member
     * function returns the current priority, which can be higher than the
     * original priority due to priority inheritance.
     * \return current priority of the thread
     */
    Priority getPriority();

    /**
     * Same as getPriority(), but meant to be used when the kernel is paused.
     */
    Priority PKgetPriority()
    {
        return getPriority(); //Safe to call directly, see implementation
    }

    /**
     * Same as getPriority(), but meant to be used inside an IRQ, or when
     * interrupts are disabled.
     */
    Priority IRQgetPriority()
    {
        return getPriority(); //Safe to call directly, see implementation
    }

    /**
     * Set the priority of this thread.<br>
     * This member function changed from previous Miosix versions since it is
     * now static. This implies a thread can no longer set the priority of
     * another thread.
     * \param priority the thread's priority, whose range depends on the
     * selected scheduler, see miosix_settings.h
     */
    static void setPriority(Priority pr);

    /**
     * Suggests a thread to terminate itself. Note that this method only makes
     * testTerminate() return true on the specified thread. If the thread does
     * not call testTerminate(), or if it calls it but does not delete itself
     * by returning from entry point function, it will NEVER
     * terminate. The user is responsible for implementing correctly this
     * functionality.<br>Thread termination is implemented like this to give
     * time to a thread to deallocate resources, close files... before
     * terminating.<br>The first call to terminate on a thread will make it
     * return prematurely form wait(), sleep() and timedWait() call, but only
     * once.<br>Can be called when the kernel is paused.
     */
    void terminate();

    /**
     * This method needs to be called periodically inside the thread's main
     * loop.
     * \return true if somebody outside the thread called terminate() on this
     * thread.
     *
     * If it returns true the thread must free all resources and terminate by
     * returning from its main function.
     * <br>Can be called when the kernel is paused.
     */
    static bool testTerminate();

    /**
     * Detach the thread if it was joinable, otherwise do nothing.<br>
     * If called on a deleted joinable thread on which join was not yet called,
     * it allows the thread's memory to be deallocated.<br>
     * If called on a thread that is not yet deleted, the call detaches the
     * thread without deleting it.
     * If called on an already detached thread, it has undefined behaviour.
     */
    void detach();

    /**
     * \return true if the thread is detached
     */
    bool isDetached() const;

    /**
     * Wait until a joinable thread is terminated.<br>
     * If the thread already terminated, this function returns immediately.<br>
     * Calling join() on the same thread multiple times, from the same or
     * multiple threads is not recomended, but in the current implementation
     * the first call will wait for join, and the other will return false.<br>
     * Trying to join the thread join is called in returns false, but must be
     * avoided.<br>
     * Calling join on a detached thread might cause undefined behaviour.
     * \param result If the entry point function of the thread to join returns
     * void *, the return value of the entry point is stored here, otherwise
     * the content of this variable is undefined. If nullptr is passed as result
     * the return value will not be stored.
     * \return true on success, false on failure
     */
    bool join(void** result=nullptr);

    /**
     * \internal
     * This method is only meant to implement functions to check the available
     * stack in a thread. Returned pointer is constant because modifying the
     * stack through it must be avoided.
     * \return pointer to bottom of stack of current thread.
     */
    static const unsigned int *getStackBottom();

    /**
     * \internal
     * \return the size of the stack of the current thread.
     */
    static int getStackSize();

    /**
     * \internal
     * To be used in interrupts where a context switch can occur to check if the
     * stack of the thread being preempted has overflowed.
     * Note that since Miosix 3 all peripheral interrupts no longer perform a
     * full context save/restore thus you cannot call this functions from such
     * interrupts.
     *
     * If the overflow check failed for a kernel thread or a thread running in
     * kernelspace this function causes a reboot. On a platform with processes
     * this function calls IRQreportFault() if the stack overflow happened in
     * userspace, causing the process to segfault.
     */
    static void IRQstackOverflowCheck();
    
    #ifdef WITH_PROCESSES

    /**
     * \return the process associated with the thread 
     */
    ProcessBase *getProcess() { return proc; }
    
    /**
     * \internal
     * Can only be called inside an IRQ, its use is to switch a thread between
     * userspace/kernelspace and back to perform context switches
     */
    static void IRQhandleSvc();
    
    /**
     * \internal
     * Can only be called inside an IRQ, its use is to report a fault so that
     * in case the fault has occurred within a process while it was executing
     * in userspace, the process can be terminated.
     * \param fault data about the occurred fault
     * \return true if the fault was caused by a process, false otherwise.
     */
    static bool IRQreportFault(const FaultData& fault);
    
    #endif //WITH_PROCESSES

    #ifdef WITH_PTHREAD_KEYS

    /**
     * \internal, used to implement pthread_setspecific
     */
    int setPthreadKeyValue(pthread_key_t key, void * const value)
    {
        if(key>=MAX_PTHREAD_KEYS) return EINVAL;
        pthreadKeyValues[key]=value;
        return 0;
    }

    /**
     * \internal, used to implement pthread_getspecific
     */
    void *getPthreadKeyValue(pthread_key_t key)
    {
        if(key>=MAX_PTHREAD_KEYS) return nullptr; //No way to report error
        return pthreadKeyValues[key];
    }

    #endif //WITH_PTHREAD_KEYS

    //Unwanted methods
    Thread(const Thread& p) = delete;
    Thread& operator = (const Thread& p) = delete;

private:
    /**
     * Curren thread status
     */
    class ThreadFlags
    {
    public:
        /**
         * Constructor, sets flags to default.
         */
        ThreadFlags(): flags(0) {}

        /**
         * Set the wait flag of the thread.
         * Can only be called with interrupts disabled or within an interrupt.
         * \param self thread whose status changed
         * \param waiting if true the flag will be set, otherwise cleared
         */
        void IRQsetWait(Thread *self, bool waiting);

        /**
         * Set the sleep flag of the thread.
         * Can only be called with interrupts disabled or within an interrupt.
         * \param self thread whose status changed
         */
        void IRQsetSleep(Thread *self);

        /**
         * Used by IRQwakeThreads to clear both the sleep and wait flags,
         * waking threads doing absoluteSleep() as well as timedWait()
         * \param self thread whose status changed
         */
        void IRQclearSleepAndWait(Thread *self);

        /**
         * Set the wait_join flag of the thread.
         * Can only be called with interrupts disabled or within an interrupt.
         * \param self thread whose status changed
         * \param waiting if true the flag will be set, otherwise cleared
         */
        void IRQsetJoinWait(Thread *self, bool waiting);

        /**
         * Set the deleted flag of the thread. This flag can't be cleared.
         * Can only be called with interrupts disabled or within an interrupt.
         * If the thread is joinable, it is not actually set to DELETED yet, but
         * to ZOMBIE until it has either been joined or detached.
         * \param self thread whose status changed
         */
        void IRQsetDeleted(Thread *self);

        /**
         * Set the sleep flag of the thread. This flag can't be cleared.
         * Can only be called with interrupts disabled or within an interrupt.
         */
        void IRQsetDeleting()
        {
            flags |= DELETING;
        }

        /**
         * Set the detached flag. This flag can't be cleared.
         * Can only be called with interrupts disabled or within an interrupt.
         */
        void IRQsetDetached()
        {
            flags |= DETACHED;
        }
        
        /**
         * Set the userspace flag of the thread.
         * Can only be called with interrupts disabled or within an interrupt.
         * \param sleeping if true the flag will be set, otherwise cleared
         */
        void IRQsetUserspace(bool userspace)
        {
            if(userspace) flags |= USERSPACE; else flags &= ~USERSPACE;
        }

        /**
         * \return true if the wait flag is set
         */
        bool isWaiting() const { return flags & WAIT; }

        /**
         * \return true if the sleep flag is set
         */
        bool isSleeping() const { return flags & SLEEP; }

        /**
         * \return true if the deleted and the detached flags are set
         * Deleted thrad can be deallocated immediately.
         */
        bool isDeleted() const { return (flags & 0x14)==0x14; }

        /**
         * \return true if the thread has been deleted, but its resources cannot
         * be reclaimed because it has not yet been joined
         */
        bool isZombie() const { return flags & DELETED; }

        /**
         * \return true if the deleting flag is set
         */
        bool isDeleting() const { return flags & DELETING; }

        /**
         * \return true if the thread is in the ready status
         */
        bool isReady() const { return (flags & 0x27)==0; }

        /**
         * \return true if the thread is detached
         */
        bool isDetached() const { return flags & DETACHED; }

        /**
         * \return true if the thread is waiting a join
         */
        bool isWaitingJoin() const { return flags & WAIT_JOIN; }
        
        /**
         * \return true if the thread is running unprivileged inside a process.
         */
        bool isInUserspace() const { return flags & USERSPACE; }

        //Unwanted methods
        ThreadFlags(const ThreadFlags& p) = delete;
        ThreadFlags& operator = (const ThreadFlags& p) = delete;

    private:
        ///\internal Thread is in the wait status. A call to wakeup will change
        ///this
        static const unsigned int WAIT=1<<0;

        ///\internal Thread is sleeping.
        static const unsigned int SLEEP=1<<1;

        ///\internal Thread is deleted. It will continue to exist until the
        ///idle thread deallocates its resources
        static const unsigned int DELETED=1<<2;

        ///\internal Somebody outside the thread asked this thread to delete
        ///itself.<br>This will make Thread::testTerminate() return true.
        static const unsigned int DELETING=1<<3;

        ///\internal Thread is detached
        static const unsigned int DETACHED=1<<4;

        ///\internal Thread is waiting for a join
        static const unsigned int WAIT_JOIN=1<<5;
        
        ///\internal Thread is running in userspace
        static const unsigned int USERSPACE=1<<6;

        unsigned char flags;///<\internal flags are stored here
    };
    
    #ifdef WITH_PROCESSES

    /**
     * \internal
     * Causes a thread belonging to a process to switch to userspace, and
     * execute userspace code. This function returns when the process performs
     * a syscall or faults.
     * \return the syscall parameters used to serve the system call.
     */
    static SyscallParameters switchToUserspace();

    /**
     * Create a thread to be used inside a process. The thread is created in
     * WAIT status, a wakeup() on it is required to actually start it.
     * \param startfunc entry point
     * \param proc process to which this thread belongs
     */
    static Thread *createUserspace(void *(*startfunc)(void *), Process *proc);
    
    /**
     * Setup the userspace context of the thread, so that it can be later
     * switched to userspace. Must be called only once for each thread instance
     * \param entry userspace entry point
     * \param argc number of arguments
     * \param argvSp pointer to arguments array. Since the args block is stored
     * above the stack and this is the pointer into the first byte of the args
     * block, this pointer doubles as the initial stack pointer when the process
     * is started.
     * \param envp pointer to environment variables
     * \param gotBase base address of the GOT, also corresponding to the start
     * of the RAM image of the process
     * \param stackSize size of the userspace stack, used for bound checking
     */
    static void setupUserspaceContext(unsigned int entry, int argc, void *argvSp,
        void *envp, unsigned int *gotBase, unsigned int stackSize);
    
    #endif //WITH_PROCESSES

    /**
     * Constructor, initializes thread data.
     * \param watermark pointer to watermark area
     * \param stacksize thread's stack size
     * \param defaultReent true if the global reentrancy structure is to be used
     */
    Thread(unsigned int *watermark, unsigned int stacksize, bool defaultReent);

    /**
     * Destructor
     */
    ~Thread();
    
    /**
     * Helper function to initialize a Thread
     * \param startfunc entry point function
     * \param stacksize stack size for the thread
     * \param argv argument passed to the thread entry point
     * \param options thread options
     * \param defaultReent true if the default C reentrancy data should be used
     * \return a pointer to a thread, or nullptr in case there are not enough
     * resources to create one.
     */
    static Thread *doCreate(void *(*startfunc)(void *), unsigned int stacksize,
                            void *argv, Options options, bool defaultReent);

    /**
     * Thread launcher, all threads start from this member function, which calls
     * the user specified entry point. When the entry point function returns,
     * it marks the thread as deleted so that the idle thread can dellocate it.
     * If exception handling is enebled, this member function also catches any
     * exception that propagates through the entry point.
     * \param threadfunc pointer to the entry point function
     * \param argv argument passed to the entry point
     */
    static void threadLauncher(void *(*threadfunc)(void*), void *argv);

    /**
     * Common implementation of all PKglobalIrqUnlockAndWait calls
     */
    static void PKrestartKernelAndWaitImpl();

    /**
     * Common implementation of all IRQglobalIrqUnlockAndWait calls
     */
    static void IRQglobalIrqUnlockAndWaitImpl();

    /**
     * Common implementation of all PK timedWait calls
     */
    static TimedWaitResult PKrestartKernelAndTimedWaitImpl(long long absoluteTimeNs);

    /**
     * Common implementation of all IRQ timedWait calls
     */
    static TimedWaitResult IRQglobalIrqUnlockAndTimedWaitImpl(long long absoluteTimeNs);

    /**
     * Same as exists() but is meant to be called only inside an IRQ or when
     * interrupts are disabled.
     */
    static bool IRQexists(Thread *p);

    /**
     * Allocates the idle thread and makes cur point to it
     * Can only be called before the kernel is started, is called exactly once
     * so that getCurrentThread() always returns a pointer to a valid thread or
     * by IRQstartKernel to create the idle thread, whichever comes first.
     * \return the newly allocated idle thread
     */
    static Thread *allocateIdleThread();
    
    /**
     * \return the C reentrancy structure of the currently running thread
     */
    static struct _reent *getCReent();

    //Thread data
    SchedulerData schedData; ///< Scheduler data, only used by class Scheduler
    ThreadFlags flags;///< thread status
    ///Saved priority.
    ///When mutexLocked!=nullptr it stores the value of priority that this
    ///thread will have when it unlocks all mutexes. This is because when a
    ///thread locks a mutex its priority can change due to priority inheritance.
    ///When not locking any Mutex,may need to be kept equal to the actual
    ///priority, see ConditionVariable.
    Priority savedPriority;
    ///List of mutexes locked by this thread
    Mutex *mutexLocked;
    ///If the thread is waiting on a Mutex, mutexWaiting points to that Mutex
    Mutex *mutexWaiting;
    ///If the thread is waiting on a WaitQueue, entry in the wait list
    WaitToken waitQueueItem;
    unsigned int *watermark;///< pointer to watermark area
    unsigned int ctxsave[CTXSAVE_SIZE];///< Holds cpu registers during ctxswitch
    unsigned int stacksize;///< Contains stack size
    ///This union is used to join threads. When the thread to join has not yet
    ///terminated and no other thread called join it contains (Thread *)nullptr,
    ///when a thread calls join on this thread it contains the thread waiting
    ///for the join, and when the thread terminated it contains (void *)result
    union
    {
        Thread *waitingForJoin;///<Thread waiting to join this
        void *result;          ///<Result returned by entry point
    } joinData;
    /// Per-thread instance of data to make the C and C++ libraries thread safe.
    struct _reent *cReentrancyData;
    CppReentrancyData cppReentrancyData;
    #ifdef WITH_PROCESSES
    ///Process to which this thread belongs. Kernel threads point to a special
    ///ProcessBase that represents the kernel.
    ProcessBase *proc;
    ///Pointer to the set of saved registers for when the thread is running in
    ///user mode. For kernel threads (i.e, threads where proc==kernel) this
    ///pointer is null
    unsigned int *userCtxsave;
    unsigned int *userWatermark;
    #endif //WITH_PROCESSES
    #ifdef WITH_CPU_TIME_COUNTER
    CPUTimeCounterPrivateThreadData timeCounterData;
    #endif //WITH_CPU_TIME_COUNTER
    #ifdef WITH_PTHREAD_KEYS
    ///Thread local values associated to pthread keys
    void *pthreadKeyValues[MAX_PTHREAD_KEYS];
    #endif //WITH_PTHREAD_KEYS
    
    //friend functions
    //Needs access to flags
    friend void IRQwakeThreads(long long);
    //Needs to create the idle thread
    friend void IRQstartKernel();
    //Needs access to savedPriority, mutexLocked and flags.
    friend class Mutex;
    //Needs access to savedPriority
    template<PriorityPolicy pp> friend class WaitQueue;
    //Needs access to flags, schedData
    friend class PriorityScheduler;
    //Needs access to flags, schedData
    friend class ControlScheduler;
    //Needs access to flags, schedData
    friend class EDFScheduler;
    //Needs access to cppReent
    friend class CppReentrancyAccessor;
    #ifdef WITH_PROCESSES
    //Needs createUserspace(), setupUserspaceContext(), switchToUserspace()
    friend class Process;
    #endif //WITH_PROCESSES
    #ifdef WITH_CPU_TIME_COUNTER
    //Needs access to timeCounterData
    friend class CPUTimeCounter;
    #endif //WITH_CPU_TIME_COUNTER
};

/**
 * \internal
 * This class is used to make a list of sleeping threads.
 * It is used by the kernel, and should not be used by end users.
 */
class SleepData : public IntrusiveListItem
{
public:
    SleepData(Thread *thread, long long wakeupTime)
        : thread(thread), wakeupTime(wakeupTime) {}

    ///\internal Thread that is sleeping
    Thread *thread;
    
    ///\internal When this number becomes equal to the kernel tick,
    ///the thread will wake
    long long wakeupTime;
};

/**
 * \}
 */

} //namespace miosix
