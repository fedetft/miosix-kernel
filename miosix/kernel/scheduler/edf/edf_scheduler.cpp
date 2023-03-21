/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico                   *
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

#include "edf_scheduler.h"
#include "kernel/error.h"
#include "kernel/process.h"
#include "interfaces/os_timer.h"
#include "kernel/scheduler/timer_interrupt.h"
#include <algorithm>

using namespace std;

#ifdef SCHED_TYPE_EDF

namespace miosix {

//These are defined in kernel.cpp
extern volatile Thread *cur;
extern volatile int kernel_running;
extern IntrusiveList<SleepData> sleepingList;

//Static members
static long long nextPreemption=numeric_limits<long long>::max();

//
// class EDFScheduler
//

bool EDFScheduler::PKaddThread(Thread *thread, EDFSchedulerPriority priority)
{
    thread->schedData.deadline=priority;
    add(thread);
    return true;
}

bool EDFScheduler::PKexists(Thread *thread)
{
    Thread *walk=head;
    while(walk!=nullptr)
    {
        if(walk==thread && (!walk->flags.isDeleted())) return true;
        walk=walk->schedData.next;
    }
    return false;
}

void EDFScheduler::PKremoveDeadThreads()
{
    //Delete all threads at the beginning of the list
    for(;;)
    {
        if(head==nullptr) errorHandler(UNEXPECTED); //Empty list is wrong.
        if(head->flags.isDeleted()==false) break;
        Thread *toBeDeleted=head;
        head=head->schedData.next;
        void *base=toBeDeleted->watermark;
        toBeDeleted->~Thread();
        free(base); //Delete ALL thread memory
    }
    //When we get here this->head is not null and does not need to be deleted
    Thread *walk=head;
    for(;;)
    {
        if(walk->schedData.next==nullptr) break;
        if(walk->schedData.next->flags.isDeleted())
        {
            Thread *toBeDeleted=walk->schedData.next;
            walk->schedData.next=walk->schedData.next->schedData.next;
            void *base=toBeDeleted->watermark;
            toBeDeleted->~Thread();
            free(base); //Delete ALL thread memory
        } else walk=walk->schedData.next;
    }
}

void EDFScheduler::PKsetPriority(Thread *thread,
        EDFSchedulerPriority newPriority)
{
    remove(thread);
    thread->schedData.deadline=newPriority;
    add(thread);
}

void EDFScheduler::IRQsetIdleThread(Thread *idleThread)
{
    idleThread->schedData.deadline=numeric_limits<long long>::max()-1;
    add(idleThread);
}

long long EDFScheduler::IRQgetNextPreemption()
{
    return nextPreemption;
}

static void IRQsetNextPreemption()
{
    if(sleepingList.empty())
    {
        //TODO: can't we just not set an interrupt?
        nextPreemption=numeric_limits<long long>::max();
    } else nextPreemption=sleepingList.front()->wakeup_time;
    internal::IRQosTimerSetInterrupt(nextPreemption);
}

void EDFScheduler::IRQfindNextThread()
{
    if(kernel_running!=0) return;//If kernel is paused, do nothing
    Thread *walk=head;
    for(;;)
    {
        if(walk==nullptr) errorHandler(UNEXPECTED);
        if(walk->flags.isReady())
        {
            cur=walk;
            #ifdef WITH_PROCESSES
            if(const_cast<Thread*>(cur)->flags.isInUserspace()==false)
            {
                ctxsave=cur->ctxsave;
                MPUConfiguration::IRQdisable();
            } else {
                ctxsave=cur->userCtxsave;
                //A kernel thread is never in userspace, so the cast is safe
                static_cast<Process*>(cur->proc)->mpu.IRQenable();
            }
            #else //WITH_PROCESSES
            ctxsave=cur->ctxsave;
            #endif //WITH_PROCESSES
            IRQsetNextPreemption();
            return;
        }
        walk=walk->schedData.next;
    }
}

void EDFScheduler::add(Thread *thread)
{
    long long newDeadline=thread->schedData.deadline.get();
    if(head==nullptr)
    {
        head=thread;
        return;
    }
    if(newDeadline<=head->schedData.deadline.get())
    {
        thread->schedData.next=head;
        head=thread;
        return;
    }
    Thread *walk=head;
    for(;;)
    {
        if(walk->schedData.next==nullptr || newDeadline<=
           walk->schedData.next->schedData.deadline.get())
        {
            thread->schedData.next=walk->schedData.next;
            walk->schedData.next=thread;
            break;
        }
        walk=walk->schedData.next;
    }
}

void EDFScheduler::remove(Thread *thread)
{
    if(head==nullptr) errorHandler(UNEXPECTED);
    if(head==thread)
    {
        head=head->schedData.next;
        return;
    }
    Thread *walk=head;
    for(;;)
    {
        if(walk->schedData.next==nullptr) errorHandler(UNEXPECTED);
        if(walk->schedData.next==thread)
        {
            walk->schedData.next=walk->schedData.next->schedData.next;
            break;
        }
        walk=walk->schedData.next;
    }
}

Thread *EDFScheduler::head=nullptr;

} //namespace miosix

#endif //SCHED_TYPE_EDF
