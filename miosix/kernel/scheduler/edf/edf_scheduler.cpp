/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
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
#include "kernel/kernel.h"
#include "kernel/error.h"
#include "interfaces-impl/bsp_impl.h"
#include "interfaces/console.h"
#include <algorithm>
#include <limits>

using namespace std;

#ifdef SCHED_TYPE_EDF

namespace miosix {

//These are defined in kernel.cpp
extern volatile Thread *cur;
extern unsigned char kernel_running;

//
// class EDFSchedulerPriority
//

bool EDFSchedulerPriority::validate() const
{
    // Deadlines must be positive, ant this is easy to understand.
    // The reason why numeric_limits<long long>::max()-1 is not allowed, is
    // because it is reserved for the idle thread.
    // Note that numeric_limits<long long>::max() is instead allowed, and
    // is used for thread that have no deadline assigned. In this way their
    // deadline comes after the deadline of the idle thread, and they never run.
    return this->deadline>=0 &&
           this->deadline!=numeric_limits<long long>::max()-1;
}

//
// class EDFScheduler
//

void EDFScheduler::PKaddThread(Thread *thread, EDFSchedulerPriority priority)
{
    thread->schedData.deadline=priority;
    add(thread);
}

bool EDFScheduler::PKexists(Thread *thread)
{
    Thread *walk=head;
    while(walk!=0)
    {
        if(walk==thread && (! (walk->flags.isDeleted()))) return true;
        walk=walk->schedData.next;
    }
    return false;
}

void EDFScheduler::PKremoveDeadThreads()
{
    //Delete all threads at the beginning of the list
    for(;;)
    {
        if(head==0) errorHandler(UNEXPECTED); //Empty list is wrong.
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
        if(walk->schedData.next==0) break;
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

EDFSchedulerPriority EDFScheduler::getPriority(Thread *thread)
{
    return thread->schedData.deadline;
}


EDFSchedulerPriority EDFScheduler::IRQgetPriority(Thread *thread)
{
    return thread->schedData.deadline;
}

void EDFScheduler::IRQsetIdleThread(Thread *idleThread)
{
    idleThread->schedData.deadline=numeric_limits<long long>::max()-1;
    add(idleThread);
}

void EDFScheduler::IRQfindNextThread()
{
    if(kernel_running!=0) return;//If kernel is paused, do nothing
    
    Thread *walk=head;
    for(;;)
    {
        if(walk==0) errorHandler(UNEXPECTED);
        if(walk->flags.isReady())
        {
            cur=walk;
            ctxsave=cur->ctxsave;
            return;
        }
        walk=walk->schedData.next;
    }
}

void EDFScheduler::add(Thread *thread)
{
    long long newDeadline=thread->schedData.deadline.get();
    if(head==0)
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
        if(walk->schedData.next==0 || newDeadline<=
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
    if(head==0) errorHandler(UNEXPECTED);
    if(head==thread)
    {
        head=head->schedData.next;
        return;
    }
    Thread *walk=head;
    for(;;)
    {
        if(walk->schedData.next==0) errorHandler(UNEXPECTED);
        if(walk->schedData.next==thread)
        {
            walk->schedData.next=walk->schedData.next->schedData.next;
            break;
        }
        walk=walk->schedData.next;
    }
}

Thread *EDFScheduler::head=0;

} //namespace miosix

#endif //SCHED_TYPE_EDF
