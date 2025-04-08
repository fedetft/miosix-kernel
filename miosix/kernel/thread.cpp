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

#include "thread.h"
#include "error.h"
#include "logging.h"
#include "sync.h"
#include "lock_private.h"
#include "boot.h"
#include "process.h"
#include "kernel/scheduler/scheduler.h"
#include "stdlib_integration/libc_integration.h"
#include "interfaces_private/cpu.h"
#include "interfaces_private/userspace.h"
#include "interfaces_private/os_timer.h"
#include "interfaces_private/sleep.h"
#include "interfaces_private/smp.h"
#include "timeconversion.h"
#include "pthread_private.h"
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <string.h>
#include <reent.h>

using namespace std;

/*
 * This global variable is used to point to the context of the currently running
 * thread. It is kept even though global variables are generally bad due to
 * performance reasons. It is used by
 * - saveContext() / restoreContext(), to perform context switches
 * - the schedulers, to set the newly running thread before a context switch
 * - IRQportableStartKernel(), to perform the first context switch
 * It is defined in the header interfaces_private/cpu.h
 */
extern "C" {
volatile unsigned int *ctxsave[miosix::CPU_NUM_CORES];
}


namespace miosix {

//Global variables used by thread.cpp. Those that are not static are also used
//in portability.cpp, lock.cpp and by the schedulers.
//These variables MUST NOT be used outside of those files

///\internal Threads currently running on all CPU cores
volatile Thread *runningThreads[CPU_NUM_CORES]={nullptr};

///\internal True if there are threads in the DELETED status. Used by idle thread
static volatile int existDeleted=0;

IntrusiveList<SleepData> sleepingList;///list of sleeping threads

bool kernelStarted=false;///<\internal becomes true after IRQstartKernel.

///Variables shared with lock.cpp for performance and encapsulation reasons
extern volatile bool pendingWakeup;

#ifdef WITH_PROCESSES
/// The proc field of the Thread class for kernel threads points to this object
static ProcessBase *kernel=nullptr;
#endif //WITH_PROCESSES

/**
 * \internal
 * Idle thread. Created when the kernel is started, it physically deallocates
 * memory for deleted threads, and puts the cpu in sleep mode.
 */
void *idleThreadCore0(void *)
{
    for(;;)
    {
        if(atomicSwap(&existDeleted,0)) Scheduler::removeDeadThreads();
        #ifdef WITH_SLEEP
        #ifdef WITH_DEEP_SLEEP
        #ifdef WITH_SMP
        #error Deep sleep not yet supported in SMP kernel
        // TODO: deep sleep turns off clock to all peripherals and cpus, so it
        // can be entered only when all cpus are idle.
        // To implement this the following are needed:
        //  - The scheduler needs to expose a global counting how many cores
        //    are idle
        //  - The deep sleep check/enter logic here needs to run on all cores
        //    (not just core 0)
        //  - Deep sleep is entered when deepSleepCounter==0 and the cpu idle
        //    global reaches the core count
        #endif
        {
            FastGlobalIrqLock lock;
            bool sleep;
            if(deepSleepCounter==0)
            {
                if(sleepingList.empty()==false)
                {
                    long long wakeup=sleepingList.front()->wakeupTime;
                    sleep=!IRQdeepSleep(wakeup);
                } else sleep=!IRQdeepSleep();
            } else sleep=true;
            //NOTE: going to sleep with interrupts disabled makes sure no
            //preemption occurs from when we take the decision to sleep till
            //we actually do sleep. Wakeup interrupt will be run when we enable
            //back interrupts
            if(sleep) sleepCpu();
        }
        #else //WITH_DEEP_SLEEP
        sleepCpu();
        #endif //WITH_DEEP_SLEEP
        #endif //WITH_SLEEP
    }
}

#ifdef WITH_SMP
/**
 * \internal
 * Idle thread for cores other than the core 0, does even less
 */
void *idleThreadOtherCores(void *)
{
    for(;;)
    {
        #ifdef WITH_SLEEP
        sleepCpu();
        #endif //WITH_SLEEP
    }
}
#endif

/**
 * \internal
 * Start the kernel.<br> There is no way to stop the kernel once it is
 * started, except a (software or hardware) system reset.<br>
 * Calls errorHandler(OUT_OF_MEMORY) if there is no heap to create the idle
 * thread. If the function succeds in starting the kernel, it never returns;
 * otherwise it will call errorHandler(OUT_OF_MEMORY) and then return
 * immediately. IRQstartKernel() must not be called when the kernel is already
 * started.
 */
void IRQstartKernel()
{
    #ifdef WITH_PROCESSES
    try {
        kernel=new ProcessBase;
    } catch(...) {
        errorHandler(OUT_OF_MEMORY);
    }
    #endif //WITH_PROCESSES
    
    // As a side effect this function allocates the first idle thread and makes
    // runningThreads[0] point to it. It's probably been called at least once
    // during boot by the time we get here, but we can't be sure
    auto *idle=Thread::IRQgetCurrentThread();
    
    #ifdef WITH_PROCESSES
    // If the idle thread was allocated before IRQstartKernel(), then its proc
    // is nullptr. We can't move kernel=new ProcessBase; earlier than this
    // function, though
    idle->proc=kernel;
    #endif //WITH_PROCESSES

    // Create the main thread and add it to the scheduler.
    Thread *main;
    main=Thread::doCreate(mainLoader,MAIN_STACK_SIZE,nullptr,Thread::DEFAULT,true);
    if(main==nullptr) errorHandler(OUT_OF_MEMORY);
    if(Scheduler::IRQaddThread(main,MAIN_PRIORITY)==false) errorHandler(UNEXPECTED);

    // Idle thread needs to be set after main (see control_scheduler.cpp)
    Scheduler::IRQsetIdleThread(0,idle);

    // On SMP platforms, create and set idle threads for all other cores, and
    // prepare the array of core main functions.
    #ifdef WITH_SMP
    void *coreBootStacks[CPU_NUM_CORES-1];
    void (*coreBootEntryPoints[CPU_NUM_CORES-1])();
    for(int i=1;i<CPU_NUM_CORES;i++)
    {
        // Create idle thread and set it as running
        idle=Thread::doCreate(idleThreadOtherCores,STACK_IDLE,nullptr,Thread::DEFAULT,true);
        if(idle==nullptr) errorHandler(OUT_OF_MEMORY);
        Scheduler::IRQsetIdleThread(i,idle);
        runningThreads[i]=idle;
        // Prepare initial stack and entry point. The core entry point does
        // nothing but an initial context switch, so we re-use the idle thread
        // stack to avoid allocating a temporary stack. The -CTXSAVE_ON_STACK is
        // important to prevent the core setup code from corrupting the idle
        // thread context prepared on the stack by Thread::doCreate.
        coreBootStacks[i-1]=reinterpret_cast<unsigned char*>(idle)-CTXSAVE_ON_STACK;
        coreBootEntryPoints[i-1]=&IRQportableStartKernel;
    }
    #endif //WITH_SMP
    
    // Make the C standard library use per-thread reeentrancy structure
    setCReentrancyCallback(Thread::getCReent);
    
    // Boot the other cores, and this core.
    kernelStarted=true;
    #ifdef WITH_SMP
    IRQinitSMP(coreBootStacks,coreBootEntryPoints);
    #endif //WITH_SMP
    IRQportableStartKernel();
}

//These are not implemented here, but in the platform/board-specific os_timer.
//long long getTime() noexcept
//long long IRQgetTime() noexcept

/**
 * \internal
 * Used by Thread::sleep() and pthread_cond_timedwait() to add a thread to
 * sleeping list. The list is sorted by the wakeupTime field to reduce time
 * required to wake threads during context switch.
 * Interrupts must be disabled prior to calling this function.
 */
static void IRQaddToSleepingList(SleepData *x)
{
    if(sleepingList.empty() || sleepingList.front()->wakeupTime>=x->wakeupTime)
    {
        sleepingList.push_front(x);
    } else {
        auto it=sleepingList.begin();
        while(it!=sleepingList.end() && (*it)->wakeupTime<x->wakeupTime) ++it;
        sleepingList.insert(it,x);
    }
}

/**
 * \internal
 * This is the OS timer interrupt for WAKEUP_HANDLING_CORE, the only core that
 * is assigned the task to handle the wakeup of sleeping threads. The OS timer
 * is set aperiodically by the scheduler to both wake sleeping threads and to
 * handle preemption on that core.
 * \warning currentTime cannot be earlier than the last timer interrupt actually
 * programmed by the scheduler!
 */
void IRQwakeThreads(long long currentTime)
{
    // Condition
    // A woken higher priority thread than running on another core
    // B woken higher priority thread than running on WAKEUP_HANDLING_CORE
    // C time to preempt task on WAKEUP_HANDLING_CORE
    // Truth table
    // A B C
    // 0 0 0 osTimerSetInterrupt(min(firstWakeup,nextPreempt))
    // 0 0 1 invokeScheduler
    // 0 1 0 invokeScheduler
    // 0 1 1 invokeScheduler
    // 1 0 0 invokeSchedulerOnCore + osTimerSetInterrupt(min(firstWakeup,nextPreempt))
    // 1 0 1 invokeSchedulerOnCore + invokeScheduler
    // 1 1 0 invokeSchedulerOnCore + invokeScheduler
    // 1 1 1 invokeSchedulerOnCore + invokeScheduler
    bool hptw=false;
    for(auto it=sleepingList.begin();it!=sleepingList.end();)
    {
        // List is sorted, if we don't need to wake one element, we don't need
        // to wake the other too
        if(currentTime<(*it)->wakeupTime) break;
        // Wake both threads doing absoluteSleep() and timedWait()
        Thread *t=(*it)->thread;
        t->flags.IRQclearSleepAndWait(t);
        auto wokenPrio=t->IRQgetPriority();
        // Heuristic load balancing: threads waking from sleep get preferentially
        // allocated to higher core numbers
        for(int i=CPU_NUM_CORES-1;i>=0;i--)
        {
            if(const_cast<Thread*>(runningThreads[i])->IRQgetPriority()<wokenPrio)
            {
                if(i==WAKEUP_HANDLING_CORE) hptw=true;
                else IRQinvokeSchedulerOnCore(i);
                break;
            }
        }
        it=sleepingList.erase(it);
    }
    if(hptw) IRQinvokeScheduler();
    else {
        long long nextPreempt=Scheduler::IRQgetNextPreemption();
        if(currentTime>=nextPreempt) IRQinvokeScheduler();
        else {
            long long firstWakeup;
            if(sleepingList.empty()) firstWakeup=numeric_limits<long long>::max();
            else firstWakeup=sleepingList.front()->wakeupTime;
            IRQosTimerSetInterrupt(min(firstWakeup,nextPreempt));
        }
    }
}

/*
Memory layout for a thread
    |------------------------|
    |     class Thread       |
    |------------------------|<-- this
    |         stack          |
    |           |            |
    |           V            |
    |------------------------|
    |       watermark        |
    |------------------------|<-- base, watermark
*/

Thread *Thread::create(void *(*startfunc)(void *), unsigned int stacksize,
                       Priority priority, void *argv, unsigned short options)
{
    //Check to see if input parameters are valid
    if(priority.validate()==false || stacksize<STACK_MIN) return nullptr;
    
    Thread *thread=doCreate(startfunc,stacksize,argv,options,false);
    if(thread==nullptr) return nullptr;
    
    //Add thread to thread list
    bool result;
    {
        FastGlobalIrqLock lock;
        result=Scheduler::IRQaddThread(thread,priority);
        if(result)
        {
            // Heuristic load balancing: threads just created get preferentially
            // allocated to higher core numbers
            for(int i=CPU_NUM_CORES-1;i>=0;i--)
            {
                if(const_cast<Thread*>(runningThreads[i])->IRQgetPriority()<priority)
                {
                    IRQinvokeSchedulerOnCore(i);
                    break;
                }
            }
        }
    }
    if(result==false)
    {
        //Reached limit on number of threads
        unsigned int *base=thread->watermark;
        thread->~Thread();
        free(base); //Delete ALL thread memory
        return nullptr;
    }
    return thread;
}

Thread *Thread::create(void (*startfunc)(void *), unsigned int stacksize,
                       Priority priority, void *argv, unsigned short options)
{
    //Just call the other version with a cast.
    return Thread::create(reinterpret_cast<void *(*)(void*)>(startfunc),
            stacksize,priority,argv,options);
}

void Thread::yield()
{
    // NOTE: IRQinvokeScheduler is currently safe to be called also without the
    // global lock. If this property changes, we'll need to take the lock
    IRQinvokeScheduler();
}

void Thread::sleep(unsigned int ms)
{
    nanoSleepUntil(getTime()+mul32x32to64(ms,1000000));
}

void Thread::nanoSleep(long long ns)
{
    nanoSleepUntil(getTime()+ns);
}

void Thread::nanoSleepUntil(long long absoluteTimeNs)
{
    //Disallow absolute sleeps with negative or too low values, as the ns2tick()
    //algorithm in TimeConversion can't handle negative values and may undeflow
    //even with very low values due to a negative adjustOffsetNs. As an unlikely
    //side effect, very short sleeps done very early at boot will be extended.
    absoluteTimeNs=max(absoluteTimeNs,100000LL);
    //pauseKernel() here is not enough since even if the kernel is stopped
    //the timer isr will wake threads, modifying the sleepingList
    {
        FastGlobalIrqLock dLock;
        SleepData d(const_cast<Thread*>(runningThreads[getCurrentCoreId()]),absoluteTimeNs);
        d.thread->flags.IRQsetSleep(d.thread); //Sleeping thread: set sleep flag
        IRQaddToSleepingList(&d);
        {
            FastGlobalIrqUnlock eLock(dLock);
            Thread::yield();
        }
        //Only required for interruptibility when terminate is called
        sleepingList.removeFast(&d);
    }
}

void Thread::wait()
{
    //pausing the kernel is not enough because of IRQwait and IRQwakeup
    {
        FastGlobalIrqLock lock;
        Thread *cur=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
        cur->flags.IRQsetWait(cur,true);
    }
    Thread::yield();
    //Return here after wakeup
}

void Thread::PKrestartKernelAndWait(PauseKernelLock& dLock)
{
    // WARNING: The implementation of this function must remain synchronized
    // with its IRQ-based counterpart (IRQglobalIrqUnlockAndWaitImpl)
    (void)dLock;
    fastDisableIrq();

    // Put the thread the sleep. We could get the current thread by calling
    // Thread::IRQgetCurrentThread but there is logic there that we want to
    // avoid.
    Thread *cur=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
    fastGlobalLockFromIrq();
    cur->flags.IRQsetWait(cur,true);
    fastGlobalUnlockFromIrq();
    
    // Save Pk lock state, yield, and restore lock state.
    // We do not save/restore the state of the GIL because it's not taken
    // (if it was, somebody is violating the constraints on when a PK function
    // can be called).
    auto savedPkNesting=irqDisabledRestartKernelForce(); // this must be nonzero!
    fastEnableIrq();
    Thread::yield(); //Here the wait becomes effective
    pauseKernelForceToDepth(savedPkNesting);
}

TimedWaitResult Thread::PKrestartKernelAndTimedWait(PauseKernelLock& dLock,
        long long absoluteTimeNs)
{
    // WARNING: The implementation of this function must remain synchronized
    // with its IRQ-based counterpart (IRQglobalIrqUnlockAndTimedWaitImpl)
    (void)dLock;
    fastDisableIrq();

    // Put the thread to sleep.
    absoluteTimeNs=max(absoluteTimeNs,100000LL);
    Thread *t=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
    SleepData sleepData(t,absoluteTimeNs);
    fastGlobalLockFromIrq();
    t->flags.IRQsetWait(t,true); //timedWait thread: set wait flag
    IRQaddToSleepingList(&sleepData);
    fastGlobalUnlockFromIrq();

    // Save Pk lock state, yield, and restore lock state.
    // We do not save/restore the state of the GIL because it's not taken
    // (if it was, somebody is violating the constraints on when a PK function
    // can be called)
    auto savedPkNesting=irqDisabledRestartKernelForce(); // this must be nonzero!
    fastEnableIrq();
    Thread::yield(); //Here the wait becomes effective
    fastDisableIrq();
    irqDisabledPauseKernelForceToDepth(savedPkNesting);

    // Remove us from the sleeping list and check how we were woken up.
    // If the thread was still in the sleeping list, it was woken up by a wakeup()
    fastGlobalLockFromIrq();
    auto removed=sleepingList.removeFast(&sleepData);
    fastGlobalUnlockFromIrq();
    fastEnableIrq();
    return removed ? TimedWaitResult::NoTimeout : TimedWaitResult::Timeout;
}

void Thread::PKwakeup()
{
    //pausing the kernel is not enough because of IRQwait and IRQwakeup
    //DO NOT refactor this code by calling IRQwakeup() as IRQwakeup can cause
    //the scheduler interrupt to be called on the current core
    FastGlobalIrqLock lock;
    this->flags.IRQsetWait(this,false);
    auto wokenPrio=this->IRQgetPriority();
    int coreId=getCurrentCoreId();
    // Heuristic load balancing: threads waking from mutexes get preferentially
    // allocated to lower core numbers. Also, check first the cores that are
    // not the one that took the lock
    for(int i=0;i<CPU_NUM_CORES;i++)
    {
        if(i!=coreId &&
           const_cast<Thread*>(runningThreads[i])->IRQgetPriority()<wokenPrio)
        {
            IRQinvokeSchedulerOnCore(i);
            return;
        }
    }
    // If we get here all the cores that did not take the lock don't run lower
    // priority threads. Check the core that is taking the lock
    if(const_cast<Thread*>(runningThreads[coreId])->IRQgetPriority()<wokenPrio)
        pendingWakeup=true;
}

void Thread::IRQwakeup()
{
    this->flags.IRQsetWait(this,false);
    auto wokenPrio=this->IRQgetPriority();
    // Heuristic load balancing: threads waking from I/O get preferentially
    // allocated to lower core numbers
    for(int i=0;i<CPU_NUM_CORES;i++)
    {
        if(const_cast<Thread*>(runningThreads[i])->IRQgetPriority()<wokenPrio)
        {
            IRQinvokeSchedulerOnCore(i);
            return;
        }
    }
}

Thread *Thread::IRQgetCurrentThread()
{
    //NOTE: this code is currently safe to be called either with interrupt
    //enabed or not, and with the kernel paused or not, as well as before the
    //kernel is started, so getCurrentThread() and PKgetCurrentThread() all
    //directly call here. If introducing changes that break this property, these
    //three functions may need to be split

    //Implementation is the same as getCurrentThread, but to keep a consistent
    //interface this method is duplicated
    Thread *result=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
    if(result) return result;
    //This function must always return a pointer to a valid thread. The first
    //time this is called before the kernel is started, however, runningThreads
    //is nullptr, thus we allocate the idle thread and return a pointer to that.
    return allocateIdleThread();
}

bool Thread::exists(Thread *p)
{
    if(p==nullptr) return false;
    GlobalIrqLock lock;
    return Scheduler::IRQexists(p);
}

Priority Thread::getPriority()
{
    //NOTE: the code in all schedulers is currently safe to be called either
    //with interrupt enabed or not, and with the kernel paused or not, so
    //PKgetPriority() and IRQgetPriority() directly call here. If introducing
    //changes that break this property, these functions may need to be split
    return Scheduler::getPriority(this);
}

void Thread::setPriority(Priority pr)
{
    if(pr.validate()==false) return;
    {
        GlobalIrqLock lock;

        Thread *running=IRQgetCurrentThread();
        //If thread is locking at least one mutex
        if(running->mutexLocked!=nullptr)
        {
            //savedPriority always changes, since when all mutexes are unlocked
            //setPriority() must become effective
            if(running->savedPriority==pr) return;
            running->savedPriority=pr;
            //Calculate new priority of thread, which is
            //max(savedPriority, inheritedPriority)
            Mutex *walk=running->mutexLocked;
            while(walk!=nullptr)
            {
                if(walk->waiting.empty()==false)
                    pr=max(pr,walk->waiting.front()->IRQgetPriority());
                walk=walk->next;
            }
        }

        //If old priority == desired priority, nothing to do.
        if(pr==running->IRQgetPriority()) return;
        Scheduler::IRQsetPriority(running,pr);
    }
    #ifdef SCHED_TYPE_EDF
    yield(); //Another thread might have a closer deadline
    // TODO: the above yield, if executed with preemption disabled, causes the
    // whole overhead of handling the PendSV just to set pendingWakeup.
    // Do we care about optimizing this? Or it's infrequent enough we don't
    // care?
    #endif //SCHED_TYPE_EDF
}

void Thread::terminate()
{
    //doing a read-modify-write operation on this->status, so pauseKernel is
    //not enough, we need to disable interrupts
    FastGlobalIrqLock lock;
    if(this->flags.isDeleting()) return; //Prevent sleep interruption abuse
    this->flags.IRQsetDeleting();
    this->flags.IRQclearSleepAndWait(this); //Interruptibility
}

bool Thread::testTerminate()
{
    //Just reading, no need for critical section
    return const_cast<Thread*>(runningThreads[getCurrentCoreId()])->flags.isDeleting();
}

void Thread::detach()
{
    FastGlobalIrqLock lock;
    if(this!=Thread::IRQgetCurrentThread() && Thread::IRQexists(this)==false)
        return;
    this->flags.IRQsetDetached();
    
    //we detached a terminated thread, so its memory needs to be deallocated
    if(this->flags.isZombie()) atomicSwap(&existDeleted,1);

    //Corner case: detaching a thread, but somebody else already called join
    //on it. This makes join return false instead of deadlocking
    Thread *t=this->joinData.waitingForJoin;
    if(t!=nullptr)
    {
        //joinData is an union, so its content can be an invalid thread
        //this happens if detaching a thread that has already terminated
        if(this->flags.isZombie()==false)
        {
            //Wake thread, or it might sleep forever
            t->flags.IRQsetJoinWait(t,false);
        }
    }
}

bool Thread::isDetached() const
{
    return this->flags.isDetached();
}

bool Thread::join(void** result)
{
    {
        FastGlobalIrqLock dLock;
        Thread *cur=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
        if(this==cur) return false;
        if(Thread::IRQexists(this)==false) return false;
        if(this->flags.isDetached()) return false;
        if(this->flags.isZombie()==false)
        {
            //Another thread already called join on toJoin
            if(this->joinData.waitingForJoin!=nullptr) return false;

            this->joinData.waitingForJoin=cur;
            for(;;)
            {
                //Wait
                cur->flags.IRQsetJoinWait(cur,true);
                {
                    FastGlobalIrqUnlock eLock(dLock);
                    Thread::yield();
                }
                if(Thread::IRQexists(this)==false) return false;
                if(this->flags.isDetached()) return false;
                if(this->flags.isZombie()) break;
            }
        }
        //Thread deleted, complete join procedure
        //Setting detached flag will make isDeleted() return true,
        //so its memory can be deallocated
        this->flags.IRQsetDetached();
        if(result!=nullptr) *result=this->joinData.result;
    }
    //Since there is surely one dead thread, deallocate it immediately
    //to free its memory as soon as possible
    Scheduler::removeDeadThreads();
    return true;
}

const unsigned int *Thread::getStackBottom()
{
    return getCurrentThread()->watermark+(WATERMARK_LEN/sizeof(unsigned int));
}

int Thread::getStackSize()
{
    return getCurrentThread()->stacksize;
}

void Thread::IRQstackOverflowCheck()
{
    Thread *cur=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
    const unsigned int watermarkSize=WATERMARK_LEN/sizeof(unsigned int);
    #ifdef WITH_PROCESSES
    if(cur->flags.isInUserspace())
    {
        bool overflow=false;
        if(cur->userCtxsave[STACK_OFFSET_IN_CTXSAVE] <
            reinterpret_cast<unsigned int>(cur->userWatermark+watermarkSize))
            overflow=true;
        if(overflow==false)
            for(unsigned int i=0;i<watermarkSize;i++)
                if(cur->userWatermark[i]!=WATERMARK_FILL) overflow=true;
        if(overflow) IRQreportFault(FaultData(fault::STACKOVERFLOW));
    }
    #endif //WITH_PROCESSES
    if(cur->ctxsave[STACK_OFFSET_IN_CTXSAVE] <
        reinterpret_cast<unsigned int>(cur->watermark+watermarkSize))
        errorHandler(STACK_OVERFLOW);
    for(unsigned int i=0;i<watermarkSize;i++)
        if(cur->watermark[i]!=WATERMARK_FILL) errorHandler(STACK_OVERFLOW);
}

#ifdef WITH_PROCESSES

void Thread::IRQhandleSvc()
{
    Thread *cur=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
    if(cur->proc==kernel) errorHandler(UNEXPECTED);
    //We know it's not the kernel, so the cast is safe
    auto *proc=static_cast<Process*>(cur->proc);
    //Don't process syscall if fault happened, may return to userspace by mistake
    if(proc->fault.IRQfaultHappened()) return;

    //Note that it is required to use ctxsave and not cur->ctxsave because
    //at this time we do not know if the active context is user or kernel
    switch(static_cast<Syscall>(peekSyscallId(const_cast<unsigned int*>(::ctxsave))))
    {
        case Syscall::YIELD:
            //Yield syscall is handled here in the IRQ by calling the scheduler
            Scheduler::IRQrunScheduler();
            return;
        case Syscall::USERSPACE:
            //Userspace syscall is handled here in the IRQ by switching to userspace
            cur->flags.IRQsetUserspace(true);
            ::ctxsave=cur->userCtxsave;
            proc->mpu.IRQenable();
            break;
        default:
            //All other syscalls are handled by switching to kernelspace
            cur->flags.IRQsetUserspace(false);
            ::ctxsave=cur->ctxsave;
            MPUConfiguration::IRQdisable();
            break;
    }
}

bool Thread::IRQreportFault(const FaultData& fault)
{
    Thread *cur=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
    if(cur->flags.isInUserspace()==false || cur->proc==kernel) return false;
    //We know it's not the kernel, so the cast is safe
    auto *proc=static_cast<Process*>(cur->proc);
    proc->fault=fault;
    proc->fault.IRQtryAddProgramCounter(cur->userCtxsave,proc->mpu);
    cur->flags.IRQsetUserspace(false);
    ::ctxsave=cur->ctxsave;
    MPUConfiguration::IRQdisable();
    return true;
}

SyscallParameters Thread::switchToUserspace()
{
    portableSwitchToUserspace();
    SyscallParameters result(runningThreads[getCurrentCoreId()]->userCtxsave);
    return result;
}

Thread *Thread::createUserspace(void *(*startfunc)(void *), Process *proc)
{
    Thread *thread=doCreate(startfunc,SYSTEM_MODE_PROCESS_STACK_SIZE,nullptr,
            Thread::DEFAULT,false);
    if(thread==nullptr) return nullptr;

    unsigned int *base=thread->watermark;
    try {
        thread->userCtxsave=new unsigned int[CTXSAVE_SIZE];
    } catch(bad_alloc&) {
        thread->~Thread();
        free(base); //Delete ALL thread memory
        return nullptr;//Error
    }
    
    thread->proc=proc;
    thread->flags.IRQsetWait(thread,true); //Thread is not yet ready
    
    //Add thread to thread list
    bool result;
    {
        FastGlobalIrqLock dLock;
        result=Scheduler::IRQaddThread(thread,MAIN_PRIORITY);
    }
    if(result==false)
    {
        //Reached limit on number of threads
        base=thread->watermark;
        thread->~Thread();
        free(base); //Delete ALL thread memory
        return nullptr;
    }

    return thread;
}

void Thread::setupUserspaceContext(unsigned int entry, int argc, void *argvSp,
    void *envp, unsigned int *gotBase, unsigned int stackSize)
{
    //Fill watermark and stack
    char *base=reinterpret_cast<char*>(argvSp)-stackSize-WATERMARK_LEN;
    memset(base, WATERMARK_FILL, WATERMARK_LEN);
    memset(base+WATERMARK_LEN, STACK_FILL, stackSize);
    Thread *cur=runningThreads[getCurrentCoreId()];
    cur->userWatermark=reinterpret_cast<unsigned int*>(base);
    //Initialize registers
    //NOTE: for the main thread in a process userWatermark is also the end of
    //the heap, used by _sbrk_r. When we'll implement threads in processes that
    //pointer will just point to the watermark end of the thread, but userspace
    //threads can just ignore that value so we'll pass it unconditionally
    initUserThreadCtxsave(cur->userCtxsave,entry,argc,argvSp,envp,
                          gotBase,runningThreads->userWatermark);
}

#endif //WITH_PROCESSES

Thread::Thread(unsigned int *watermark, unsigned int stacksize,
               bool defaultReent) : schedData(), savedPriority(0),
               mutexLocked(nullptr), mutexWaiting(nullptr), watermark(watermark),
               ctxsave(), stacksize(stacksize)
{
    joinData.waitingForJoin=nullptr;
    if(defaultReent) cReentrancyData=_GLOBAL_REENT;
    else {
        cReentrancyData=new _reent;
        if(cReentrancyData) _REENT_INIT_PTR(cReentrancyData);
    }
    #ifdef WITH_PROCESSES
    proc=kernel;
    userCtxsave=nullptr;
    #endif //WITH_PROCESSES
    #ifdef WITH_PTHREAD_KEYS
    memset(pthreadKeyValues,0,sizeof(pthreadKeyValues));
    #endif //WITH_PTHREAD_KEYS
}

Thread::~Thread()
{
    if(cReentrancyData && cReentrancyData!=_GLOBAL_REENT)
    {
        _reclaim_reent(cReentrancyData);
        delete cReentrancyData;
    }
    #ifdef WITH_PROCESSES
    if(userCtxsave) delete[] userCtxsave;
    #endif //WITH_PROCESSES
}

Thread *Thread::doCreate(void*(*startfunc)(void*) , unsigned int stacksize,
                      void* argv, unsigned short options, bool defaultReent)
{
    unsigned int fullStackSize=WATERMARK_LEN+CTXSAVE_ON_STACK+stacksize;

    //Align fullStackSize to the platform required stack alignment
    fullStackSize+=CTXSAVE_STACK_ALIGNMENT-1;
    fullStackSize/=CTXSAVE_STACK_ALIGNMENT;
    fullStackSize*=CTXSAVE_STACK_ALIGNMENT;

    //Allocate memory for the thread, return if fail
    unsigned int *base=static_cast<unsigned int*>(malloc(sizeof(Thread)+
            fullStackSize));
    if(base==nullptr) return nullptr;

    //At the top of thread memory allocate the Thread class with placement new
    void *threadClass=base+(fullStackSize/sizeof(unsigned int));
    Thread *thread=new (threadClass) Thread(base,stacksize,defaultReent);

    if(thread->cReentrancyData==nullptr)
    {
         thread->~Thread();
         free(base); //Delete ALL thread memory
         return nullptr;
    }

    //Fill watermark and stack
    memset(base, WATERMARK_FILL, WATERMARK_LEN);
    base+=WATERMARK_LEN/sizeof(unsigned int);
    memset(base, STACK_FILL, fullStackSize-WATERMARK_LEN);

    //On some architectures some registers are saved on the stack, therefore
    //initKernelThreadCtxsave *must* be called after filling the stack.
    initKernelThreadCtxsave(thread->ctxsave,&Thread::threadLauncher,
                            reinterpret_cast<unsigned int*>(thread),
                            startfunc,argv);

    if((options & JOINABLE)==0) thread->flags.IRQsetDetached();
    return thread;
}

void Thread::threadLauncher(void *(*threadfunc)(void*), void *argv)
{
    void *result=nullptr;
    #ifdef __NO_EXCEPTIONS
    result=threadfunc(argv);
    #else //__NO_EXCEPTIONS
    try {
        result=threadfunc(argv);
    #ifdef WITH_PTHREAD_EXIT
    } catch(PthreadExitException& e) {
        result=e.getReturnValue();
    #endif //WITH_PTHREAD_EXIT
    } catch(exception& e) {
        errorLog("***An exception propagated through a thread\n");
        errorLog("what():%s\n",e.what());
    } catch(...) {
        errorLog("***An exception propagated through a thread\n");
    }
    #endif //__NO_EXCEPTIONS
    //Thread returned from its entry point, so delete it

    Thread* cur=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
    #ifdef WITH_PTHREAD_KEYS
    callPthreadKeyDestructors(cur->pthreadKeyValues);
    #endif //WITH_PTHREAD_KEYS
    //Since the thread is running, it cannot be in the sleepingList, so no need
    //to remove it from the list
    {
        FastGlobalIrqLock lock;
        cur->flags.IRQsetDeleted(cur);

        if(cur->flags.isDetached()==false)
        {
            //If thread is joinable, handle join
            Thread *t=cur->joinData.waitingForJoin;
            if(t!=nullptr)
            {
                //Wake thread
                t->flags.IRQsetJoinWait(t,false);
            }
            //Set result
            cur->joinData.result=result;
        } else {
            //If thread is detached, memory can be deallocated immediately
            atomicSwap(&existDeleted,1);
        }
    }
    Thread::yield();//Since the thread is now deleted, yield immediately.
    //Will never reach here
    errorHandler(UNEXPECTED);
}

void Thread::IRQglobalIrqUnlockAndWaitImpl()
{
    // Put the thread the sleep. We could get the current thread by calling
    // Thread::IRQgetCurrentThread but there is logic there that we want to
    // avoid.
    Thread *cur=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
    cur->flags.IRQsetWait(cur,true);

    // Save GIL state, yield, and restore the state.
    // Note that we are not sure here whether we have taken the PK lock or not.
    // If the PK lock appears taken, it might be currently taken by another core
    // and as a result we cannot touch it!
    // So better to leave it alone. But as a side-effect we cannot upgrade a PK
    // lock to a GIL and then use this function!
    auto savedGILNesting=globalIrqForceUnlock();
    Thread::yield(); //Here the wait becomes effective
    if(savedGILNesting) globalIrqForceLockToDepth(savedGILNesting);
    else fastGlobalIrqLock(); //The GIL was taken using the fast primitives
}

TimedWaitResult Thread::IRQglobalIrqUnlockAndTimedWaitImpl(long long absoluteTimeNs)
{
    // Put the thread to sleep.
    absoluteTimeNs=max(absoluteTimeNs,100000LL);
    Thread *t=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
    SleepData sleepData(t,absoluteTimeNs);
    t->flags.IRQsetWait(t,true); //timedWait thread: set wait flag
    IRQaddToSleepingList(&sleepData);

    // Save GIL state, yield, and restore the state.
    auto savedGILNesting=globalIrqForceUnlock();
    Thread::yield(); //Here the wait becomes effective
    if(savedGILNesting) globalIrqForceLockToDepth(savedGILNesting);
    else fastGlobalIrqLock(); //The GIL was taken using the fast primitives

    // Remove us from the sleeping list and check how we were woken up.
    // If the thread was still in the sleeping list, it was woken up by a wakeup()
    bool removed=sleepingList.removeFast(&sleepData);
    return removed ? TimedWaitResult::NoTimeout : TimedWaitResult::Timeout;
}

bool Thread::IRQexists(Thread* p)
{
    if(p==nullptr) return false;
    return Scheduler::IRQexists(p);
}

Thread *Thread::allocateIdleThread()
{
    //NOTE: this function is only called once before the kernel is started, so
    //there are no concurrency issues, not even with interrupts

    // Create the idle and main thread
    auto *idle=Thread::doCreate(idleThreadCore0,STACK_IDLE,nullptr,Thread::DEFAULT,true);
    if(idle==nullptr) errorHandler(OUT_OF_MEMORY);

    // runningThreads[0] must point to a valid thread, so we make it point
    // to the the idle one. Moreover, we must be on core 0 during boot
    if(getCurrentCoreId()!=0) errorHandler(UNEXPECTED);
    runningThreads[0]=idle;
    return idle;
}

struct _reent *Thread::getCReent()
{
    return getCurrentThread()->cReentrancyData;
}

//
// class ThreadFlags
//

void Thread::ThreadFlags::IRQsetWait(Thread *self, bool waiting)
{
    bool wasReady=isReady();
    if(waiting) flags |= WAIT;
    else {
        flags &= ~WAIT;
        if(wasReady==false && isReady()) Scheduler::IRQwokenThread(self);
    }
    Scheduler::IRQwaitStatusHook(self);
}

void Thread::ThreadFlags::IRQsetSleep(Thread *self)
{
    flags |= SLEEP;
    Scheduler::IRQwaitStatusHook(self);
}

void Thread::ThreadFlags::IRQclearSleepAndWait(Thread *self)
{
    bool wasReady=isReady();
    flags &= ~(WAIT | SLEEP);
    if(wasReady==false && isReady()) Scheduler::IRQwokenThread(self);
    Scheduler::IRQwaitStatusHook(self);
}

void Thread::ThreadFlags::IRQsetJoinWait(Thread *self, bool waiting)
{
    bool wasReady=isReady();
    if(waiting) flags |= WAIT_JOIN;
    else {
        flags &= ~WAIT_JOIN;
        if(wasReady==false && isReady()) Scheduler::IRQwokenThread(self);
    }
    Scheduler::IRQwaitStatusHook(self);
}

void Thread::ThreadFlags::IRQsetDeleted(Thread *self)
{
    flags |= DELETED;
    Scheduler::IRQwaitStatusHook(self);
}

} //namespace miosix
