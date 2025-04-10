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
#include "interfaces_private/cpu.h"
#include "interfaces_private/os_timer.h"
#include <algorithm>

using namespace std;

#ifdef SCHED_TYPE_EDF

namespace miosix {

//These are defined in thread.cpp
extern volatile Thread *runningThread;
extern volatile bool pendingWakeup;
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
    // Search in the RT task list
    Thread *walk=head;
    while(walk!=nullptr)
    {
        if(walk==thread && (!walk->flags.isDeleted())) return true;
        walk=walk->schedData.next;
    }

    // Search in the NRT task list (circular list)
    if(headNRT != nullptr)
    {
        Thread *start = headNRT;
        Thread *walkNRT = headNRT;
        do {
            if(walkNRT == thread && !walkNRT->flags.isDeleted()) return true;
            walkNRT = walkNRT->schedData.next;
        } while(walkNRT != start); // Stop when we complete a full loop
    }

    return false;
}

void EDFScheduler::PKremoveDeadThreads()
{
    // Handle RT tasks (head list)
    while(head!=nullptr)
    {
        if(head->flags.isDeleted()==false) break;
        Thread *toBeDeleted=head;
        head=head->schedData.next;
        void *base=toBeDeleted->watermark;
        toBeDeleted->~Thread();
        free(base); //Delete ALL thread memory
    }
    if(head!=nullptr)
    {
        //When we get here, head is not null and does not need to be deleted
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

    // Handle NRT tasks (headNRT circular list)
    if(headNRT != nullptr)
    {
        bool firstPass = true;
        Thread *prev = headNRT;
        Thread *curr = headNRT;

        do {
            if(curr->flags.isDeleted())
            {
                Thread *toBeDeleted = curr;
                
                if(curr == headNRT)
                {
                    if(headNRT->schedData.next == headNRT)
                    {
                        // Only one element in the circular list
                        headNRT = nullptr;
                        free(toBeDeleted->watermark);
                        toBeDeleted->~Thread();
                        return;
                    } else {
                        // Find the last node to fix circular list
                        Thread *tail = headNRT;
                        while(tail->schedData.next != headNRT)
                        {
                            tail = tail->schedData.next;
                        }
                        headNRT = headNRT->schedData.next;
                        tail->schedData.next = headNRT;
                    }
                } else {
                    prev->schedData.next = curr->schedData.next;
                }

                free(toBeDeleted->watermark);
                toBeDeleted->~Thread();

                curr = prev->schedData.next;
            } else {
                prev = curr;
                curr = curr->schedData.next;
            }

            firstPass = false;
        } while(curr != headNRT && !firstPass);
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
    idle = idleThread;
}

long long EDFScheduler::IRQgetNextPreemption()
{
    return nextPreemption;
}

static void IRQsetNextPreemption(long long currentDeadline)
{
    long long first;
    if(sleepingList.empty()) first = std::numeric_limits<long long>::max();
    else first = sleepingList.front()->wakeupTime;

    if(currentDeadline != std::numeric_limits<long long>::max() - 2)
    {
        // RT task (and idle), have no time slice, preempt at next task wakeup
        nextPreemption = first;
    } else {
        // NRT task, set preemption based on time slice and next task wakeup
        nextPreemption = std::min(first, IRQgetTime() + MAX_TIME_SLICE);
    }

    IRQosTimerSetInterrupt(nextPreemption);
}

void EDFScheduler::IRQrunScheduler()
{
    #ifdef WITH_CPU_TIME_COUNTER
    Thread *prev=const_cast<Thread*>(runningThread);
    #endif // WITH_CPU_TIME_COUNTER
    
    // Try to find a ready RT task first
    Thread *walk=head;
    int selected=0;
    while(walk!=nullptr)
    {
        if(walk->flags.isReady())
        {
            selected=1;
            break;
        }
        walk=walk->schedData.next;
    }

    // If no RT tasks are ready, check NRT tasks (round-robin list)
    if(headNRT != nullptr && selected == 0)
    {
        Thread *start = headNRT;
        do {
            if(headNRT->flags.isReady())
            {
                walk = headNRT;
                headNRT = headNRT->schedData.next; // Round-robin: move to next
                selected = 1;
                break;
            }
            headNRT = headNRT->schedData.next;
        } while(headNRT != start);
    }

    // If no RT or NRT tasks are ready, run the idle thread
    if(idle != nullptr && selected == 0)
    {
        walk = idle;
        selected = 1;
    } 

    // if no RT, NRT or idle is available, Error (should never happen)
    if(selected == 0) errorHandler(UNEXPECTED);
   

    runningThread = walk;

    #ifdef WITH_PROCESSES
    if(const_cast<Thread*>(runningThread)->flags.isInUserspace() == false)
    {
        ctxsave=runningThread->ctxsave;
        MPUConfiguration::IRQdisable();
    } else {
        ctxsave=runningThread->userCtxsave;
        //A kernel thread is never in userspace, so the cast is safe
        static_cast<Process*>(runningThread->proc)->mpu.IRQenable();
    }
    #else // WITH_PROCESSES
    ctxsave=runningThread->ctxsave;
    #endif // WITH_PROCESSES

    IRQsetNextPreemption(const_cast<Thread*>(runningThread)->schedData.deadline.get());

    #ifdef WITH_CPU_TIME_COUNTER
    IRQprofileContextSwitch(prev->timeCounterData, walk->timeCounterData,
                            IRQgetTime());
    #endif // WITH_CPU_TIME_COUNTER
}

void EDFScheduler::add(Thread *thread)
{

    long long newDeadline=thread->schedData.deadline.get();

    if(newDeadline == std::numeric_limits<long long>::max()-2) // NRT tasks
    {
        if(headNRT==nullptr)
        {
            headNRT=thread;
            thread->schedData.next=thread;//Circular list
        } else {
            thread->schedData.next=headNRT->schedData.next;
            headNRT->schedData.next=thread;
        }

        return;

    } else if(newDeadline < std::numeric_limits<long long>::max()-2) // RT Tasks
    {
        if(head==nullptr)
        {
            thread->schedData.next=nullptr;
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

}

void EDFScheduler::remove(Thread *thread)
{
    long long deadline = thread->schedData.deadline.get();

    if(deadline == std::numeric_limits<long long>::max() - 2) // NRT tasks
    {
        if(headNRT == nullptr) errorHandler(UNEXPECTED);

        if(headNRT == thread)
        {
            if(headNRT->schedData.next == headNRT)
            {
                // Only one element in the circular list
                headNRT = nullptr;
            } else {
                Thread *tail = headNRT;
                while(tail->schedData.next != headNRT)
                {
                    tail = tail->schedData.next;
                }
                headNRT = headNRT->schedData.next;
                tail->schedData.next = headNRT;
            }
            return;
        }

        Thread *walk = headNRT;
        for(;;)
        {
            if(walk->schedData.next == headNRT) errorHandler(UNEXPECTED);
            if(walk->schedData.next == thread)
            {
                walk->schedData.next = walk->schedData.next->schedData.next;
                break;
            }
            walk = walk->schedData.next;
        }
    }
    else // RT tasks
    {
        if(head == nullptr) errorHandler(UNEXPECTED);
        if(head == thread)
        {
            head = head->schedData.next;
            return;
        }

        Thread *walk = head;
        for(;;)
        {
            if(walk->schedData.next == nullptr) errorHandler(UNEXPECTED);
            if(walk->schedData.next == thread)
            {
                walk->schedData.next = walk->schedData.next->schedData.next;
                break;
            }
            walk = walk->schedData.next;
        }
    }
}

Thread *EDFScheduler::head=nullptr;
Thread *EDFScheduler::headNRT=nullptr;
Thread *EDFScheduler::idle=nullptr;

} //namespace miosix

#endif //SCHED_TYPE_EDF
