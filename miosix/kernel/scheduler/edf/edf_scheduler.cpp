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

#include "edf_scheduler.h"
#include "kernel/error.h"
#include "kernel/process.h"
#include "interfaces_private/cpu.h"
#include "interfaces_private/os_timer.h"
#include "interfaces_private/smp.h"
#include <algorithm>

using namespace std;

#ifdef SCHED_TYPE_EDF

namespace miosix {

//These are defined in thread.cpp
extern volatile Thread *runningThreads[CPU_NUM_CORES];
extern TimeSortedQueue<SleepToken,GetWakeupTime> sleepingList;

//
// class EDFScheduler
//

bool EDFScheduler::IRQaddThread(Thread *thread, EDFSchedulerPriority priority)
{
    thread->schedData.deadline=priority;
    //Priority and savedPriority must be the same except when locking a mutex
    //with priority inheritance. A newly created thread isn't yet locking mutex
    thread->savedPriority=priority;
    #ifdef WITH_PROCESSES
    // Check isReady() as processes are initially created in not ready state
    if(thread->flags.isReady()==false) notReadyThreads.push_front(thread);
    else
    #endif //WITH_PROCESSES
    if(priority.get()==numeric_limits<long long>::max()-2)
        readyRrThreads.enqueue(thread);
    else readyEdfThreads.enqueue(thread);
    return true;
}

bool EDFScheduler::IRQexists(Thread *thread)
{
    for(int i=0;i<CPU_NUM_CORES;i++)
        if(runningThreads[i]==thread) return !thread->flags.isDeleted();
    if(readyRrThreads.contains(thread)) return true;
    if(readyEdfThreads.contains(thread)) return true;
    for(auto t : notReadyThreads) if(t==thread) return !thread->flags.isDeleted();
    return false;
}

void EDFScheduler::removeDeadThreads()
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

void EDFScheduler::IRQsetPriority(Thread *thread, EDFSchedulerPriority newPriority)
{
    //if(thread->flags.isZombie()) errorHandler(Error::UNEXPECTED);
    // If thread is running it is not in any list, only change priority value
    for(int i=0;i<CPU_NUM_CORES;i++)
    {
        if(thread==runningThreads[i])
        {
            thread->schedData.deadline=newPriority;
            return;
        }
    }
    // If thread is not ready it will remain in the notReadyThreads list,
    // only change priority value
    if(thread->flags.isReady()==false)
    {
        thread->schedData.deadline=newPriority;
        return;
    }
    bool oldRR=thread->schedData.deadline.get()==numeric_limits<long long>::max()-2;
    bool newRR=newPriority==numeric_limits<long long>::max()-2;
    // Ready threads are harder because of two constraints:
    // 1) Changing priority may switch a thread from real-time (with deadline,
    //    scheduled with EDF) to non-real-time (no deadline, scheduled with RR)
    //    and in this case it needs to switch queue
    // 2) Even if a thread is and remains real-time, it needs to be removed
    //    from the EDF queue and be re-inserted with the new priority (deadline)
    //    to keep it sorted
    // Thus a thread will need to be removed/re-inserted unless it is and will
    // remain non-real-time
    if(oldRR==true || newRR==true)
    {
        thread->schedData.deadline=newPriority;
        return;
    }
    // Remove from old queue
    if(oldRR) readyRrThreads.remove(thread);
    else readyEdfThreads.remove(thread);
    // Set priority to the new value
    thread->schedData.deadline=newPriority;
    // After priority changed, can insert the thread in the new queue
    if(newRR) readyRrThreads.enqueue(thread);
    else readyEdfThreads.enqueue(thread);
}

void EDFScheduler::IRQsetIdleThread(int whichCore, Thread *idleThread)
{
    idleThread->schedData.deadline=numeric_limits<long long>::max()-1;
    idleThread->savedPriority=numeric_limits<long long>::max()-1;
    idle[whichCore]=idleThread;
}

void EDFScheduler::IRQwokenThread(Thread* thread)
{
    // NOTE: this check is necessary as there is the corner case of a thread
    // that has just set itself to sleeping/waiting but it gets woken up before
    // the scheduler has a chance to run. Thus it is both awakened and running,
    // and we must not call notReadyThreads.removeFast(thread) if it's not in
    // that list as it causes undefined behavior
    for(int i=0;i<CPU_NUM_CORES;i++) if(runningThreads[i]==thread) return;
    notReadyThreads.removeFast(thread);
    if(thread->schedData.deadline.get()==numeric_limits<long long>::max()-2)
        readyRrThreads.enqueue(thread);
    else readyEdfThreads.enqueue(thread);
}

/*
 * Please read the comment in the priority scheduler for the general overview
 * of the Miosix 3.0 scheduler operation.
 * Compared to the priority scheduler the EDF scheduler is actually a
 * combination of two schedulers in one: the actual EDF scheduler and a
 * Round Robin (RR) scheduler. The reason for this choice is due to the fact
 * that not all threads may be real-time threads, and it's hard to assign a
 * deadline value to non-real-time threads without causing side effect such as
 * "priority inversion". This was the main usability issue of the EDF scheduler
 * in older Miosix versions, and it has been overcome by scheduling
 * non-real-time threads separately using RR.
 * With this scheduler, every thread is assigned a priority which is a 64 bit
 * number, to be set with the Thread::setPriority() API.
 * The special priority value std::numeric_limits<long long>::max()-2 known as
 * DEFAULT_PRIORITY as defined in miosix_settings.h corresponds to non-real-time
 * threads which are scheduled using RR.
 * Any other priority value is interpreted as a deadline expressed in absolute
 * time and makes the thread real-time and scheduled with EDF instead of RR.
 * This scheduler will always schedule real-time threads before non-real-time
 * threads, the RR scheduler only works when all real-time threads are blocked.
 * Since deadlines are absolute times, a thread must constantly update the
 * deadline value by calling Thread::setPriority() as soon as it has completed
 * its workload (hopefully the task pool is schedulable, and this will occur
 * before the deadline expired).
 * This approach can flexibly support periodic tasks (deadline is updated at
 * every period to be the end of the next period), aperiodic tasks (just change
 * the period on-the-fly when updating the deadline), sporadic tasks (usually
 * waiting for an event, when the event arrives, they are woken and a deadline
 * is set) as well as threads that switch between real-time and non-real-time
 * (just calling Thread::setPriority(DEFAULT_PRIORITY) will switch the thread
 * from being a real-time thread to a non-real-time one).
 * Unlike other operating systems, the EDF scheduler in Miosix will never
 * preempt the thread with the earliest deadline, even if the deadline is missed
 * and now in the past. A thread waking up with a deadline in the past will
 * also be scheduled immediately (unless another thread with a deadline even
 * more in the past is running). This design makes it possible to use EDF beyond
 * periodic tasks, but an off-line schedulability analysis is suggested to make
 * sure the task pool is schedulable.
 */

#ifdef WITH_SMP
void EDFScheduler::IRQrunScheduler(unsigned char coreId)
{
#else //WITH_SMP
void EDFScheduler::IRQrunScheduler()
{
    constexpr unsigned char coreId=0;
#endif //WITH_SMP
    // If the previously running thread is not idle, we need to put it in a list
    Thread *prev=const_cast<Thread*>(runningThreads[coreId]);
    if(prev!=idle[coreId])
    {
        // NOTE: notReadyThreads must be pushed back if deleted, front if not
        if(prev->flags.isZombie()) notReadyThreads.push_back(prev);
        else if(prev->flags.isReady()==false) notReadyThreads.push_front(prev);
        else if(prev->schedData.deadline.get()==numeric_limits<long long>::max()-2)
            readyRrThreads.enqueue(prev);
        else readyEdfThreads.enqueue(prev);
    }

    // Try to find a ready real-time thread first.
    // If not found, try to find a ready non-real-time thread
    #if defined(WITH_THREAD_AFFINITY) && defined(WITH_SMP)
    // Cache the deadline of all running threads in an array. Note that we're
    // interested in the deadlines after the scheduler is run but we still
    // don't what the deadline will be on the current core as we haven't
    // done the scheduling yet. However, since this variable is used to decide
    // if we need to invoke the scheduler on another core we just lie and set
    // the deadline of the current core to 0 value so as to always exclude the
    // current core
    long long runningDeadline[CPU_NUM_CORES];
    for(int c=0;c<CPU_NUM_CORES;c++)
        runningDeadline[c]=const_cast<Thread*>(runningThreads[c])->schedData.deadline.get();
    runningDeadline[coreId]=0;
    int scheduleOnOtherCore=-1;
    // If the kernel is compiled with affinity support we can't just pick
    // the first thread in the ready list, we need to check the affinity
    Thread *next=nullptr;
    for(auto it=begin(readyEdfThreads);it!=end(readyEdfThreads);++it)
    {
        auto affinity=(*it)->affinity;
        if(affinity & (1<<coreId))
        {
            // Found highest priority thread whose affinity is compatible
            // with this core. That's the one we'll schedule
            next=*it;
            readyEdfThreads.erase(it);
            break;
        } else {
            // Found thread that can't run on this core due to affinity.
            // On the cores it can run it may however preempt the currently
            // running thread. We only need to find one such thread though
            // as if there are more, they will be discovered when the
            // scheduler is called on the other core (distributed algorithm)
            if(scheduleOnOtherCore>=0) continue;
            long long deadline=(*it)->schedData.deadline.get();
            for(int c=0;c<CPU_NUM_CORES;c++)
            {
                if((affinity & (1<<c))==0) continue;
                if(runningDeadline[c]<deadline) continue;
                scheduleOnOtherCore=c;
                break;
            }
        }
    }
    if(next==nullptr)
    {
        for(auto it=begin(readyRrThreads);it!=end(readyRrThreads);++it)
        {
            auto affinity=(*it)->affinity;
            if(affinity & (1<<coreId))
            {
                // Found highest priority thread whose affinity is compatible
                // with this core. That's the one we'll schedule
                next=*it;
                readyRrThreads.erase(it);
                break;
            } else {
                // Non-real-time threads can only preempt idle
                if(scheduleOnOtherCore>=0) continue;
                for(int c=0;c<CPU_NUM_CORES;c++)
                {
                    if((affinity & (1<<c))==0) continue;
                    if(runningDeadline[c]!=numeric_limits<long long>::max()-1)
                        continue;
                    scheduleOnOtherCore=c;
                    break;
                }
            }
        }
    }
    #else //defined(WITH_THREAD_AFFINITY) && defined(WITH_SMP)
    Thread *next=readyEdfThreads.dequeueOne();
    if(next==nullptr) next=readyRrThreads.dequeueOne();
    #endif //defined(WITH_THREAD_AFFINITY) && defined(WITH_SMP)
    //Otherwise, run idle
    if(next==nullptr) next=idle[coreId];

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
    IRQcomputePreemption(coreId,next->schedData.deadline.get());
    #else //WITH_CPU_TIME_COUNTER
    auto now=IRQcomputePreemption(coreId,next->schedData.deadline.get());
    CPUTimeCounter::IRQprofileContextSwitch(prev,next,now,coreId);
    #endif //WITH_CPU_TIME_COUNTER

    #ifdef WITH_SMP
    // In case multiple threads are woken at the same time, we may have to
    // schedule more than one earlier deadline thread than currently running.
    // When this happens, we need to call the scheduler again on more than
    // one core. Additionally, if compiling with thread affinity we may have
    // already found that we need to invoke the scheduler on another core
    // because an earliest deadline thread wasn't selected on this core due to
    // incompatible affinity. In this case skip this algorithm as we don't
    // need to solve the schedule for all cores, just figuring out that
    // it's wrong one core is enough. Then the invoked scheduler on that
    // core will complete the job and figure out if there is the need to
    // reschedule on yet another core
    #ifdef WITH_THREAD_AFFINITY
    // With affinity, see if we find a compatible thread with earlier deadline
    if(scheduleOnOtherCore<0)
    {
        long long latestRunningDeadline=runningDeadline[0];
        for(int c=1;c<CPU_NUM_CORES;c++)
            latestRunningDeadline=max(latestRunningDeadline,runningDeadline[c]);

        for(auto it=begin(readyEdfThreads);it!=end(readyEdfThreads);++it)
        {
            long long deadline=(*it)->schedData.deadline.get();
            if(deadline>=latestRunningDeadline) goto checkCompleted;
            auto affinity=(*it)->affinity;
            for(int c=0;c<CPU_NUM_CORES;c++)
            {
                if((affinity & (1<<c))==0) continue;
                if(runningDeadline[c]<deadline) continue;
                IRQinvokeSchedulerOnCore(c);
                goto checkCompleted;
            }
        }
        for(auto it=begin(readyRrThreads);it!=end(readyRrThreads);++it)
        {
            auto affinity=(*it)->affinity;
            // Non-real-time threads can only preempt idle
            for(int c=0;c<CPU_NUM_CORES;c++)
            {
                if((affinity & (1<<c))==0) continue;
                if(runningDeadline[c]!=numeric_limits<long long>::max()-1)
                    continue;
                IRQinvokeSchedulerOnCore(c);
                goto checkCompleted;
            }
        }
        checkCompleted:;
    } else IRQinvokeSchedulerOnCore(scheduleOnOtherCore);
    #else //WITH_THREAD_AFFINITY
    // No affinity, just knowing a ready thread exists with an eraliest deadline
    // is enough, it can surely be running on any core
    long long secondEarliestDeadline=numeric_limits<long long>::max()-1;
    if(readyEdfThreads.empty()==false)
        secondEarliestDeadline=readyEdfThreads.front()->schedData.deadline.get();
    else if(readyRrThreads.empty()==false)
        secondEarliestDeadline=numeric_limits<long long>::max()-2;
    if(secondEarliestDeadline!=numeric_limits<long long>::max()-1)
    {
        for(int c=0;c<CPU_NUM_CORES;c++)
        {
            if(secondEarliestDeadline<const_cast<Thread*>(runningThreads[c])->
                schedData.deadline.get())
            {
                IRQinvokeSchedulerOnCore(c);
                break;
            }
        }
    }
    #endif //WITH_THREAD_AFFINITY
    #endif //WITH_SMP
}

long long EDFScheduler::IRQcomputePreemption(unsigned char coreId, long long currentDeadline)
{
    long long firstWakeup;
    if(sleepingList.empty()) firstWakeup=numeric_limits<long long>::max();
    else firstWakeup=sleepingList.front()->wakeupTime;

    long long t=IRQgetTime(), nextPreempt;
    // Real-time threads (and idle), have no time slice
    if(currentDeadline!=std::numeric_limits<long long>::max()-2)
        nextPreempt=numeric_limits<long long>::max();
    else nextPreempt=t+MAX_TIME_SLICE;

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

long long EDFScheduler::nextPreemptionWakeupCore=numeric_limits<long long>::max();
TimeSortedQueue<Thread,EDFScheduler::GetTime> EDFScheduler::readyEdfThreads;
FifoQueue<Thread> EDFScheduler::readyRrThreads;
IntrusiveList<Thread> EDFScheduler::notReadyThreads;
Thread *EDFScheduler::idle[CPU_NUM_CORES]={nullptr};

} //namespace miosix

#endif //SCHED_TYPE_EDF
