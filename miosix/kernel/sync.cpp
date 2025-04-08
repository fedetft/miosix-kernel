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
#include "error.h"
#include "pthread_private.h"
#include "kernel/scheduler/scheduler.h"
#include <algorithm>

using namespace std;

namespace miosix {

/**
 * Helper lambda to sort threads in a min heap to implement priority inheritance
 * \param lhs first thread to compare
 * \param rhs second thread to compare
 * \return true if lhs->getPriority() < rhs->getPriority()
 */
static auto PKlowerPriority=[](Thread *lhs, Thread *rhs)
{
    return lhs->PKgetPriority().mutexLessOp(rhs->PKgetPriority());
};

//
// class FastMutex
//

FastMutex::FastMutex(Options opt)
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

//
// class Mutex
//

Mutex::Mutex(Options opt): owner(nullptr), next(nullptr), waiting()
{
    recursiveDepth= opt==RECURSIVE ? 0 : -1;
}

void Mutex::PKlock(PauseKernelLock& dLock)
{
    Thread *p=Thread::PKgetCurrentThread();
    if(owner==nullptr)
    {
        owner=p;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(owner->mutexLocked==nullptr) owner->savedPriority=owner->PKgetPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the wait is enclosed in a
    //while(owner!=p) which is immeditely false.
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
    push_heap(waiting.begin(),waiting.end(),PKlowerPriority);

    //Handle priority inheritance
    if(p->mutexWaiting!=nullptr) errorHandler(UNEXPECTED);
    p->mutexWaiting=this;
    if(owner->PKgetPriority().mutexLessOp(p->PKgetPriority()))
    {
        Thread *walk=owner;
        for(;;)
        {
            auto prio=p->PKgetPriority();
            {
                FastGlobalIrqLock irqLock;
                Scheduler::IRQsetPriority(walk,prio);
            }
            if(walk->mutexWaiting==nullptr) break;
            make_heap(walk->mutexWaiting->waiting.begin(),
                      walk->mutexWaiting->waiting.end(),PKlowerPriority);
            walk=walk->mutexWaiting->owner;
        }
    }

    //The while is necessary to protect against spurious wakeups
    while(owner!=p) Thread::PKrestartKernelAndWait(dLock);
}

void Mutex::PKlockToDepth(PauseKernelLock& dLock, unsigned int depth)
{
    Thread *p=Thread::PKgetCurrentThread();
    if(owner==nullptr)
    {
        owner=p;
        if(recursiveDepth>=0) recursiveDepth=depth;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(owner->mutexLocked==nullptr) owner->savedPriority=owner->PKgetPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the wait is enclosed in a
    //while(owner!=p) which is immeditely false.
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
    push_heap(waiting.begin(),waiting.end(),PKlowerPriority);

    //Handle priority inheritance
    if(p->mutexWaiting!=nullptr) errorHandler(UNEXPECTED);
    p->mutexWaiting=this;
    if(owner->PKgetPriority().mutexLessOp(p->PKgetPriority()))
    {
        Thread *walk=owner;
        for(;;)
        {
            auto prio=p->PKgetPriority();
            {
                FastGlobalIrqLock irqLock;
                Scheduler::IRQsetPriority(walk,prio);
            }
            if(walk->mutexWaiting==nullptr) break;
            make_heap(walk->mutexWaiting->waiting.begin(),
                      walk->mutexWaiting->waiting.end(),PKlowerPriority);
            walk=walk->mutexWaiting->owner;
        }
    }

    //The while is necessary to protect against spurious wakeups
    while(owner!=p) Thread::PKrestartKernelAndWait(dLock);
    if(recursiveDepth>=0) recursiveDepth=depth;
}

bool Mutex::PKtryLock(PauseKernelLock& dLock)
{
    Thread *p=Thread::PKgetCurrentThread();
    if(owner==nullptr)
    {
        owner=p;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(owner->mutexLocked==nullptr) owner->savedPriority=owner->PKgetPriority();
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
    Thread *p=Thread::PKgetCurrentThread();
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
        if(owner->savedPriority!=owner->PKgetPriority())
        {
            FastGlobalIrqLock irqLock;
            Scheduler::IRQsetPriority(owner,owner->savedPriority);
        }
    } else {
        Priority pr=owner->savedPriority;
        //Calculate new priority of thread, which is
        //max(savedPriority, inheritedPriority)
        Mutex *walk=owner->mutexLocked;
        while(walk!=nullptr)
        {
            if(walk->waiting.empty()==false)
                if(pr.mutexLessOp(walk->waiting.front()->PKgetPriority()))
                    pr=walk->waiting.front()->PKgetPriority();
            walk=walk->next;
        }
        if(pr!=owner->PKgetPriority())
        {
            FastGlobalIrqLock irqLock;
            Scheduler::IRQsetPriority(owner,pr);
        }
    }

    //Choose next thread to lock the mutex
    if(waiting.empty()==false)
    {
        //There is at least another thread waiting
        owner=waiting.front();
        pop_heap(waiting.begin(),waiting.end(),PKlowerPriority);
        waiting.pop_back();
        if(owner->mutexWaiting!=this) errorHandler(UNEXPECTED);
        owner->mutexWaiting=nullptr;
        owner->PKwakeup();
        if(owner->mutexLocked==nullptr) owner->savedPriority=owner->PKgetPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        //Handle priority inheritance of new owner
        if(waiting.empty()==false &&
                owner->PKgetPriority().mutexLessOp(waiting.front()->PKgetPriority()))
        {
            auto prio=waiting.front()->PKgetPriority();
            {
                FastGlobalIrqLock irqLock;
                Scheduler::IRQsetPriority(owner,prio);
            }
        }
        return p->PKgetPriority().mutexLessOp(owner->PKgetPriority());
    } else {
        owner=nullptr; //No threads waiting
        std::vector<Thread *>().swap(waiting); //Save some RAM
        return false;
    }
}

unsigned int Mutex::PKunlockAllDepthLevels(PauseKernelLock& dLock)
{
    Thread *p=Thread::PKgetCurrentThread();
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
        if(owner->savedPriority!=owner->PKgetPriority())
        {
            FastGlobalIrqLock irqLock;
            Scheduler::IRQsetPriority(owner,owner->savedPriority);
        }
    } else {
        Priority pr=owner->savedPriority;
        //Calculate new priority of thread, which is
        //max(savedPriority, inheritedPriority)
        Mutex *walk=owner->mutexLocked;
        while(walk!=nullptr)
        {
            if(walk->waiting.empty()==false)
                if(pr.mutexLessOp(walk->waiting.front()->PKgetPriority()))
                    pr=walk->waiting.front()->PKgetPriority();
            walk=walk->next;
        }
        if(pr!=owner->PKgetPriority())
        {
            FastGlobalIrqLock irqLock;
            Scheduler::IRQsetPriority(owner,pr);
        }
    }

    //Choose next thread to lock the mutex
    if(waiting.empty()==false)
    {
        //There is at least another thread waiting
        owner=waiting.front();
        pop_heap(waiting.begin(),waiting.end(),PKlowerPriority);
        waiting.pop_back();
        if(owner->mutexWaiting!=this) errorHandler(UNEXPECTED);
        owner->mutexWaiting=nullptr;
        owner->PKwakeup();
        if(owner->mutexLocked==nullptr) owner->savedPriority=owner->PKgetPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        //Handle priority inheritance of new owner
        if(waiting.empty()==false &&
                owner->PKgetPriority().mutexLessOp(waiting.front()->PKgetPriority()))
        {
            auto prio=waiting.front()->PKgetPriority();
            {
                FastGlobalIrqLock irqLock;
                Scheduler::IRQsetPriority(owner,prio);
            }
        }
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
    WaitToken listItem(Thread::getCurrentThread());
    PauseKernelLock dLock;
    unsigned int depth=m.PKunlockAllDepthLevels(dLock);
    condList.push_back(&listItem); //Putting this thread last on the list (lifo policy)
    Thread::PKrestartKernelAndWait(dLock);
    condList.removeFast(&listItem); //In case of timeout or spurious wakeup
    m.PKlockToDepth(dLock,depth);
}

void ConditionVariable::wait(pthread_mutex_t *m)
{
    WaitToken listItem(Thread::getCurrentThread());
    PauseKernelLock dLock;
    unsigned int depth=PKdoMutexUnlockAllDepthLevels(m);
    condList.push_back(&listItem); //Putting this thread last on the list (lifo policy)
    Thread::PKrestartKernelAndWait(dLock);
    condList.removeFast(&listItem); //In case of spurious wakeup
    PKdoMutexLockToDepth(m,dLock,depth);
}

TimedWaitResult ConditionVariable::timedWait(Mutex& m, long long absTime)
{
    WaitToken listItem(Thread::getCurrentThread());
    PauseKernelLock dLock;
    unsigned int depth=m.PKunlockAllDepthLevels(dLock);
    condList.push_back(&listItem); //Putting this thread last on the list (lifo policy)
    auto result=Thread::PKrestartKernelAndTimedWait(dLock,absTime);
    condList.removeFast(&listItem); //In case of timeout or spurious wakeup
    m.PKlockToDepth(dLock,depth);
    return result;
}

TimedWaitResult ConditionVariable::timedWait(pthread_mutex_t *m, long long absTime)
{
    WaitToken listItem(Thread::getCurrentThread());
    PauseKernelLock dLock;
    unsigned int depth=PKdoMutexUnlockAllDepthLevels(m);
    condList.push_back(&listItem); //Putting this thread last on the list (lifo policy)
    auto result=Thread::PKrestartKernelAndTimedWait(dLock,absTime);
    condList.removeFast(&listItem); //In case of timeout or spurious wakeup
    PKdoMutexLockToDepth(m,dLock,depth);
    return result;
}

void ConditionVariable::signal()
{
    PauseKernelLock dLock;
    if(condList.empty()) return;
    Thread *t=condList.front()->thread;
    condList.pop_front();
    t->PKwakeup();
    /*
     * A note on whether we should yield if waking a higher priority thread.
     * Doing a signal()/broadcast() is permitted either with the mutex locked
     * or not. If we're calling signal with the mutex locked, yielding if we
     * woke up a higher priority thread causes a "bounce back" since the woken
     * thread will block trying to lock the mutex we're holding.
     * The issue is, within signal()/broadcast(), we don't know if we're being
     * called with the mutex locked or not.
     * We used to only yield in ConditionVariable and not in pthread_cond_singal
     * as only the former can be used with priority inheritance mutexes, relying
     * in the latter case on the higher priority thread being scheduled anyway
     * at the end of the scheduling time quantum, but we changed this policy in
     * Miosix 3 and yield always. Note that even though there's no explicit
     * yield code, PKwakeup() is where it's hidden. This new behavior is better
     * for real-time but does incur the bounce back penalty. Tradeoffs.
     */
}

void ConditionVariable::broadcast()
{
    // Disabling interrupts would be faster but pausing kernel is an opportunity
    // to reduce interrupt latency in case we loop a large number of iterations
    PauseKernelLock dLock;
    while(!condList.empty())
    {
        Thread *t=condList.front()->thread;
        condList.pop_front();
        t->PKwakeup();
    }
}

//
// class Semaphore
//

Thread *Semaphore::IRQsignalImpl()
{
    //Check if somebody is waiting
    if(fifo.empty())
    {
        //Nobody there, just increment the counter
        count++;
        return nullptr;
    }
    WaitToken *cd=fifo.front();
    Thread *t=cd->thread;
    cd->thread=nullptr; //Thread pointer doubles as flag against spurious wakeup
    fifo.pop_front();
    t->IRQwakeup();
    return t;
}

void Semaphore::IRQsignal(bool& hppw)
{
    Thread *t=IRQsignalImpl();
    if(t==nullptr) return;
    if(Thread::IRQgetCurrentThread()->IRQgetPriority()<t->IRQgetPriority())
        hppw=true;
}

void Semaphore::IRQsignal()
{
    IRQsignalImpl();
}

void Semaphore::signal()
{
    //Global interrupt lock because Semaphore is IRQ-safe
    FastGlobalIrqLock dLock;
    //Update the state of the FIFO and the counter
    IRQsignalImpl();
}

void Semaphore::wait()
{
    //Global interrupt lock because Semaphore is IRQ-safe
    FastGlobalIrqLock dLock;
    //If the counter is positive, decrement it and we're done
    if(count>0)
    {
        count--;
        return;
    }
    //Otherwise put ourselves in queue and wait
    WaitToken listItem(Thread::IRQgetCurrentThread());
    fifo.push_back(&listItem); //Add entry to tail of list
    while(listItem.thread) Thread::IRQglobalIrqUnlockAndWait(dLock);
    //Spurious wakeup handled by while loop, listItem already removed from fifo
}

TimedWaitResult Semaphore::timedWait(long long absTime)
{
    //Global interrupt lock because Semaphore is IRQ-safe
    FastGlobalIrqLock dLock;
    //If the counter is positive, decrement it and we're done
    if(count>0)
    {
        count--;
        return TimedWaitResult::NoTimeout;
    }
    //Otherwise put ourselves in queue and wait
    WaitToken listItem(Thread::IRQgetCurrentThread());
    fifo.push_back(&listItem); //Add entry to tail of list
    while(listItem.thread)
    {
        if(Thread::IRQglobalIrqUnlockAndTimedWait(dLock,absTime)==TimedWaitResult::Timeout)
        {
            fifo.removeFast(&listItem); //Remove fifo entry in case of timeout
            return TimedWaitResult::Timeout;
        }
    }
    return TimedWaitResult::NoTimeout;
}

} //namespace miosix
