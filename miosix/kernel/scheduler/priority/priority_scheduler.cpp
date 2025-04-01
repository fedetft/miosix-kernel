/***************************************************************************
 *   Copyright (C) 2010-2025 by Terraneo Federico                          *
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
extern volatile Thread *runningThreads[CPU_NUM_CORES];
extern volatile int pauseKernelNesting;
extern unsigned char globalPkNestLockHoldingCore;
extern volatile bool pendingWakeup;
extern IntrusiveList<SleepData> sleepingList;

//
// class PriorityScheduler
//

bool PriorityScheduler::IRQaddThread(Thread *thread,
        PrioritySchedulerPriority priority)
{
    thread->schedData.priority=priority;
    readyThreads[priority.get()].push_back(thread);
    return true;
}

bool PriorityScheduler::IRQexists(Thread *thread)
{
    for(int i=0;i<CPU_NUM_CORES;i++)
        if(runningThreads[i]==thread) return true; //Running threads are not in list
    for(int i=PRIORITY_MAX-1;i>=0;i--)
        for(auto t : readyThreads[i]) if(t==thread) return true;
    for(auto t : notReadyThreads) if(t==thread) return !t->flags.isDeleted();
    return false;
}

void PriorityScheduler::removeDeadThreads()
{
    for(;;)
    {
        Thread *t;
        {
            FastGlobalIrqLock dLock;
            if(notReadyThreads.empty()) return;
            t=notReadyThreads.back();
            // All deleted threads are at the bottom of the list, so the first
            // not deleted we found means there are no more
            if(t->flags.isDeleted()==false) return;
            notReadyThreads.pop_back();
        }
        //Optimization: don't keep the lock while thread is being deleted
        void *base=t->watermark;
        t->~Thread();//Call destructor manually because of placement new
        free(base);  //Delete ALL thread memory
    }
}

void PriorityScheduler::IRQsetPriority(Thread *thread,
        PrioritySchedulerPriority newPriority)
{
    if(thread->flags.isDeleted()) errorHandler(UNEXPECTED); //TODO remove
    // If thread is running it is not in any list, only change priority value
    for(int i=0;i<CPU_NUM_CORES;i++)
    {
        if(thread==runningThreads[i])
        {
            thread->schedData.priority=newPriority;
            return;
        }
    }
    // Thread isn't running, remove the thread from its old list
    if(thread->flags.isReady()==false) notReadyThreads.removeFast(thread);
    else readyThreads[thread->schedData.priority.get()].removeFast(thread);
    // Set priority to the new value
    thread->schedData.priority=newPriority;
    // Last insert the thread in the new list
    // NOTE: notReadyThreads must be pushed front to keep invariant
    if(thread->flags.isReady()==false) notReadyThreads.push_front(thread);
    else readyThreads[newPriority.get()].push_back(thread);
}

void PriorityScheduler::IRQsetIdleThread(int whichCore, Thread *idleThread)
{
    idleThread->schedData.priority=-1;
    idle[whichCore]=idleThread;
    nextPreemption[whichCore]=std::numeric_limits<long long>::max();
}

void PriorityScheduler::IRQwokenThread(Thread* thread)
{
    //TODO: understand why can this happen, how can a thread transition to ready
    //while being running
    for(int i=0;i<CPU_NUM_CORES;i++)
        if(runningThreads[i]==thread) return;

    notReadyThreads.removeFast(thread);
    readyThreads[thread->schedData.priority.get()].push_back(thread);
}

void PriorityScheduler::IRQrunScheduler()
{
    int coreId=getCurrentCoreId();
    bool forceRunIdle=false;
    if(pauseKernelNesting!=0) //If kernel is paused, preemption is disabled
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
    //If the previously running thread is not idle, we need to put it in a list
    Thread *prev=const_cast<Thread*>(runningThreads[coreId]);
    if(prev!=idle[coreId])
    {
        // NOTE: notReadyThreads must be pushed back if deleted, front if not
        // while if ready always back (round-robin)
        if(prev->flags.isDeleted()) notReadyThreads.push_back(prev);
        else if(prev->flags.isReady()==false) notReadyThreads.push_front(prev);
        else readyThreads[prev->schedData.priority.get()].push_back(prev);
    }
    if(forceRunIdle==false)
    {
        for(int i=PRIORITY_MAX-1;i>=0;i--)
        {
            for(auto next : readyThreads[i])
            {
                if(next->flags.isReady()==false) continue;
                //Found a READY thread, so run this one
                runningThreads[coreId]=next;
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
                //iterators so it should be done last
                readyThreads[i].removeFast(next);
                #ifdef WITH_SMP
                //TODO relax dual-core assumption
                //TODO maybe check if there's a ready thread that can run
                if(const_cast<Thread*>(runningThreads[1-coreId])->IRQgetPriority()<i)
                    IRQinvokeSchedulerOnCore(1-coreId);
                #endif //WITH_SMP
                return;
            }
        }
    }
    //No thread found, run the idle thread
    runningThreads[coreId]=idle[coreId];
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

long long PriorityScheduler::IRQsetNextPreemption(int coreId, bool runningIdleThread)
{
    //TODO: only set wakeup interrupts on core 1?
    //but need to consider the pause kernel issue below
    long long firstWakeup;
    if(sleepingList.empty()) firstWakeup=std::numeric_limits<long long>::max();
    else firstWakeup=sleepingList.front()->wakeupTime;

    long long t=IRQgetTime(), nextPreempt;
    if(runningIdleThread==false) nextPreempt=t+MAX_TIME_SLICE;
    else nextPreempt=std::numeric_limits<long long>::max();
    nextPreemption[coreId]=nextPreempt;

    // We could avoid setting an interrupt if the sleeping list is empty and
    // runningThreads[coreId] is idle but there's no such hurry to run idle
    // anyway, so why bother?
    IRQosTimerSetInterrupt(std::min(firstWakeup,nextPreempt));
    return t;
}

IntrusiveList<Thread> PriorityScheduler::readyThreads[PRIORITY_MAX];
IntrusiveList<Thread> PriorityScheduler::notReadyThreads;
Thread *PriorityScheduler::idle[CPU_NUM_CORES]={nullptr};
long long PriorityScheduler::nextPreemption[CPU_NUM_CORES];

} //namespace miosix

#endif //SCHED_TYPE_PRIORITY
