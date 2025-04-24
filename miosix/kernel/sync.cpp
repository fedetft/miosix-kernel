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

    WaitingList waiting; //Element of a linked list on stack
    waiting.thread=cur;
    waiting.next=nullptr; //Putting this thread last on the list (lifo policy)
    if(first==nullptr)
    {
        first=&waiting;
        last=&waiting;
    } else {
        last->next=&waiting;
        last=&waiting;
    }

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
    if(first!=nullptr)
    {
        Thread *t=first->thread;
        first=first->next;
        owner=t;
        t->PKwakeup(); //Also sets pendingWakeup if higher priority thread woken
        return 0;
    }
    owner=nullptr;
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

    WaitingList waiting; //Element of a linked list on stack
    waiting.thread=cur;
    waiting.next=nullptr; //Putting this thread last on the list (lifo policy)
    if(first==nullptr)
    {
        first=&waiting;
        last=&waiting;
    } else {
        last->next=&waiting;
        last=&waiting;
    }

    //The while is necessary to protect against spurious wakeups
    while(owner!=cur) Thread::PKrestartKernelAndWait(dLock);
    if(recursiveDepth>=0) recursiveDepth=depth;
}

inline unsigned int FastMutex::PKunlockAllDepthLevels()
{
//    Safety check removed for speed reasons
//    if(owner!=Thread::PKgetCurrentThread()) errorHandler(Error::MUTEX_ERROR);
    if(first!=nullptr)
    {
        Thread *t=first->thread;
        first=first->next;
        owner=t;
        //NOTE: unlike FastMutex::unlock() this function is only used to wait in
        //condition variables, after this call the current thread is descheduled
        //so it's irrelevant whether we woke a higher priority thread
        t->PKwakeup();

        if(recursiveDepth<0) return 0;
        unsigned int result=recursiveDepth;
        recursiveDepth=0;
        return result;
    }

    owner=nullptr;

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
    addToWaitQueue(&item);
    Thread::PKrestartKernelAndWait(dLock);
    removeFromWaitQueue(&item); //In case of spurious wakeup
    m.PKlockToDepth(dLock,depth);
}

void ConditionVariable::wait(FastMutex& m)
{
    WaitToken item(Thread::getCurrentThread());
    FastPauseKernelLock dLock;
    unsigned int depth=m.PKunlockAllDepthLevels();
    addToWaitQueue(&item);
    Thread::PKrestartKernelAndWait(dLock);
    removeFromWaitQueue(&item); //In case of spurious wakeup
    m.PKlockToDepth(dLock,depth);
}

TimedWaitResult ConditionVariable::timedWait(Mutex& m, long long absTime)
{
    WaitToken item(Thread::getCurrentThread());
    PauseKernelLock dLock;
    unsigned int depth=m.PKunlockAllDepthLevels();
    addToWaitQueue(&item);
    auto result=Thread::PKrestartKernelAndTimedWait(dLock,absTime);
    removeFromWaitQueue(&item); //In case of timeout or spurious wakeup
    m.PKlockToDepth(dLock,depth);
    return result;
}

TimedWaitResult ConditionVariable::timedWait(FastMutex& m, long long absTime)
{
    WaitToken item(Thread::getCurrentThread());
    FastPauseKernelLock dLock;
    unsigned int depth=m.PKunlockAllDepthLevels();
    addToWaitQueue(&item);
    auto result=Thread::PKrestartKernelAndTimedWait(dLock,absTime);
    removeFromWaitQueue(&item); //In case of timeout or spurious wakeup
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
    #if defined(SCHED_TYPE_PRIORITY) && defined(CONDVAR_WAKEUP_BY_PRIORITY)
    if(condLists==nullptr) return;
    for(int i=NUM_PRIORITIES-1;i>=0;i--)
    {
        if(condLists[i].empty()) continue;
        //Also sets pendingWakeup if higher priority thread woken
        condLists[i].front()->t->PKwakeup();
        condLists[i].pop_front();
        break;
    }
    #else
    if(condList.empty()) return;
    //Also sets pendingWakeup if higher priority thread woken
    condList.front()->t->PKwakeup();
    condList.pop_front();
    #endif
}

void ConditionVariable::broadcast()
{
    FastPauseKernelLock dLock;
    #if defined(SCHED_TYPE_PRIORITY) && defined(CONDVAR_WAKEUP_BY_PRIORITY)
    if(condLists==nullptr) return;
    for(int i=NUM_PRIORITIES-1;i>=0;i--)
    {
        while(!condLists[i].empty())
        {
            //Also sets pendingWakeup if higher priority thread woken
            condLists[i].front()->t->PKwakeup();
            condLists[i].pop_front();
        }
    }
    #else
    while(!condList.empty())
    {
        //Also sets pendingWakeup if higher priority thread woken
        condList.front()->t->PKwakeup();
        condList.pop_front();
    }
    #endif
}

bool ConditionVariable::isEmpty() const
{
    #if defined(SCHED_TYPE_PRIORITY) && defined(CONDVAR_WAKEUP_BY_PRIORITY)
    if(condLists==nullptr) return true;
    for(int i=NUM_PRIORITIES-1;i>=0;i--) if(condLists[i].empty()==false) return false;
    return true;
    #else
    return condList.empty();
    #endif
}

