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

#include "priority_scheduler.h"
#include "kernel/error.h"
#include "kernel/process.h"
#include "interfaces_private/cpu.h"
#include "interfaces_private/os_timer.h"
#include "interfaces_private/smp.h"
#include <limits>

#ifdef SCHED_TYPE_PRIORITY
namespace miosix {

//These are defined in thread.cpp / lock.cpp
extern volatile Thread *runningThread[CPU_NUM_CORES];
extern volatile int kernelRunning;
extern unsigned char globalPkNestLockHoldingCore;
extern volatile bool pendingWakeup;
extern IntrusiveList<SleepData> sleepingList;

//Internal data
static long long nextPeriodicPreemption[CPU_NUM_CORES];

//
// class PriorityScheduler
//

bool PriorityScheduler::PKaddThread(Thread *thread,
        PrioritySchedulerPriority priority)
{
    thread->schedData.priority=priority;
    threadList[priority.get()].push_back(thread);
    return true;
}

bool PriorityScheduler::PKexists(Thread *thread)
{
    for(int i=0;i<CPU_NUM_CORES;i++)
        if(runningThread[i]==thread) return true; //Running thread is not in any list
    for(int i=PRIORITY_MAX-1;i>=0;i--)
        for(auto t : threadList[i]) if(t==thread) return !t->flags.isDeleted();
    return false;
}

void PriorityScheduler::PKremoveDeadThreads()
{
    //TODO: create a separate list for threads to be deleted
    for(int i=PRIORITY_MAX-1;i>=0;i--)
    {
        auto t=threadList[i].begin(), e=threadList[i].end();
        while(t!=e)
        {
            if((*t)->flags.isDeleted())
            {
                void *base=(*t)->watermark;
                (*t)->~Thread();//Call destructor manually because of placement new
                free(base);  //Delete ALL thread memory
                t=threadList[i].erase(t);
            } else ++t;
        }
    }
}

void PriorityScheduler::PKsetPriority(Thread *thread,
        PrioritySchedulerPriority newPriority)
{
    //If thread is running it is not in any list, only change priority value
    for(int i=0;i<CPU_NUM_CORES;i++)
    {
        if(thread==runningThread[i])
        {
            thread->schedData.priority=newPriority;
            return;
        }
    }
    //Thread isn't running, remove the thread from its old list
    threadList[thread->schedData.priority.get()].removeFast(thread);
    //Set priority to the new value
    thread->schedData.priority=newPriority;
    //Last insert the thread in the new list
    threadList[newPriority.get()].push_back(thread);
}

void PriorityScheduler::IRQsetIdleThread(int whichCore, Thread *idleThread)
{
    idleThread->schedData.priority=-1;
    idle[whichCore]=idleThread;
    nextPeriodicPreemption[whichCore]=std::numeric_limits<long long>::max();
}

long long PriorityScheduler::IRQgetNextPreemption()
{
    return nextPeriodicPreemption[getCurrentCoreId()];
}

static long long IRQsetNextPreemption(int coreId, bool runningIdleThread)
{
    //TODO: only set wakeup interrupts on core 1?
    //but need to consider the pause kernel issue below
    long long first;
    if(sleepingList.empty()) first=std::numeric_limits<long long>::max();
    else first=sleepingList.front()->wakeupTime;

    long long t=IRQgetTime();
    if(runningIdleThread) nextPeriodicPreemption[coreId]=first;
    else nextPeriodicPreemption[coreId]=std::min(first,t+MAX_TIME_SLICE);

    //We could not set an interrupt if the sleeping list is empty and runningThread
    //is idle but there's no such hurry to run idle anyway, so why bother?
    IRQosTimerSetInterrupt(nextPeriodicPreemption[coreId]);
    return t;
}

void PriorityScheduler::IRQrunScheduler()
{
    int coreId=getCurrentCoreId();
    bool forceRunIdle=false;
    if(kernelRunning!=0) //If kernel is paused, do nothing
    {
        pendingWakeup=true;
        #ifndef WITH_SMP
        return;
        #else //WITH_SMP
        // In a multi core environment one core can take the pause kernel lock
        // while another core can attempt a context switch. The issue is, we
        // cannot really deny this and forcedly give back the CPU to that thread,
        // as it may be a thread that just terminated and would crash if given
        // back the CPU, or a thread sleeping or waiting for I/O that would crash
        // if given back the CPU before tha wakeup condition occurred.
        // Thus, we switch to the idle thread. Of course we must not do so for
        // the core that is currently holding the pause kernel lock or it would
        // deadlock
        if(globalPkNestLockHoldingCore==coreId) return;
        forceRunIdle=true;
        #endif //WITH_SMP
    }
    //Add the previous thread to the back of the priority list (round-robin)
    Thread *prev=const_cast<Thread*>(runningThread[coreId]);
    int prevPriority=prev->schedData.priority.get();
    if(prevPriority!=-1) threadList[prevPriority].push_back(prev);
    if(forceRunIdle==false)
    {
        for(int i=PRIORITY_MAX-1;i>=0;i--)
        {
            for(auto next : threadList[i])
            {
                if(next->flags.isReady()==false) continue;
                //Found a READY thread, so run this one
                runningThread[coreId]=next;
                #ifdef WITH_PROCESSES
                if(next->flags.isInUserspace()==false)
                {
                    ctxsave[coreId]=next->ctxsave;
                    MPUConfiguration::IRQdisable();
                } else {
                    ctxsave[coreId]=next->userCtxsave;
                    //A kernel thread is never in userspace, so the cast is safe
                    static_cast<Process*>(next->proc)->mpu.IRQenable();
                }
                #else //WITH_PROCESSES
                ctxsave[coreId]=next->ctxsave;
                #endif //WITH_PROCESSES
                #ifndef WITH_CPU_TIME_COUNTER
                IRQsetNextPreemption(coreId,false);
                #else //WITH_CPU_TIME_COUNTER
                auto t=IRQsetNextPreemption(coreId,false);
                IRQprofileContextSwitch(prev->timeCounterData,next->timeCounterData,t);
                #endif //WITH_CPU_TIME_COUNTER
                //Remove the selected thread from the list. This invalidates
                //iterators sho it should be done last
                threadList[i].removeFast(next);
                #ifdef WITH_SMP
                //TODO also reschedule if other core has lower priority
                if(runningThread[1-coreId]==idle[1-coreId])
                    IRQcallOnCore(1-coreId,reinterpret_cast<void (*)(void*)>(IRQinvokeScheduler),nullptr);
                #endif //WITH_SMP
                return;
            }
        }
    }
    //No thread found, run the idle thread
    runningThread[coreId]=idle[coreId];
    ctxsave[coreId]=idle[coreId]->ctxsave;
    #ifdef WITH_PROCESSES
    MPUConfiguration::IRQdisable();
    #endif //WITH_PROCESSES
    #ifndef WITH_CPU_TIME_COUNTER
    IRQsetNextPreemption(coreId,true);
    #else //WITH_CPU_TIME_COUNTER
    auto t=IRQsetNextPreemption(coreId,true);
    IRQprofileContextSwitch(prev->timeCounterData,idle->timeCounterData,t);
    #endif //WITH_CPU_TIME_COUNTER
}

IntrusiveList<Thread> PriorityScheduler::threadList[PRIORITY_MAX];
Thread *PriorityScheduler::idle[CPU_NUM_CORES]={nullptr};

} //namespace miosix

#endif //SCHED_TYPE_PRIORITY
