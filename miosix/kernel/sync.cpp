/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010 by Terraneo Federico                   *
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

#include "sync.h"
#include "kernel.h"
#include "kernel/scheduler/scheduler.h"
#include "error.h"
#include <algorithm>

using namespace std;

namespace miosix {

// FIXME : begin

/**
 * Implementation code to lock a mutex. Must be called with interrupts disabled
 * \param mutex mutexto be locked
 * \param d The instance of FastInterruptDisableLock used to disable interrupts
 */
static inline void IRQdoMutexLock(MutexImpl *mutex, FastInterruptDisableLock& d)
{
    void *p=reinterpret_cast<void*>(Thread::IRQgetCurrentThread());
    if(mutex->owner==0)
    {
        mutex->owner=p;
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the Thread::IRQwait() is
    //enclosed in a while(owner!=p) which is immeditely false.
    if(mutex->owner==p)
    {
        if(mutex->recursive>=0)
        {
            mutex->recursive++;
            return;
        } else errorHandler(MUTEX_DEADLOCK); //Bad, deadlock
    }

    WaitingList waiting; //Element of a linked list on stack
    waiting.thread=p;
    waiting.next=0; //Putting this thread last on the list (lifo policy)
    if(mutex->first==0)
    {
        mutex->first=&waiting;
        mutex->last=&waiting;
    } else {
        mutex->last->next=&waiting;
        mutex->last=&waiting;
    }

    //The while is necessary because some other thread might call wakeup()
    //on this thread. So the thread can wakeup also for other reasons not
    //related to the mutex becoming free
    while(mutex->owner!=p)
    {
        Thread::IRQwait();//Returns immediately
        {
            FastInterruptEnableLock eLock(d);
            Thread::yield(); //Now the IRQwait becomes effective
        }
    }
}

/**
 * Implementation code to unlock a mutex.
 * Must be called with interrupts disabled
 * \param mutex mutex to unlock
 * \return true if a higher priority thread was woken,
 * only if EDF scheduler is selected, otherwise it always returns false
 */
static inline bool IRQdoMutexUnlock(MutexImpl *mutex)
{
//    Safety check removed for speed reasons
//    if(mutex->owner!=reinterpret_cast<void*>(Thread::IRQgetCurrentThread()))
//    {
//        errorHandler(MUTEX_UNLOCK_NOT_OWNER);
//        return false;
//    }
    if(mutex->recursive>0)
    {
        mutex->recursive--;
        return false;
    }
    if(mutex->first!=0)
    {
        Thread *t=reinterpret_cast<Thread*>(mutex->first->thread);
        t->IRQwakeup();
        mutex->owner=mutex->first->thread;
        mutex->first=mutex->first->next;

        #ifndef SCHED_TYPE_EDF
        if(t->IRQgetPriority() >Thread::IRQgetCurrentThread()->IRQgetPriority())
            return true;
        #endif //SCHED_TYPE_EDF
        return false;
    }
    mutex->owner=0;
    return false;
}

void fixmeMutexInit(MutexImpl *mutex, bool recursive)
{
    mutex->owner=0;
    mutex->first=0;
    //No need to initialize mutex->last
    mutex->recursive= recursive ? 0 : -1;
}

void fixmeMutexDestroy(MutexImpl *mutex)
{
    //Do nothing
}

void fixmeMutexLock(MutexImpl *mutex)
{
    FastInterruptDisableLock dLock;
    IRQdoMutexLock(mutex,dLock);
}

bool fixmeMutexTryLock(MutexImpl *mutex)
{
    FastInterruptDisableLock dLock;
    void *p=reinterpret_cast<void*>(Thread::IRQgetCurrentThread());
    if(mutex->owner==0)
    {
        mutex->owner=p;
        return true;
    }
    if(mutex->owner==p && mutex->recursive>=0)
    {
        mutex->recursive++;
        return true;
    }
    return false;
}

void fixmeMutexUnlock(MutexImpl *mutex)
{
    #ifndef SCHED_TYPE_EDF
    FastInterruptDisableLock dLock;
    IRQdoMutexUnlock(mutex);
    #else //SCHED_TYPE_EDF
    bool hppw;
    {
        FastInterruptDisableLock dLock;
        hppw=IRQdoMutexUnlock(mutex);
    }
    //If the woken thread has higher priority, yield
    if(hppw) Thread::yield();
    #endif //SCHED_TYPE_EDF
}

void fixmeCondInit(CondvarImpl *cond)
{
    cond->first=0;
    //No need to initialize cond->last
}

void fixmeCondDestroy(CondvarImpl *cond)
{
    //Do nothing
}

void fixmeCondWait(CondvarImpl *cond, MutexImpl *mutex)
{
//    FastInterruptDisableLock dLock;
//    Thread *p=Thread::IRQgetCurrentThread();
//    WaitingList waiting; //Element of a linked list on stack
//    waiting.thread=reinterpret_cast<void*>(p);
//    waiting.next=0; //Putting this thread last on the list (lifo policy)
//    if(cond->first==0)
//    {
//        cond->first=&waiting;
//        cond->last=&waiting;
//    } else {
//        cond->last->next=&waiting;
//        cond->last=&waiting;
//    }
//    p->flags.IRQsetCondWait(true);
//
//    IRQdoMutexUnlock(mutex);
//    {
//        FastInterruptEnableLock eLock(dLock);
//        Thread::yield(); //Here the wait becomes effective
//    }
//    IRQdoMutexLock(mutex,dLock);
}

void fixmeCondSignal(CondvarImpl *cond)
{
//    #ifdef SCHED_TYPE_EDF
//    bool hppw=false;
//    #endif //SCHED_TYPE_EDF
//    {
//        FastInterruptDisableLock dLock;
//        if(cond->first==0) return;
//
//        Thread *t=reinterpret_cast<Thread*>(cond->first->thread);
//        t->flags.IRQsetCondWait(false);
//        cond->first=cond->first->next;
//
//        #ifdef SCHED_TYPE_EDF
//        if(t->IRQgetPriority() >Thread::IRQgetCurrentThread()->IRQgetPriority())
//            hppw=true;
//        #endif //SCHED_TYPE_EDF
//    }
//    #ifdef SCHED_TYPE_EDF
//    //If the woken thread has higher priority, yield
//    if(hppw) Thread::yield();
//    #endif //SCHED_TYPE_EDF
}

void fixmeCondBroadcast(CondvarImpl *cond)
{
//    #ifdef SCHED_TYPE_EDF
//    bool hppw=false;
//    #endif //SCHED_TYPE_EDF
//    {
//        FastInterruptDisableLock lock;
//        while(cond->first!=0)
//        {
//            Thread *t=reinterpret_cast<Thread*>(cond->first->thread);
//            t->flags.IRQsetCondWait(false);
//            cond->first=cond->first->next;
//
//            #ifdef SCHED_TYPE_EDF
//            if(t->IRQgetPriority() >
//                    Thread::IRQgetCurrentThread()->IRQgetPriority()) hppw=true;
//            #endif //SCHED_TYPE_EDF
//        }
//    }
//    #ifdef SCHED_TYPE_EDF
//    //If at least one of the woken thread has higher, yield
//    if(hppw) Thread::yield();
//    #endif //SCHED_TYPE_EDF
}

// FIXME : end

//
// class Mutex
//

Mutex::Mutex(Options opt): owner(0), next(0), waiting(), mutexOptions(opt),
        recursiveDepth(0) {}

void Mutex::PKlock(PauseKernelLock& dLock)
{
    Thread *p=Thread::getCurrentThread();
    if(owner==0)
    {
        owner=p;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(owner->mutexLocked==0) owner->savedPriority=owner->getPriority();
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
        if(mutexOptions==RECURSIVE)
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
    if(p->mutexWaiting!=0) errorHandler(UNEXPECTED);
    p->mutexWaiting=this;
    if(p->getPriority()>owner->getPriority())
    {
        Thread *walk=owner;
        for(;;)
        {
            Scheduler::PKsetPriority(walk,p->getPriority());
            if(walk->mutexWaiting==0) break;
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

bool Mutex::PKtryLock(PauseKernelLock& dLock)
{
    Thread *p=Thread::getCurrentThread();
    if(owner==0)
    {
        owner=p;
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(owner->mutexLocked==0) owner->savedPriority=owner->getPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        return true;
    }
    if(owner==p && mutexOptions==RECURSIVE)
    {
        recursiveDepth++;
        return true;
    }
    return false;
}

void Mutex::PKunlock(PauseKernelLock& dLock)
{
    Thread *p=Thread::getCurrentThread();
    if(owner!=p)
    {
        errorHandler(MUTEX_UNLOCK_NOT_OWNER);
        return;
    }

    if(mutexOptions==RECURSIVE && recursiveDepth>0)
    {
        recursiveDepth--;
        return;
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
            if(walk->next==0) errorHandler(UNEXPECTED);
            if(walk->next==this)
            {
                walk->next=walk->next->next;
                break;
            }
            walk=walk->next;
        }
    }

    //Handle priority inheritance
    if(owner->mutexLocked==0)
    {
        //Not locking any other mutex
        if(owner->savedPriority!=owner->getPriority())
            Scheduler::PKsetPriority(owner,owner->savedPriority);
    } else {
        Priority pr=owner->savedPriority;
        //Calculate new priority of thread, which is
        //max(savedPriority, inheritedPriority)
        Mutex *walk=owner->mutexLocked;
        while(walk!=0)
        {
            if(walk->waiting.empty()==false)
                pr=max(pr,walk->waiting.front()->getPriority());
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
        owner->mutexWaiting=0;
        owner->PKwakeup();
        if(owner->mutexLocked==0) owner->savedPriority=owner->getPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        //Handle priority inheritance of new owner
        if(waiting.empty()==false &&
           waiting.front()->getPriority()>owner->getPriority())
                Scheduler::PKsetPriority(owner,waiting.front()->getPriority());
    } else {
        owner=0; //No threads waiting
        std::vector<Thread *>().swap(waiting); //Save some RAM
    }
}

//
// class ConditionVariable
//

ConditionVariable::ConditionVariable(): first(0), last(0) {}

void ConditionVariable::PKwait(Mutex& m, PauseKernelLock& dLock)
{
    //Add to list
    WaitingData w;
    w.p=Thread::getCurrentThread();
    w.next=0; 
    //Add entry to tail of list
    if(first==0)
    {
        first=last=&w;
    } else {
       last->next=&w;
       last=&w;
    }
    //Unlock mutex and wait
    {
        FastInterruptDisableLock l;
        w.p->flags.IRQsetCondWait(true);
    }

    m.PKunlock(dLock);
    {
        RestartKernelLock eLock(dLock);
        Thread::yield(); //Here the wait becomes effective
    }
    m.PKlock(dLock);
}

void ConditionVariable::signal()
{
    bool hppw=false;
    {
        //Using interruptDisableLock because we need to call IRQsetCondWait
        //that can only be called with irq disabled, othrwise we would use
        //PauseKernelLock
        FastInterruptDisableLock lock;
        if(first==0) return;
        //Wakeup
        first->p->flags.IRQsetCondWait(false);
        //Check for priority issues
        if(first->p->IRQgetPriority() >
                Thread::IRQgetCurrentThread()->IRQgetPriority()) hppw=true;
        //Remove from list
        first=first->next;
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
        while(first!=0)
        {
            //Wakeup
            first->p->flags.IRQsetCondWait(false);
            //Check for priority issues
            if(first->p->IRQgetPriority() >
                Thread::IRQgetCurrentThread()->IRQgetPriority()) hppw=true;
            //Remove from list
            first=first->next;
        }
    }
    //If at least one of the woken thread has higher priority than our priority,
    //yield
    if(hppw) Thread::yield();
}

//
// class Timer
//

Timer::Timer()
{
    first=true;
    running=false;
    start_tick=tick_count=0;
}

void Timer::start()
{
    first=false;
    running=true;
    start_tick=getTick();
}

void Timer::stop()
{
    if(running==false) return;
    running=false;
    tick_count+=getTick()-start_tick;
    start_tick=0;
}

bool Timer::isRunning() const
{
    return running;
}

int Timer::interval() const
{
    if((first==true)||(running==true)||(tick_count>2147483647)) return -1;
    return (int)tick_count;
}

void Timer::clear()
{
    first=true;
    running=false;
    start_tick=tick_count=0;
}

}; //namespace miosix