#if defined(SCHED_TYPE_PRIORITY) && defined(CONDVAR_WAKEUP_BY_PRIORITY)
ConditionVariable::~ConditionVariable()
{
    if(condLists) delete[] condLists;
}
#endif

inline void ConditionVariable::addToWaitQueue(WaitToken *item)
{
    #if defined(SCHED_TYPE_EDF)
    /*
     * With EDF there is a simple implementation: the priority is the absolute
     * deadline time in nanoseconds, thus we can just use a single list sorted
     * by deadline and not care about threads having the same priority, just
     * like sleepingList. If more threads with the exact same deadline are
     * inserted, they will be awakened in lifo order but it doesn't matter since
     * if the task pool is schedulable they will still complete before their
     * deadline.
     * NOTE: we can just call PKgetPriority() and not bother with savedPriority
     * as the case we care about is when a thread has locked a single mutex and
     * that's the one that was atomically unlocked as part of the wait(). Since
     * we get here after the mutex has been unlocked, the priority or better,
     * deadline, has already been de-inherited, if at all. Even if some weird
     * code did the antipattern of locking some more mutex and thus waiting with
     * some mutex locked and a priority inheritance occurs while waiting, since
     * we have only one list no memory corruption is possible in
     * removeFromWaitQueue
     */
    Priority pr=item->t->PKgetPriority();
    auto it=condList.begin();
    while(it!=condList.end() && pr.mutexLessOp((*it)->PKgetPriority())) ++it;
    condList.insert(it,item);
    #elif defined(SCHED_TYPE_PRIORITY) && defined(CONDVAR_WAKEUP_BY_PRIORITY)
    /*
     * That's the hard case. Using a single list leaves no way to efficiently
     * implement this algorithm. We could inserion-sort into the list by
     * priority like with EDF, but doing so in the use case where more threads
     * in the list have the same priority causes wakeup in lifo order which may
     * result in starvation. Doing inserion-sort into the list but adding
     * another loop to put the thread at the end of the threads with the same
     * priority already in the list allows prioritization and fifo order, but
     * causes O(n) insertion in the use case where all threads in the list have
     * the same priority.
     * The only way out is to have an array of lists, one for each priority
     * level, but that adds more complications that we address in our code:
     * - The array has to be dynamically allocated otherwise the size of the
     *   condition variable object would depend on the number of priority levels
     *   selected when compiling the kernel. This is not possible since the
     *   memory layout of ConditionVariable must be compatible with
     *   pthread_cond_t which is fixed when the C standard library is compiled.
     * - Moreover, we can't even allocate the array in the cosntructor since
     *   when a pthread_cond_t is staticall initialized with
     *   PTHREAD_COND_INITIALIZER the constructor isn't called and all class
     *   fields are set to 0/nullptr (see Miosix compiler patches). Thus we must
     *   allocate it on first use. Thankfully deallocation is easy as statically
     *   constructed objects have a lifetime equal to the entire program thus no
     *   deallocation is needed, while pthread_cond_destroy calls the destructor
     *   so no memory leaks are possible.
     * - Finally, once we have an array of lists, one for each priority, we must
     *   be absolutely sure that removeFromWaitQueue will try to remove the
     *   thread from the same list we are inserting it here, otherise memory
     *   corruption occurs in IntrusiveList::removeFast. That's made easier by
     *   the fact that in Miosix a thread can only change its own priority, and
     *   of course while waiting on a condition variable a thread isn't running
     *   and thus cannot call Thread::setPriority() but there's a catch:
     *   priority inheritance. When waiting on a condition variable a thread
     *   should have locked a mutex but the mutex passed to wait() is atomically
     *   unlocked before waiting (and before this function is called) so any
     *   priority inheritance happening on that mutex is no longer a concern.
     *   However, a thread may have locked other mutexes. I know, that's an
     *   antipattern since you shouldn't hold additional mutexes locked while
     *   waiting on a condition variable and if code does that we don't bother
     *   to inherit the priority from a mutex to a condition variable (i.e: the
     *   waiting thread doesn't switch to a higher priority list) because that's
     *   not priority inversion: that's an antipattern. However, we  don't want
     *   to cause memory corruption and a crash in code does that antipattern
     *   either. To solve the issue, we use the thread's savedPriority instead
     *   of actual priority as index into the list. That doesn't change.
     *   Final remark: this is the only code in the kernel that cares about
     *   savedPriority even when not locking and mutex, so the code to make sure
     *   savedPriority is set to a valid value even when not locking any mutex
     *   is added only when needed, look for CONDVAR_WAKEUP_BY_PRIORITY in the
     *   kernel code.
     */
    if(item->t->savedPriority<0) errorHandler(Error::UNEXPECTED); //Idle can't wait
    if(condLists==nullptr) condLists=new IntrusiveList<WaitToken>[NUM_PRIORITIES];
    condLists[item->t->savedPriority.get()].push_back(item);
    #else
    condList.push_back(item); //fallback is simple fifo policy
    #endif
}

inline void ConditionVariable::removeFromWaitQueue(WaitToken *item)
{
    #if defined(SCHED_TYPE_PRIORITY) && defined(CONDVAR_WAKEUP_BY_PRIORITY)
    condLists[item->t->savedPriority.get()].removeFast(item);
    #else
    condList.removeFast(item);
    #endif
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
