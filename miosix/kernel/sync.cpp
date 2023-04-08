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

#include "sync.h"
#include "kernel.h"
#include "kernel/scheduler/scheduler.h"
#include "error.h"
#include "pthread_private.h"
#include <algorithm>

using namespace std;

namespace miosix {

void IRQaddToSleepingList(SleepData *x);
void IRQremoveFromSleepingList(SleepData *x);

//
// class Mutex
//

Mutex::Mutex(Options opt): owner(nullptr), next(nullptr), waiting()
{
    recursiveDepth= opt==RECURSIVE ? 0 : -1;
}

void Mutex::PKlock(PauseKernelLock& dLock)
{
    Thread *p=Thread::getCurrentThread();
    if(owner==nullptr)
    {
        owner=p;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(owner->mutexLocked==nullptr) owner->savedPriority=owner->getPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the Thread::IRQwait() is
    //enclosed in a while(owner!=p) which is immeditely false.
    if(owner==p)
    {
        if(recursiveDepth>=0)
        {
            recursiveDepth++;
            return;
        } else errorHandler(MUTEX_DEADLOCK); //Bad, deadlock
    }

    //Add thread to mutex' waiting queue
    waiting.push_back(p);
    LowerPriority l;
    push_heap(waiting.begin(),waiting.end(),l);

    //Handle priority inheritance
    if(p->mutexWaiting!=nullptr) errorHandler(UNEXPECTED);
    p->mutexWaiting=this;
    if (owner->getPriority().mutexLessOp(p->getPriority()))
    {
        Thread *walk=owner;
        for(;;)
        {
            Scheduler::PKsetPriority(walk,p->getPriority());
            if(walk->mutexWaiting==nullptr) break;
            make_heap(walk->mutexWaiting->waiting.begin(),
                      walk->mutexWaiting->waiting.end(),l);
            walk=walk->mutexWaiting->owner;
        }
    }

    //The while is necessary because some other thread might call wakeup()
    //on this thread. So the thread can wakeup also for other reasons not
    //related to the mutex becoming free
    while(owner!=p)
    {
        //Wait can only be called with kernel started, while IRQwait can
        //only be called with interupts disabled, so that's why interrupts
        //are disabled
        {
            FastInterruptDisableLock l;
            Thread::IRQwait();//Return immediately
        }
        {
            RestartKernelLock eLock(dLock);
            //Now the IRQwait becomes effective
            Thread::yield();
        }
    }
}

void Mutex::PKlockToDepth(PauseKernelLock& dLock, unsigned int depth)
{
    Thread *p=Thread::getCurrentThread();
    if(owner==nullptr)
    {
        owner=p;
        if(recursiveDepth>=0) recursiveDepth=depth;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(owner->mutexLocked==nullptr) owner->savedPriority=owner->getPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the Thread::IRQwait() is
    //enclosed in a while(owner!=p) which is immeditely false.
    if(owner==p)
    {
        if(recursiveDepth>=0)
        {
            recursiveDepth=depth;
            return;
        } else errorHandler(MUTEX_DEADLOCK); //Bad, deadlock
    }

    //Add thread to mutex' waiting queue
    waiting.push_back(p);
    LowerPriority l;
    push_heap(waiting.begin(),waiting.end(),l);

    //Handle priority inheritance
    if(p->mutexWaiting!=nullptr) errorHandler(UNEXPECTED);
    p->mutexWaiting=this;
    if (owner->getPriority().mutexLessOp(p->getPriority()))
    {
        Thread *walk=owner;
        for(;;)
        {
            Scheduler::PKsetPriority(walk,p->getPriority());
            if(walk->mutexWaiting==nullptr) break;
            make_heap(walk->mutexWaiting->waiting.begin(),
                      walk->mutexWaiting->waiting.end(),l);
            walk=walk->mutexWaiting->owner;
        }
    }

    //The while is necessary because some other thread might call wakeup()
    //on this thread. So the thread can wakeup also for other reasons not
    //related to the mutex becoming free
    while(owner!=p)
    {
        //Wait can only be called with kernel started, while IRQwait can
        //only be called with interupts disabled, so that's why interrupts
        //are disabled
        {
            FastInterruptDisableLock l;
            Thread::IRQwait();//Return immediately
        }
        {
            RestartKernelLock eLock(dLock);
            //Now the IRQwait becomes effective
            Thread::yield();
        }
    }
    if(recursiveDepth>=0) recursiveDepth=depth;
}

bool Mutex::PKtryLock(PauseKernelLock& dLock)
{
    Thread *p=Thread::getCurrentThread();
    if(owner==nullptr)
    {
        owner=p;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(owner->mutexLocked==nullptr) owner->savedPriority=owner->getPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        return true;
    }
    if(owner==p && recursiveDepth>=0)
    {
        recursiveDepth++;
        return true;
    }
    return false;
}

bool Mutex::PKunlock(PauseKernelLock& dLock)
{
    Thread *p=Thread::getCurrentThread();
    if(owner!=p) return false;

    if(recursiveDepth>0)
    {
        recursiveDepth--;
        return false;
    }

    //Remove this mutex from the list of mutexes locked by the owner
    if(owner->mutexLocked==this)
    {
        owner->mutexLocked=owner->mutexLocked->next;
    } else {
        Mutex *walk=owner->mutexLocked;
        for(;;)
        {
            //this Mutex not in owner's list? impossible
            if(walk->next==nullptr) errorHandler(UNEXPECTED);
            if(walk->next==this)
            {
                walk->next=walk->next->next;
                break;
            }
            walk=walk->next;
        }
    }

    //Handle priority inheritance
    if(owner->mutexLocked==nullptr)
    {
        //Not locking any other mutex
        if(owner->savedPriority!=owner->getPriority())
            Scheduler::PKsetPriority(owner,owner->savedPriority);
    } else {
        Priority pr=owner->savedPriority;
        //Calculate new priority of thread, which is
        //max(savedPriority, inheritedPriority)
        Mutex *walk=owner->mutexLocked;
        while(walk!=nullptr)
        {
            if(walk->waiting.empty()==false)
                if (pr.mutexLessOp(walk->waiting.front()->getPriority()))
                    pr = walk->waiting.front()->getPriority();
            walk=walk->next;
        }
        if(pr!=owner->getPriority()) Scheduler::PKsetPriority(owner,pr);
    }

    //Choose next thread to lock the mutex
    if(waiting.empty()==false)
    {
        //There is at least another thread waiting
        owner=waiting.front();
        LowerPriority l;
        pop_heap(waiting.begin(),waiting.end(),l);
        waiting.pop_back();
        if(owner->mutexWaiting!=this) errorHandler(UNEXPECTED);
        owner->mutexWaiting=nullptr;
        owner->PKwakeup();
        if(owner->mutexLocked==nullptr) owner->savedPriority=owner->getPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        //Handle priority inheritance of new owner
        if(waiting.empty()==false &&
                owner->getPriority().mutexLessOp(waiting.front()->getPriority()))
                Scheduler::PKsetPriority(owner,waiting.front()->getPriority());
        return p->getPriority().mutexLessOp(owner->getPriority());
    } else {
        owner=nullptr; //No threads waiting
        std::vector<Thread *>().swap(waiting); //Save some RAM
        return false;
    }
}

unsigned int Mutex::PKunlockAllDepthLevels(PauseKernelLock& dLock)
{
    Thread *p=Thread::getCurrentThread();
    if(owner!=p) return 0;

    //Remove this mutex from the list of mutexes locked by the owner
    if(owner->mutexLocked==this)
    {
        owner->mutexLocked=owner->mutexLocked->next;
    } else {
        Mutex *walk=owner->mutexLocked;
        for(;;)
        {
            //this Mutex not in owner's list? impossible
            if(walk->next==nullptr) errorHandler(UNEXPECTED);
            if(walk->next==this)
            {
                walk->next=walk->next->next;
                break;
            }
            walk=walk->next;
        }
    }

    //Handle priority inheritance
    if(owner->mutexLocked==nullptr)
    {
        //Not locking any other mutex
        if(owner->savedPriority!=owner->getPriority())
            Scheduler::PKsetPriority(owner,owner->savedPriority);
    } else {
        Priority pr=owner->savedPriority;
        //Calculate new priority of thread, which is
        //max(savedPriority, inheritedPriority)
        Mutex *walk=owner->mutexLocked;
        while(walk!=nullptr)
        {
            if(walk->waiting.empty()==false)
                if (pr.mutexLessOp(walk->waiting.front()->getPriority()))
                    pr = walk->waiting.front()->getPriority();
            walk=walk->next;
        }
        if(pr!=owner->getPriority()) Scheduler::PKsetPriority(owner,pr);
    }

    //Choose next thread to lock the mutex
    if(waiting.empty()==false)
    {
        //There is at least another thread waiting
        owner=waiting.front();
        LowerPriority l;
        pop_heap(waiting.begin(),waiting.end(),l);
        waiting.pop_back();
        if(owner->mutexWaiting!=this) errorHandler(UNEXPECTED);
        owner->mutexWaiting=nullptr;
        owner->PKwakeup();
        if(owner->mutexLocked==nullptr) owner->savedPriority=owner->getPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        //Handle priority inheritance of new owner
        if(waiting.empty()==false &&
                owner->getPriority().mutexLessOp(waiting.front()->getPriority()))
                Scheduler::PKsetPriority(owner,waiting.front()->getPriority());
    } else {
        owner=nullptr; //No threads waiting
        std::vector<Thread *>().swap(waiting); //Save some RAM
    }
    
    if(recursiveDepth<0) return 0;
    unsigned int result=recursiveDepth;
    recursiveDepth=0;
    return result;
}

//
// class ConditionVariable
//

//Memory layout must be kept in sync with pthread_cond, see pthread.cpp
static_assert(sizeof(ConditionVariable)==sizeof(pthread_cond_t),"");

void ConditionVariable::wait(Mutex& m)
{
    PauseKernelLock dLock;
    Thread *t=Thread::getCurrentThread();
    CondData listItem(t);
    condList.push_back(&listItem); //Add entry to tail of list

    //Unlock mutex and wait
    {
        FastInterruptDisableLock l;
        t->flags.IRQsetCondWait(true);
    }

    unsigned int depth=m.PKunlockAllDepthLevels(dLock);
    {
        RestartKernelLock eLock(dLock);
        Thread::yield(); //Here the wait becomes effective
    }
    m.PKlockToDepth(dLock,depth);
}

void ConditionVariable::wait(pthread_mutex_t *m)
{
    FastInterruptDisableLock dLock;
    Thread *t=Thread::IRQgetCurrentThread();
    CondData listItem(t);
    condList.push_back(&listItem); //Putting this thread last on the list (lifo policy)
    t->flags.IRQsetCondWait(true);

    unsigned int depth=IRQdoMutexUnlockAllDepthLevels(m);
    {
        FastInterruptEnableLock eLock(dLock);
        Thread::yield(); //Here the wait becomes effective
    }
    IRQdoMutexLockToDepth(m,dLock,depth);
}

TimedWaitResult ConditionVariable::timedWait(Mutex& m, long long absTime)
{
    //Disallow absolute sleeps with negative or too low values, as the ns2tick()
    //algorithm in TimeConversion can't handle negative values and may undeflow
    //even with very low values due to a negative adjustOffsetNs. As an unlikely
    //side effect, very short sleeps done very early at boot will be extended.
    absTime=std::max(absTime,100000LL);

    PauseKernelLock dLock;
    Thread *t=Thread::getCurrentThread();
    CondData listItem(t);
    condList.push_back(&listItem); //Add entry to tail of list
    SleepData sleepData(t,absTime);
    {
        FastInterruptDisableLock l;
        IRQaddToSleepingList(&sleepData); //Putting this thread on the sleeping list too
        t->flags.IRQsetCondWait(true);
    }

    //Unlock mutex and wait
    unsigned int depth=m.PKunlockAllDepthLevels(dLock);
    {
        RestartKernelLock eLock(dLock);
        Thread::yield(); //Here the wait becomes effective
    }

    //Ensure that the thread is removed from both list, as it can be woken by
    //either a signal/broadcast (that removes it from condList) or by
    //IRQwakeThreads (that removes it from sleeping list).
    bool removed=condList.removeFast(&listItem);
    {
        FastInterruptDisableLock l;
        IRQremoveFromSleepingList(&sleepData);
    }

    m.PKlockToDepth(dLock,depth);

    //If the thread was still in the cond variable list, it was woken up by a timeout
    return removed ? TimedWaitResult::Timeout : TimedWaitResult::NoTimeout;
}

TimedWaitResult ConditionVariable::timedWait(pthread_mutex_t *m, long long absTime)
{
    //Disallow absolute sleeps with negative or too low values, as the ns2tick()
    //algorithm in TimeConversion can't handle negative values and may undeflow
    //even with very low values due to a negative adjustOffsetNs. As an unlikely
    //side effect, very short sleeps done very early at boot will be extended.
    absTime=std::max(absTime,100000LL);
    FastInterruptDisableLock dLock;
    Thread *t=Thread::IRQgetCurrentThread();
    CondData listItem(t);
    condList.push_back(&listItem); //Putting this thread last on the list (lifo policy)
    SleepData sleepData(t,absTime);
    IRQaddToSleepingList(&sleepData); //Putting this thread on the sleeping list too
    t->flags.IRQsetCondWait(true);

    unsigned int depth=IRQdoMutexUnlockAllDepthLevels(m);
    {
        FastInterruptEnableLock eLock(dLock);
        Thread::yield(); //Here the wait becomes effective
    }
    //Ensure that the thread is removed from both list, as it can be woken by
    //either a signal/broadcast (that removes it from condList) or by
    //IRQwakeThreads (that removes it from sleeping list).
    bool removed=condList.removeFast(&listItem);
    IRQremoveFromSleepingList(&sleepData);

    IRQdoMutexLockToDepth(m,dLock,depth);

    //If the thread was still in the cond variable list, it was woken up by a timeout
    return removed ? TimedWaitResult::Timeout : TimedWaitResult::NoTimeout;
}

void ConditionVariable::signal()
{
    bool hppw=false;
    {
        //Using interruptDisableLock because we need to call IRQsetCondWait
        //that can only be called with irq disabled, othrwise we would use
        //PauseKernelLock
        FastInterruptDisableLock lock;
        if(condList.empty()) return;
        //Remove from list and wakeup
        Thread *t=condList.front()->thread;
        condList.pop_front();
        t->flags.IRQsetCondWait(false);
        t->flags.IRQsetSleep(false); //Needed due to timedwait
        //Check for priority issues
        if(t->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
            hppw=true;
    }
    //If the woken thread has higher priority than our priority, yield
    if(hppw) Thread::yield();
}

void ConditionVariable::broadcast()
{
    bool hppw=false;
    {
        //Using interruptDisableLock because we need to call IRQsetCondWait
        //that can only be called with irq disabled, othrwise we would use
        //PauseKernelLock
        FastInterruptDisableLock lock;
        while(!condList.empty())
        {
            //Remove from list and wakeup
            Thread *t=condList.front()->thread;
            condList.pop_front();
            t->flags.IRQsetCondWait(false);
            t->flags.IRQsetSleep(false); //Needed due to timedwait
            //Check for priority issues
            if(t->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
                hppw=true;
        }
    }
    //If at least one of the woken thread has higher priority than our priority,
    //yield
    if(hppw) Thread::yield();
}

//
// class Semaphore
//

Thread *Semaphore::IRQsignalNoPreempt()
{
    //Check if somebody is waiting
    if(fifo.empty())
    {
        //Nobody there, just increment the counter
        count++;
        return nullptr;
    }
    CondData *cd=fifo.front();
    Thread *t=cd->thread;
    fifo.pop_front();
    t->flags.IRQsetCondWait(false);
    t->flags.IRQsetSleep(false); //Needed due to timedwait
    return t;
}

void Semaphore::IRQsignal()
{
    //Update the state of the FIFO and the counter
    Thread *t=IRQsignalNoPreempt();
    if(t==nullptr) return;
    //If the woken thread has higher priority trigger a reschedule
    if(Thread::IRQgetCurrentThread()->IRQgetPriority()<t->IRQgetPriority())
        Scheduler::IRQfindNextThread();
}

void Semaphore::signal()
{
    bool hppw=false;
    {
        //Global interrupt lock because Semaphore is IRQ-safe
        FastInterruptDisableLock dLock;
        //Update the state of the FIFO and the counter
        Thread *t=IRQsignalNoPreempt();
        if(t)
        {
            //If the woken thread has higher priority trigger a yield
            if(Thread::IRQgetCurrentThread()->IRQgetPriority()<t->IRQgetPriority())
                hppw=true;
        }
    }
    if(hppw) Thread::yield();
}

void Semaphore::wait()
{
    //Global interrupt lock because Semaphore is IRQ-safe
    FastInterruptDisableLock dLock;
    //If the counter is positive, decrement it and we're done
    if(count>0)
    {
        count--;
        return;
    }
    //Otherwise put ourselves in queue and wait
    Thread *t=Thread::getCurrentThread();
    CondData listItem(t);
    fifo.push_back(&listItem); //Add entry to tail of list
    t->flags.IRQsetCondWait(true);
    {
        FastInterruptEnableLock eLock(dLock);
        //The wait becomes effective here
        Thread::yield();
    }
}

TimedWaitResult Semaphore::timedWait(long long absTime)
{
    //Disallow absolute sleeps with negative or too low values, as the ns2tick()
    //algorithm in TimeConversion can't handle negative values and may undeflow
    //even with very low values due to a negative adjustOffsetNs. As an unlikely
    //side effect, very short sleeps done very early at boot will be extended.
    absTime=std::max(absTime,100000LL);

    //Global interrupt lock because Semaphore is IRQ-safe
    FastInterruptDisableLock dLock;
    //If the counter is positive, decrement it and we're done
    if(count>0)
    {
        count--;
        return TimedWaitResult::NoTimeout;
    }
    //Otherwise put ourselves in queue...
    Thread *t=Thread::getCurrentThread();
    CondData listItem(t);
    fifo.push_back(&listItem);
    //...and simultaneously to sleep
    SleepData sleepData(t,absTime);
    IRQaddToSleepingList(&sleepData);
    t->flags.IRQsetCondWait(true);
    {
        FastInterruptEnableLock eLock(dLock);
        //Wait/sleep becomes effective here
        Thread::yield();
    }
    
    //We got woken up by either the sleep or the wait. Ensure that the thread
    //is removed from both the wait list and the sleep list.
    bool removed=fifo.removeFast(&listItem);
    IRQremoveFromSleepingList(&sleepData);
    //If we were still in the fifo, we were woken up by a timeout
    return removed ? TimedWaitResult::Timeout : TimedWaitResult::NoTimeout;
}

} //namespace miosix
