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

#include "kernel.h"
#include "interfaces/portability.h"
#include "error.h"
#include "logging.h"
#include "arch_settings.h"
#include "sync.h"
#include "kernel/scheduler/scheduler.h"
#include <stdexcept>
#include <algorithm>
#include <string.h>

/*
Used by assembler context switch macros
This variable is set by miosix::IRQfindNextThread in file kernel.cpp
*/
extern "C" {
volatile unsigned int *ctxsave;
}


namespace miosix {

//Global variables used by kernel.cpp. Those that are not static are also used
//in portability.cpp and by the schedulers.
//These variables MUST NOT be used outside kernel.cpp and portability.cpp

volatile Thread *cur=NULL;///<\internal Thread currently running

///\internal True if there are threads in the DELETED status. Used by idle thread
static volatile bool exist_deleted=false;

static SleepData *sleeping_list=NULL;///<\internal list of sleeping threads

static volatile long long tick=0;///<\internal Kernel tick

///\internal !=0 after pauseKernel(), ==0 after restartKernel()
unsigned char kernel_running=0;

///\internal true if a tick occurs while the kernel is paused
volatile bool tick_skew=false;

static bool kernel_started=false;///<\internal becomes true after startKernel.

/// This is used by disableInterrupts() and enableInterrupts() to allow nested
/// calls to these functions.
static unsigned char interruptDisableNesting=0;

/**
 * \internal
 * Idle thread. Created when the kernel is started, it phisically deallocates
 * memory for deleted threads, and puts the cpu in sleep mode.
 */
void *idleThread(void *argv)
{
    for(;;)
    {
        if(exist_deleted)
        {
            PauseKernelLock lock;
            exist_deleted=false;
            Scheduler::PKremoveDeadThreads();
        }
        #ifndef JTAG_DISABLE_SLEEP
        //JTAG debuggers lose communication with the device if it enters sleep
        //mode, so to use debugging it is necessary to remove this instruction
        miosix_private::sleepCpu();
        #endif
    }
    return 0; //Just to avoid a compiler warning
}

void disableInterrupts()
{
    //Before the kernel is started interrupts are disabled,
    //so disabling them again won't hurt
    miosix_private::doDisableInterrupts();
    if(interruptDisableNesting==0xff) errorHandler(NESTING_OVERFLOW);
    interruptDisableNesting++;
}

void enableInterrupts()
{
    if(interruptDisableNesting==0)
    {
        //Bad, enableInterrupts was called one time more than disableInterrupts
        errorHandler(DISABLE_INTERRUPTS_NESTING);
    }
    interruptDisableNesting--;
    if(interruptDisableNesting==0 && kernel_started==true)
    {
        miosix_private::doEnableInterrupts();
    }
}

void pauseKernel()
{
    miosix_private::doDisableInterrupts();
    if(kernel_running==0xff) errorHandler(NESTING_OVERFLOW);
    kernel_running++;
    if(kernel_started==true) miosix_private::doEnableInterrupts();
}

void restartKernel()
{
    miosix_private::doDisableInterrupts();
    if(kernel_running==0)
    {
        //Bad, restartKernel was called one time more than pauseKernel
        errorHandler(PAUSE_KERNEL_NESTING);
    }
    kernel_running--;
    if(kernel_started==true) miosix_private::doEnableInterrupts();
    
    if((kernel_running==0)&&tick_skew)//If we missed some tick yield immediately
    { 
        tick_skew=false;
        Thread::yield();
    }
}

bool areInterruptsEnabled()
{
    return miosix_private::checkAreInterruptsEnabled();
}

void startKernel()
{
    //
    //Create the idle thread
    //
    unsigned int *base=static_cast<unsigned int*>(malloc(sizeof(Thread)+
            STACK_IDLE+CTXSAVE_ON_STACK+WATERMARK_LEN));
    if(base==NULL)
    {
        errorHandler(OUT_OF_MEMORY);
        return;//Error
    }
    //At the top of thread memory allocate the Thread class with placement new
    void *threadClass=base+((STACK_IDLE+CTXSAVE_ON_STACK+WATERMARK_LEN)/
            sizeof(unsigned int));
    Thread *idle=new (threadClass) Thread(base,STACK_IDLE);

    //Fill watermark and stack
    memset(base, WATERMARK_FILL, WATERMARK_LEN);
    base+=WATERMARK_LEN/sizeof(unsigned int);
    memset(base, STACK_FILL, STACK_IDLE);

    //On some architectures some registers are saved on the stack, therefore
    //initCtxsave *must* be called after filling the stack.
    miosix_private::initCtxsave(idle->ctxsave,idleThread,
            reinterpret_cast<unsigned int*>(idle),NULL);
    idle->flags.IRQsetDetached();
    Scheduler::IRQsetIdleThread(idle);
    kernel_started=true;//Now kernel is started

    //Now that the tick timer is running, start first thread
    //cur must point to a valid thread, but now, since the kernel isn't started
    //there's no current thread. So we make it point to the the idle thread.
    cur=idle;
    
    //
    //Dispatch the task to the architecture-specific function
    //
    miosix_private::IRQportableStartKernel();
}

bool isKernelRunning()
{
    return (kernel_running==0) && kernel_started;
}

long long getTick()
{
    /*
     * Reading a volatile 64bit integer on a 32bit platform with interrupts
     * enabled is tricky because the operation is not atomic, so we need to
     * read it twice to see if we were interrupted in the middle of the read
     * operation.
     */
    long long a,b;
    for(;;)
    {
        a=static_cast<long long>(tick);
        b=static_cast<long long>(tick);
        if(a==b) return a;
    }
}

/**
 * \internal
 * Used by Thread::sleep() to add a thread to sleeping list. The list is sorted
 * by the wakeup_time field to reduce time required to wake threads during
 * context switch.
 * Also sets thread SLEEP_FLAG. It is labeled IRQ not because it is meant to be
 * used inside an IRQ, but because interrupts must be disabled prior to calling
 * this function.
 */
void IRQaddToSleepingList(SleepData *x)
{
    x->p->flags.IRQsetSleep(true);
    if((sleeping_list==NULL)||(x->wakeup_time <= sleeping_list->wakeup_time))
    {
        x->next=sleeping_list;
        sleeping_list=x;   
    } else {
       SleepData *cur=sleeping_list;
       for(;;)
       {
           if((cur->next==NULL)||(x->wakeup_time <= cur->next->wakeup_time))
           {
               x->next=cur->next;
               cur->next=x;
               break;
           }
           cur=cur->next;
       }
    }
}

/**
 * \internal
 * Called @ every tick to check if it's time to wake some thread.
 * Also increases the system tick.
 * Takes care of clearing SLEEP_FLAG.
 * It is used by the kernel, and should not be used by end users.
 * \return true if some thread was woken.
 */
bool IRQwakeThreads()
{
    tick++;//Increment tick
    bool result=false;
    for(;;)
    {
        if(sleeping_list==NULL) break;//If no item in list, return
        //Since list is sorted, if we don't need to wake the first element
        //we don't need to wake the other too
        if(tick != sleeping_list->wakeup_time) break;
        sleeping_list->p->flags.IRQsetSleep(false);//Wake thread
        sleeping_list=sleeping_list->next;//Remove from list
        result=true;
    }
    return result;
}

/*
Memory layout for a thread
	|------------------------|
	|     class Thread       |
	|------------------------|<-- proc, this
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
    if(priority.validate()==false || stacksize<STACK_MIN)
    {
        errorHandler(INVALID_PARAMETERS);
        return NULL;
    }
    //If stacksize is not divisible by 4, round it to a number divisible by 4
    stacksize &= ~0x3;
    //Allocate memory for the thread, return if fail
    unsigned int *base=static_cast<unsigned int*>(malloc(sizeof(Thread)+
            stacksize+WATERMARK_LEN+CTXSAVE_ON_STACK));
    if(base==NULL)
    {
        errorHandler(OUT_OF_MEMORY);
        return NULL;//Error
    }
    //At the top of thread memory allocate the Thread class with placement new
    void *threadClass=base+((stacksize+WATERMARK_LEN+CTXSAVE_ON_STACK)/
            sizeof(unsigned int));
    Thread *thread=new (threadClass) Thread(base,stacksize);

    //Fill watermark and stack
    memset(base, WATERMARK_FILL, WATERMARK_LEN);
    base+=WATERMARK_LEN/sizeof(unsigned int);
    memset(base, STACK_FILL, stacksize);

    //On some architectures some registers are saved on the stack, therefore
    //initCtxsave *must* be called after filling the stack.
    miosix_private::initCtxsave(thread->ctxsave,startfunc,
            reinterpret_cast<unsigned int*>(thread),argv);

    if((options & JOINABLE)==0) thread->flags.IRQsetDetached();
    
    //Add thread to thread list
    {
        //Handling the list of threads, critical section is required
        PauseKernelLock lock;
        if(Scheduler::PKaddThread(thread,priority)==false)
        {
            //Reached limit on number of threads
            base=thread->watermark;
            thread->~Thread();
            free(base); //Delete ALL thread memory
            return NULL;
        }
    }
    #ifdef SCHED_TYPE_EDF
    if(isKernelRunning()) yield(); //The new thread might have a closer deadline
    #endif //SCHED_TYPE_EDF
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
    miosix_private::doYield();
}

bool Thread::testTerminate()
{
    //Just reading, no need for critical section
    return const_cast<Thread*>(cur)->flags.isDeleting();
}
	
void Thread::sleep(unsigned int ms)
{
    if(ms==0) return;
    //pauseKernel() here is not enough since even if the kernel is stopped
    //the tick isr will wake threads, modifying the sleeping_list
    {
        FastInterruptDisableLock lock;
        SleepData d;
        d.p=const_cast<Thread*>(cur);
        if(((ms*TICK_FREQ)/1000)>0) d.wakeup_time=getTick()+(ms*TICK_FREQ)/1000;
        //If tick resolution is too low, wait one tick
        else d.wakeup_time=getTick()+1;
        IRQaddToSleepingList(&d);//Also sets SLEEP_FLAG
    }
    Thread::yield();
}

void Thread::sleepUntil(long long absoluteTime)
{
    //pauseKernel() here is not enough since even if the kernel is stopped
    //the tick isr will wake threads, modifying the sleeping_list
    {
        FastInterruptDisableLock lock;
        if(absoluteTime<=getTick()) return; //Wakeup time in the past, return
        SleepData d;
        d.p=const_cast<Thread*>(cur);
        d.wakeup_time=absoluteTime;
        IRQaddToSleepingList(&d);//Also sets SLEEP_FLAG
    }
    Thread::yield();
}

Thread *Thread::getCurrentThread()
{
    Thread *result=const_cast<Thread*>(cur);
    return result;
}

bool Thread::exists(Thread *p)
{
    if(p==NULL) return false;
    PauseKernelLock lock;
    return Scheduler::PKexists(p);
}

Priority Thread::getPriority()
{
    return Scheduler::getPriority(this);
}

void Thread::setPriority(Priority pr)
{
    if(pr.validate()==false) errorHandler(INVALID_PARAMETERS);
    PauseKernelLock lock;

    Thread *current=getCurrentThread();
    //If thread is locking at least one mutex
    if(current->mutexLocked!=0)
    {   
        //savedPriority always changes, since when all mutexes are unlocked
        //setPriority() must become effective
        if(current->savedPriority==pr) return;
        current->savedPriority=pr;
        //Calculate new priority of thread, which is
        //max(savedPriority, inheritedPriority)
        Mutex *walk=current->mutexLocked;
        while(walk!=0)
        {
            if(walk->waiting.empty()==false)
                pr=std::max(pr,walk->waiting.front()->getPriority());
            walk=walk->next;
        }
    }
    
    //If old priority == desired priority, nothing to do.
    if(pr==current->getPriority()) return;
    Scheduler::PKsetPriority(current,pr);
    #ifdef SCHED_TYPE_EDF
    if(isKernelRunning()) yield(); //Another thread might have a closer deadline
    #endif //SCHED_TYPE_EDF
}

void Thread::terminate()
{
    //doing a read-modify-write operation on this->status, so pauseKernel is
    //not enough, we need to disable interrupts
    FastInterruptDisableLock lock;
    this->flags.IRQsetDeleting();
}

void Thread::wait()
{
    //pausing the kernel is not enough because of IRQwait and IRQwakeup
    {
        FastInterruptDisableLock lock;
        const_cast<Thread*>(cur)->flags.IRQsetWait(true);
    }
    Thread::yield();
    //Return here after wakeup
}
	
void Thread::wakeup()
{
    //pausing the kernel is not enough because of IRQwait and IRQwakeup
    {
        FastInterruptDisableLock lock;
        this->flags.IRQsetWait(false);
    }
    #ifdef SCHED_TYPE_EDF
    yield();//The other thread might have a closer deadline
    #endif //SCHED_TYPE_EDF
}

void Thread::PKwakeup()
{
    //pausing the kernel is not enough because of IRQwait and IRQwakeup
    FastInterruptDisableLock lock;
    this->flags.IRQsetWait(false);
}

void Thread::detach()
{
    FastInterruptDisableLock lock;
    this->flags.IRQsetDetached();
    
    //we detached a terminated thread, so its memory needs to be deallocated
    if(this->flags.isDeletedJoin()) exist_deleted=true;

    //Corner case: detaching a thread, but somebody else already called join
    //on it. This makes join return false instead of deadlocking
    if(this->joinData.waitingForJoin!=NULL)
    {
        //joinData is an union, so its content can be an invalid thread
        //this happens if detaching a thread that has already terminated
        if(this->flags.isDeletedJoin()==false)
        {
            //Wake thread, or it might sleep forever
            this->joinData.waitingForJoin->flags.IRQsetJoinWait(false);
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
        FastInterruptDisableLock dLock;
        if(this==Thread::IRQgetCurrentThread()) return false;
        if(Thread::IRQexists(this)==false) return false;
        if(this->flags.isDetached()) return false;
        if(this->flags.isDeletedJoin()==false)
        {
            //Another thread already called join on toJoin
            if(this->joinData.waitingForJoin!=NULL) return false;

            this->joinData.waitingForJoin=Thread::IRQgetCurrentThread();
            for(;;)
            {
                //Wait
                Thread::IRQgetCurrentThread()->flags.IRQsetJoinWait(true);
                {
                    FastInterruptEnableLock eLock(dLock);
                    Thread::yield();
                }
                if(Thread::IRQexists(this)==false) return false;
                if(this->flags.isDetached()) return false;
                if(this->flags.isDeletedJoin()) break;
            }
        }
        //Thread deleted, complete join procedure
        //Setting detached flag will make isDeleted() return true,
        //so its memory can be deallocated
        this->flags.IRQsetDetached();
        if(result!=NULL) *result=this->joinData.result;
    }
    {
        PauseKernelLock lock;
        //Since there is surely one dead thread, deallocate it immediately
        //to free its memory as soon as possible
        Scheduler::PKremoveDeadThreads();
    }
    return true;
}

Thread *Thread::IRQgetCurrentThread()
{
    //Implementation is the same as getCurrentThread, but to keep a consistent
    //interface this method is duplicated
    Thread *result=const_cast<Thread*>(cur);
    return result;
}

Priority Thread::IRQgetPriority()
{
    //Implementation is the same as getPriority, but to keep a consistent
    //interface this method is duplicated
    return Scheduler::IRQgetPriority(this);
}

void Thread::IRQwait()
{
    const_cast<Thread*>(cur)->flags.IRQsetWait(true);
}

void Thread::IRQwakeup()
{
    this->flags.IRQsetWait(false);
}

bool Thread::IRQexists(Thread* p)
{
    if(p==NULL) return false;
    return Scheduler::PKexists(p);
}

const unsigned int *Thread::getStackBottom()
{
    return cur->watermark+(WATERMARK_LEN/sizeof(unsigned int));
}

const int Thread::getStackSize()
{
    return cur->stacksize;
}

void Thread::threadLauncher(void *(*threadfunc)(void*), void *argv)
{
    void *result=0;
    #ifdef __NO_EXCEPTIONS
    result=threadfunc(argv);
    #else //__NO_EXCEPTIONS
    try {
        result=threadfunc(argv);
    } catch(std::exception& e)
    {
        errorHandler(PROPAGATED_EXCEPTION);
        errorLog("what():");
        errorLog(e.what());
        errorLog("\r\n");
    } catch(...)
    {
        errorHandler(PROPAGATED_EXCEPTION);
    }
    #endif //__NO_EXCEPTIONS
    //Thread returned from its entry point, so delete it

    //Since the thread is running, it cannot be in the sleeping_list, so no need
    //to remove it from the list
    {
        FastInterruptDisableLock lock;
        const_cast<Thread*>(cur)->flags.IRQsetDeleted();

        if(const_cast<Thread*>(cur)->flags.isDetached()==false)
        {
            //If thread is joinable, handle join
            if(cur->joinData.waitingForJoin!=NULL)
            {
                //Wake thread
                cur->joinData.waitingForJoin->flags.IRQsetJoinWait(false);
            }
            //Set result
            cur->joinData.result=result;
        } else {
            //If thread is detached, memory can be deallocated immediately
            exist_deleted=true;
        }
    }
    Thread::yield();//Since the thread is now deleted, yield immediately.
    //Will never reach here
    errorHandler(UNEXPECTED);
    
}

//
// class ThreadFlags
//

void Thread::ThreadFlags::IRQsetWait(bool waiting)
{
    if(waiting) flags |= WAIT; else flags &= ~WAIT;
    Scheduler::IRQwaitStatusHook();
}

void Thread::ThreadFlags::IRQsetJoinWait(bool waiting)
{
    if(waiting) flags |= WAIT_JOIN; else flags &= ~WAIT_JOIN;
    Scheduler::IRQwaitStatusHook();
}

void Thread::ThreadFlags::IRQsetCondWait(bool waiting)
{
    if(waiting) flags |= WAIT_COND; else flags &= ~WAIT_COND;
    Scheduler::IRQwaitStatusHook();
}

void Thread::ThreadFlags::IRQsetSleep(bool sleeping)
{
    if(sleeping) flags |= SLEEP; else flags &= ~SLEEP;
    Scheduler::IRQwaitStatusHook();
}

void Thread::ThreadFlags::IRQsetDeleted()
{
    flags |= DELETED;
    Scheduler::IRQwaitStatusHook();
}

}; //namespace miosix
