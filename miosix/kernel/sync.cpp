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

#include "sync.h"
#include "error.h"
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

int FastMutex::lock()
{
    FastPauseKernelLock dLock;
    Thread *cur=Thread::PKgetCurrentThread();
    if(owner==nullptr)
    {
        owner=cur;
        return 0;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the wait is enclosed in a
    //while(owner!=cur) which is immeditely false.
    if(owner==cur)
    {
        if(recursiveDepth>=0)
        {
            recursiveDepth++;
            return 0;
        } else errorHandler(Error::MUTEX_ERROR); //Bad, deadlock
    }

    WaitToken item(cur);
    waitQueue.PKenqueue(&item);
    //The while is necessary to protect against spurious wakeups
    while(owner!=cur) Thread::PKrestartKernelAndWait(dLock);
    return 0;
}

bool FastMutex::tryLock()
{
    FastPauseKernelLock dLock;
    Thread *cur=Thread::PKgetCurrentThread();
    if(owner==nullptr)
    {
        owner=cur;
        return true;
    }
    if(owner==cur && recursiveDepth>=0)
    {
        recursiveDepth++;
        return true;
    }
    return false;
}

int FastMutex::unlock()
{
    FastPauseKernelLock dLock;
//    Safety check removed for speed reasons
//    if(owner!=Thread::PKgetCurrentThread()) errorHandler(Error::MUTEX_ERROR);
    if(recursiveDepth>0)
    {
        recursiveDepth--;
        return 0;
    }
    owner=waitQueue.PKwakeOne();
    return 0;
}

inline void FastMutex::PKlockToDepth(FastPauseKernelLock& dLock, unsigned int depth)
{
    Thread *cur=Thread::PKgetCurrentThread();
    if(owner==nullptr)
    {
        owner=cur;
        if(recursiveDepth>=0) recursiveDepth=depth;
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the wait is enclosed in a
    //while(owner!=cur) which is immeditely false.
    if(owner==cur)
    {
        if(recursiveDepth>=0)
        {
            recursiveDepth=depth;
            return;
        } else errorHandler(Error::MUTEX_ERROR); //Bad, deadlock
    }

    WaitToken item(cur);
    waitQueue.PKenqueue(&item);
    //The while is necessary to protect against spurious wakeups
    while(owner!=cur) Thread::PKrestartKernelAndWait(dLock);
    if(recursiveDepth>=0) recursiveDepth=depth;
}

inline unsigned int FastMutex::PKunlockAllDepthLevels()
{
//    Safety check removed for speed reasons
//    if(owner!=Thread::PKgetCurrentThread()) errorHandler(Error::MUTEX_ERROR);
    owner=waitQueue.PKwakeOne();
    if(recursiveDepth<0) return 0;
    unsigned int result=recursiveDepth;
    recursiveDepth=0;
    return result;
}

//
// class Mutex
//

int Mutex::lock()
{
    PauseKernelLock dLock;
    Thread *cur=Thread::PKgetCurrentThread();
    if(owner==nullptr)
    {
        owner=cur;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(lockedListEmpty(owner)) owner->savedPriority=owner->PKgetPriority();
        this->addToLockedList(owner);
        return 0;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the wait is enclosed in a
    //while(owner!=cur) which is immeditely false.
    if(owner==cur)
    {
        if(recursiveDepth>=0)
        {
            recursiveDepth++;
            return 0;
        } else errorHandler(Error::MUTEX_ERROR); //Bad, deadlock
    }

    //Add thread to mutex' waiting queue
    waiting.push_back(cur);
    push_heap(waiting.begin(),waiting.end(),PKlowerPriority);

    //Handle priority inheritance
    if(cur->mutexWaiting!=nullptr) errorHandler(Error::UNEXPECTED);
    cur->mutexWaiting=this;
    inheritPriorityTowardsMutexOwner(cur->PKgetPriority());

    //The while is necessary to protect against spurious wakeups
    while(owner!=cur) Thread::PKrestartKernelAndWait(dLock);
    return 0;
}

bool Mutex::tryLock()
{
    PauseKernelLock dLock;
    Thread *cur=Thread::PKgetCurrentThread();
    if(owner==nullptr)
    {
        owner=cur;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(lockedListEmpty(owner)) owner->savedPriority=owner->PKgetPriority();
        this->addToLockedList(owner);
        return true;
    }
    if(owner==cur && recursiveDepth>=0)
    {
        recursiveDepth++;
        return true;
    }
    return false;
}

int Mutex::unlock()
{
    PauseKernelLock dLock;
//    Safety check removed for speed reasons
//    if(owner!=Thread::PKgetCurrentThread()) errorHandler(Error::MUTEX_ERROR);

    if(recursiveDepth>0)
    {
        recursiveDepth--;
        return 0;
    }

    deInheritPriority();
    chooseNextOwner();
    return 0;
}

void Mutex::PKlockToDepth(PauseKernelLock& dLock, unsigned int depth)
{
    Thread *cur=Thread::PKgetCurrentThread();
    if(owner==nullptr)
    {
        owner=cur;
        if(recursiveDepth>=0) recursiveDepth=depth;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(lockedListEmpty(owner)) owner->savedPriority=owner->PKgetPriority();
        this->addToLockedList(owner);
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the wait is enclosed in a
    //while(owner!=cur) which is immeditely false.
    if(owner==cur)
    {
        if(recursiveDepth>=0)
        {
            recursiveDepth=depth;
            return;
        } else errorHandler(Error::MUTEX_ERROR); //Bad, deadlock
    }

    //Add thread to mutex' waiting queue
    waiting.push_back(cur);
    push_heap(waiting.begin(),waiting.end(),PKlowerPriority);

    //Handle priority inheritance
    if(cur->mutexWaiting!=nullptr) errorHandler(Error::UNEXPECTED);
    cur->mutexWaiting=this;
    inheritPriorityTowardsMutexOwner(cur->PKgetPriority());

    //The while is necessary to protect against spurious wakeups
    while(owner!=cur) Thread::PKrestartKernelAndWait(dLock);
    if(recursiveDepth>=0) recursiveDepth=depth;
}

unsigned int Mutex::PKunlockAllDepthLevels()
{
//    Safety check removed for speed reasons
//    if(owner!=Thread::PKgetCurrentThread()) errorHandler(Error::MUTEX_ERROR);

    //NOTE: unlike Mutex::unlock() this function is only used to wait in
    //condition variables, after this call the current thread is descheduled
    //so it's irrelevant whether we woke a higher priority thread
    deInheritPriority();
    chooseNextOwner();

    if(recursiveDepth<0) return 0;
    unsigned int result=recursiveDepth;
    recursiveDepth=0;
    return result;
}

inline void Mutex::deInheritPriority()
{
    this->removeFromLockedList(owner);
    Priority pr=owner->savedPriority;
    if(lockedListEmpty(owner)==false) pr=inheritPriorityFromLockedList(owner,pr);
    if(pr!=owner->PKgetPriority())
    {
        FastGlobalIrqLock irqLock;
        Scheduler::IRQsetPriority(owner,pr);
    }
}

inline void Mutex::chooseNextOwner()
{
    if(waiting.empty()==false)
    {
        //There is at least another thread waiting
        owner=waiting.front();
        pop_heap(waiting.begin(),waiting.end(),PKlowerPriority);
        waiting.pop_back();
        if(owner->mutexWaiting!=this) errorHandler(Error::UNEXPECTED);
        owner->mutexWaiting=nullptr;
        owner->PKwakeup(); //Also sets pendingWakeup if higher priority thread woken
        if(lockedListEmpty(owner)) owner->savedPriority=owner->PKgetPriority();
        this->addToLockedList(owner);
        //NOTE: since we always pick the highest priority waiting thread
        //(and this priority includes priority inheritance while waiting, see
        //Mutex::inheritPriorityTowardsMutexOwner) to become the new mutex owner
        //there's no need to make the new owner inherit priority from the other
        //waiting threads on the same mutex. If there were a higher priority
        //thread there, it would have been chosen as the owner
    } else {
        owner=nullptr; //No threads waiting
        //Here we could either reclaim some RAM (by uncommenting this line) or
        //boost performance since especially with periodic tasks locking mutexes
        //in the same pattern leaving the vector capacity allows the mutex to
        //reach a steady state where no more memory allocations occur
        // std::vector<Thread *>().swap(waiting);
    }
}

void Mutex::inheritPriorityTowardsMutexOwner(Priority prio)
{
    //NOTE: even though here we may change the priority of one or more
    //threads there's no need to check if a high priority thread needs
    //to be scheduled on some core and call the scheduler, as at the end
    //of this algorithm we call Thread::PKrestartKernelAndWait that will
    //take care of calling the scheduler
    Thread *walk=owner;
    while(walk->PKgetPriority().mutexLessOp(prio))
    {
        {
            FastGlobalIrqLock irqLock;
            Scheduler::IRQsetPriority(walk,prio);
        }
        if(walk->mutexWaiting==nullptr) break;
        //We upgraded the priority of the thread that is currently the owner
        //of the mutex we want to lock, but unfortunately the owner is stuck
        //waiting on another mutex. Thus, we need to update the min heap of
        //the other mutex so it is kept sorted, and then continue down the
        //chain to find who's the owner of that mutex
        make_heap(walk->mutexWaiting->waiting.begin(),
                  walk->mutexWaiting->waiting.end(),PKlowerPriority);
        walk=walk->mutexWaiting->owner;
    }
}

/*
 * A note about the following functions: the list of mutexes with priority
 * inheritance a thread is locking is implemented intrusively partly in
 * class Thread (the list head is Thread::mutexLocked), and partly in
 * class Mutex (Mutex::next). Using an intrusive singly linked list provides
 * optimal performance and mininal RAM requirements as mutexes should be
 * unlocked in reverse order and lifo insert/removal from a singly linked list
 * is O(1).
 * However, while from a software engineering standpoint list handling functions
 * would belong to class Thread (the thread has a list of locked mutexes, not
 * the other way), they are implemented in class Mutex as they are mostly used
 * here and moving them to class Thread would prevent inlining them as they
 * can't be in the header file without creating an #include loop.
 */

inline bool Mutex::lockedListEmpty(Thread *t) { return t->mutexLocked==nullptr; }

inline void Mutex::addToLockedList(Thread *t)
{
    next=t->mutexLocked;
    t->mutexLocked=this;
}

inline void Mutex::removeFromLockedList(Thread *t)
{
    //If mutexes are unlocked in reverse order (as they should) we always take
    //the fast path
    if(t->mutexLocked==this)
    {
        t->mutexLocked=t->mutexLocked->next;
    } else {
        for(Mutex *walk=t->mutexLocked;;walk=walk->next)
        {
            //this Mutex not in owner's list? impossible
            if(walk->next==nullptr) errorHandler(Error::UNEXPECTED);
            if(walk->next==this)
            {
                walk->next=walk->next->next;
                break;
            }
        }
    }
}

Priority Mutex::inheritPriorityFromLockedList(Thread *t, Priority pr)
{
    Mutex *walk=t->mutexLocked;
    while(walk!=nullptr)
    {
        if(walk->waiting.empty()==false)
        {
            Priority inheritedPr=walk->waiting.front()->PKgetPriority();
            if(pr.mutexLessOp(inheritedPr)) pr=inheritedPr;
        }
        walk=walk->next;
    }
    return pr;
}

//
// class ConditionVariable
//

void ConditionVariable::wait(Mutex& m)
{
    WaitToken item(Thread::getCurrentThread());
    PauseKernelLock dLock;
    unsigned int depth=m.PKunlockAllDepthLevels();
    waitQueue.PKenqueue(&item);
    Thread::PKrestartKernelAndWait(dLock);
    waitQueue.PKremove(&item); //In case of spurious wakeup
    m.PKlockToDepth(dLock,depth);
}

void ConditionVariable::wait(FastMutex& m)
{
    WaitToken item(Thread::getCurrentThread());
    FastPauseKernelLock dLock;
    unsigned int depth=m.PKunlockAllDepthLevels();
    waitQueue.PKenqueue(&item);
    Thread::PKrestartKernelAndWait(dLock);
    waitQueue.PKremove(&item); //In case of spurious wakeup
    m.PKlockToDepth(dLock,depth);
}

TimedWaitResult ConditionVariable::timedWait(Mutex& m, long long absTime)
{
    WaitToken item(Thread::getCurrentThread());
    PauseKernelLock dLock;
    unsigned int depth=m.PKunlockAllDepthLevels();
    waitQueue.PKenqueue(&item);
    auto result=Thread::PKrestartKernelAndTimedWait(dLock,absTime);
    waitQueue.PKremove(&item); //In case of timeout or spurious wakeup
    m.PKlockToDepth(dLock,depth);
    return result;
}

TimedWaitResult ConditionVariable::timedWait(FastMutex& m, long long absTime)
{
    WaitToken item(Thread::getCurrentThread());
    FastPauseKernelLock dLock;
    unsigned int depth=m.PKunlockAllDepthLevels();
    waitQueue.PKenqueue(&item);
    auto result=Thread::PKrestartKernelAndTimedWait(dLock,absTime);
    waitQueue.PKremove(&item); //In case of timeout or spurious wakeup
    m.PKlockToDepth(dLock,depth);
    return result;
}

void ConditionVariable::signal()
{
    /*
     * A note on whether we should yield if waking a higher priority thread.
     * Doing a signal()/broadcast() is permitted either with the mutex locked
     * or not. If we're calling signal with the mutex locked, yielding if we
     * woke up a higher priority thread causes a "bounce back" since the woken
     * thread will block trying to lock the mutex we're holding.
     * The issue is, within signal()/broadcast(), we don't know if we're being
     * called with the mutex locked or not. In Miosix 3 we always yield if a
     * higher priority thread is awakened. This new behavior is better for
     * real-time but does incur the bounce back penalty. Tradeoffs.
     */
    FastPauseKernelLock dLock;
    waitQueue.PKwakeOne();
}

void ConditionVariable::broadcast()
{
    FastPauseKernelLock dLock;
    waitQueue.PKwakeAll();
}

bool ConditionVariable::empty() const
{
    FastPauseKernelLock dLock;
    return waitQueue.PKempty();
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
