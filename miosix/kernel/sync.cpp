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
#include "error.h"

namespace miosix {

//
// class Mutex
//

Mutex::Mutex(Options opt): owner(0), next(0), waiting(), mutexOptions(opt),
        recursiveDepth(0) {}

void Mutex::PKlock(PauseKernelLock& dLock)
{
    if(owner==0)
    {
        owner=Thread::getCurrentThread();
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(owner->mutexLocked==0) owner->savedPriority=owner->getPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
    } else {
        Thread* p=Thread::getCurrentThread();

        //This check is very important. Without this attempting to lock the same
        //mutex twice won't cause a deadlock because the Thread::IRQwait() is
        //enclosed in a while(owner!=p) which is immeditely false.
        if(owner==p)
        {
            if(mutexOptions==RECURSIVE)
            {
                recursiveDepth++;
                return;
            }
            else {
                //Bad, deadlock
                errorHandler(MUTEX_DEADLOCK);
            }
        }
        
        //Add thread to mutex' waiting queue
        waiting.push(p);
        
        //Handle priority inheritance
        if(p->getPriority()>owner->getPriority())
            Scheduler::PKsetPriority(owner,p->getPriority());

        //The while is necessary because some other thread might call wakeup()
        //on this thread. So the thread can wakeup also for other reasons not
        //related to the mutex becoming free
        while(owner!=p)
        {
            //Wait can only be called with kernel started, while IRQwait can
            //only be called with interupts disabled, so that's why interrupts
            //are disabled
            {
                InterruptDisableLock l;
                Thread::IRQwait();//Return immediately
            }
            {
                RestartKernelLock eLock(dLock);
                //Now the IRQwait becomes effective
                Thread::yield();
            }
        }
    }
}

bool Mutex::PKtryLock(PauseKernelLock& dLock)
{
    if(owner==0)
    {
        owner=Thread::getCurrentThread();
        //Save original thread priority, if the thread has not yet locked
        //another mutex
        if(owner->mutexLocked==0) owner->savedPriority=owner->getPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        return true;
    }
    return false;
}

void Mutex::PKunlock(PauseKernelLock& dLock)
{
    if(Thread::getCurrentThread()!=owner)
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
            if(walk->waiting.empty()==false && walk->waiting.top()->
                    getPriority()>pr) pr=walk->waiting.top()->getPriority();
            walk=walk->next;
        }
        if(pr!=owner->getPriority()) Scheduler::PKsetPriority(owner,pr);
    }

    //Choose next thread to lock the mutex
    if(waiting.empty()==false)
    {
        //There is at least another thread waiting
        owner=waiting.top();
        waiting.pop();
        owner->PKwakeup();
        if(owner->mutexLocked==0) owner->savedPriority=owner->getPriority();
        //Add this mutex to the list of mutexes locked by owner
        this->next=owner->mutexLocked;
        owner->mutexLocked=this;
        //Handle priority inheritance of new owner
        if(waiting.empty()==false && waiting.top()->getPriority()>owner->
                getPriority())
        {
            Scheduler::PKsetPriority(owner,waiting.top()->getPriority());
        }
    } else owner=0; //No threads waiting
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
        InterruptDisableLock l;
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
        InterruptDisableLock lock;
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
        InterruptDisableLock lock;
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
