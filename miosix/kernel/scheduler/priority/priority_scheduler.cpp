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
#include "kernel/cpu_time_counter.h"
#include <limits>

using namespace std;

#ifdef SCHED_TYPE_PRIORITY
namespace miosix {

//These are defined in thread.cpp
extern volatile Thread *runningThreads[CPU_NUM_CORES];
extern IntrusiveList<SleepData> sleepingList;

//
// class PriorityScheduler
//

bool PriorityScheduler::IRQaddThread(Thread *thread,
        PrioritySchedulerPriority priority)
{
    thread->schedData.priority=priority;
    #ifdef CONDVAR_WAKEUP_BY_PRIORITY
    thread->savedPriority=priority;
    #endif //CONDVAR_WAKEUP_BY_PRIORITY
    #ifdef WITH_PROCESSES
    // Check isReady() as processes are initially created in not ready state
    if(thread->flags.isReady()==false) notReadyThreads.push_front(thread);
    else
    #endif //WITH_PROCESSES
    readyThreads[priority.get()].push_back(thread);
    return true;
}

bool PriorityScheduler::IRQexists(Thread *thread)
{
    for(int i=0;i<CPU_NUM_CORES;i++)
        if(runningThreads[i]==thread) return !thread->flags.isDeleted();
    for(int i=NUM_PRIORITIES-1;i>=0;i--)
        for(auto t : readyThreads[i]) if(t==thread) return true;
    for(auto t : notReadyThreads) if(t==thread) return !thread->flags.isDeleted();
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
    //if(thread->flags.isZombie()) errorHandler(Error::UNEXPECTED);
    if(thread->flags.isReady()==false) notReadyThreads.push_front(thread);
    else readyThreads[newPriority.get()].push_back(thread);
}

void PriorityScheduler::IRQsetIdleThread(int whichCore, Thread *idleThread)
{
    idleThread->schedData.priority=-1;
    #ifdef CONDVAR_WAKEUP_BY_PRIORITY
    idleThread->savedPriority=-1;
    #endif //CONDVAR_WAKEUP_BY_PRIORITY
    idle[whichCore]=idleThread;
}

void PriorityScheduler::IRQwokenThread(Thread* thread)
{
    // NOTE: this check is necessary as there is the corner case of a thread
    // that has just set itself to sleeping/waiting but it gets woken up before
    // the scheduler has a chance to run. Thus it is both awakened and running,
    // and we must not call notReadyThreads.removeFast(thread) if it's not in
    // that list as it causes undefined behavior
    for(int i=0;i<CPU_NUM_CORES;i++) if(runningThreads[i]==thread) return;
    notReadyThreads.removeFast(thread);
    readyThreads[thread->schedData.priority.get()].push_back(thread);
}

#ifdef WITH_SMP
void PriorityScheduler::IRQrunScheduler(unsigned char coreId)
{
    IRQrunSchedulerImpl(coreId);
}
#else
void PriorityScheduler::IRQrunScheduler()
{
    // Thanks to the magic of compilers this should elide a lot of pointer
    // arithmetic
    IRQrunSchedulerImpl(0);
}
#endif

inline void PriorityScheduler::IRQrunSchedulerImpl(unsigned char coreId)
{
    //If the previously running thread is not idle, we need to put it in a list
    Thread *prev=const_cast<Thread*>(runningThreads[coreId]);
    if(prev!=idle[coreId])
    {
        // NOTE: notReadyThreads must be pushed back if deleted, front if not
        // while if ready always back (round-robin)
        if(prev->flags.isZombie()) notReadyThreads.push_back(prev);
        else if(prev->flags.isReady()==false) notReadyThreads.push_front(prev);
        else readyThreads[prev->schedData.priority.get()].push_back(prev);
    }
    for(int i=NUM_PRIORITIES-1;i>=0;i--)
    {
        if(readyThreads[i].empty()) continue;
        Thread *t=readyThreads[i].front();
        readyThreads[i].pop_front(); //Remove selected thread from list
        runningThreads[coreId]=t;
        #ifdef WITH_PROCESSES
        if(t->flags.isInUserspace()==false)
        {
            ctxsave[coreId]=t->ctxsave;
            MPUConfiguration::IRQdisable();
        } else {
            ctxsave[coreId]=t->userCtxsave;
            //A kernel thread is never in userspace, so the cast is safe
            static_cast<Process*>(t->proc)->mpu.IRQenable();
        }
        #else //WITH_PROCESSES
        ctxsave[coreId]=t->ctxsave;
        #endif //WITH_PROCESSES
        #ifndef WITH_CPU_TIME_COUNTER
        IRQcomputePreemption(coreId,false);
        #else //WITH_CPU_TIME_COUNTER
        auto now=IRQcomputePreemption(coreId,false);
        CPUTimeCounter::IRQprofileContextSwitch(prev,t,now,coreId);
        #endif //WITH_CPU_TIME_COUNTER
        #ifdef WITH_SMP
        // In case multiple tasks are woken at the same time, we may
        // have to schedule more than one higher priority thread than
        // currently running. When this happens, we need to call the
        // scheduler again on more than one core
        signed char minRunningPriority=
            const_cast<Thread*>(runningThreads[0])->schedData.priority.get();
        int coreRunningMinPriorityThread=0;
        for(int j=1;j<CPU_NUM_CORES;j++)
        {
            signed char p=
                const_cast<Thread*>(runningThreads[j])->schedData.priority.get();
            if(p<minRunningPriority)
            {
                minRunningPriority=p;
                coreRunningMinPriorityThread=j;
            }
        }
        // This is a loop in a loop with the same variable to continue from
        // where we left, but we'll never go back to the outer loop
        for(;i>=0;i--)
        {
            if(i<=minRunningPriority) break;
            if(readyThreads[i].empty()) continue;
            IRQinvokeSchedulerOnCore(coreRunningMinPriorityThread);
            break;
        }
        #endif //WITH_SMP
        return;
    }
    //No thread found, run the idle thread
    runningThreads[coreId]=idle[coreId];
    ctxsave[coreId]=idle[coreId]->ctxsave;
    #ifdef WITH_PROCESSES
    MPUConfiguration::IRQdisable();
    #endif //WITH_PROCESSES
    #ifndef WITH_CPU_TIME_COUNTER
    IRQcomputePreemption(coreId,true);
    #else //WITH_CPU_TIME_COUNTER
    auto now=IRQcomputePreemption(coreId,true);
    CPUTimeCounter::IRQprofileContextSwitch(prev,idle[coreId],now,coreId);
    #endif //WITH_CPU_TIME_COUNTER
}

long long PriorityScheduler::IRQcomputePreemption(int coreId, bool runningIdleThread)
{
    long long firstWakeup;
    if(sleepingList.empty()) firstWakeup=numeric_limits<long long>::max();
    else firstWakeup=sleepingList.front()->wakeupTime;

    long long t=IRQgetTime(), nextPreempt;
    if(runningIdleThread==false) nextPreempt=t+MAX_TIME_SLICE;
    else nextPreempt=numeric_limits<long long>::max();

    // We could avoid setting an interrupt if the sleeping list is empty and
    // runningThreads[coreId] is idle but there's no such hurry to run idle
    // anyway, so why bother?
    #ifdef WITH_SMP
    if(coreId!=WAKEUP_HANDLING_CORE)
    {
        // IRQosTimerSetPreemption is to be used by all cores that are not
        // WAKEUP_HANDLING_CORE
        IRQosTimerSetPreemption(nextPreempt);
        // NOTE: even if we're not on the WAKEUP_HANDLING_CORE, the thread we
        // just preempted may have started a sleep whose wakeup is earlier than
        // any other sleep, thus we should check and modify the preemption of
        // the WAKEUP_HANDLING_CORE
        if(firstWakeup<nextPreemptionWakeupCore)
            IRQosTimerSetInterrupt(firstWakeup);
    } else {
        nextPreemptionWakeupCore=nextPreempt;
        IRQosTimerSetInterrupt(min(firstWakeup,nextPreempt));
    }
    #else //WITH_SMP
    nextPreemptionWakeupCore=nextPreempt;
    IRQosTimerSetInterrupt(min(firstWakeup,nextPreempt));
    #endif //WITH_SMP
    return t;
}

long long PriorityScheduler::nextPreemptionWakeupCore=numeric_limits<long long>::max();
IntrusiveList<Thread> PriorityScheduler::readyThreads[NUM_PRIORITIES];
IntrusiveList<Thread> PriorityScheduler::notReadyThreads;
Thread *PriorityScheduler::idle[CPU_NUM_CORES]={nullptr};

} //namespace miosix

#endif //SCHED_TYPE_PRIORITY
