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

#pragma once

#include "config/miosix_settings.h"
#include "interfaces/cpu_const.h"
#include "edf_scheduler_types.h"
#include "kernel/thread.h"
#include "kernel/sched_data_structures.h"

#ifdef SCHED_TYPE_EDF

namespace miosix {

/**
 * \internal
 * EDF based scheduler.
 */
class EDFScheduler
{
public:
    /**
     * \internal
     * Add a new thread to the scheduler.
     * This is called when a thread is created.
     * \param thread a pointer to a valid thread instance.
     * The behaviour is undefined if a thread is added multiple timed to the
     * scheduler, or if thread is nullptr.
     * \param priority the priority of the new thread.
     * Priority must be a positive value.
     * Note that the meaning of priority is scheduler specific.
     * \return false if an error occurred and the thread could not be added to
     * the scheduler
     *
     * Note: this member function is called also before the kernel is started
     * to add the main and idle thread.
     */
    static bool IRQaddThread(Thread *thread, EDFSchedulerPriority priority);

    /**
     * \internal
     * \return true if thread exists, false if does not exist or has been
     * deleted. A joinable thread is considered existing until it has been
     * joined, even if it returns from its entry point (unless it is detached
     * and terminates).
     */
    static bool IRQexists(Thread *thread);

    /**
     * \internal
     * Called when there is at least one dead thread to be removed from the
     * scheduler
     */
    static void removeDeadThreads();

    /**
     * \internal
     * Set the priority of a thread.
     * Note that the meaning of priority is scheduler specific.
     * \param thread thread whose priority needs to be changed.
     * \param newPriority new thread priority.
     * Priority must be a positive value.
     */
    static void IRQsetPriority(Thread *thread, EDFSchedulerPriority newPriority);

    /**
     * \internal
     * Get the priority of a thread. Must be callable also with kernel paused
     * or IRQ disabled.
     * Note that the meaning of priority is scheduler specific.
     * \param thread thread whose priority needs to be queried.
     * \return the priority of thread.
     */
    static EDFSchedulerPriority getPriority(Thread *thread)
    {
        return thread->schedData.deadline;
    }

    /**
     * \internal
     * This is called before the kernel is started by the kernel. The given
     * thread is the idle thread, to be run all the times where no other thread
     * can run.
     * \param whichCore either 0 on single core platforms, or specify for which
     * core this idle thread is meant to be used. Note that it is expected that
     * during boot exactly one idle thread for each core is given to the
     * scheduler
     */
    static void IRQsetIdleThread(int whichCore, Thread *idleThread);

    /**
     * \internal
     * This member function is called by the kernel every time a thread changes
     * its running status. For example when a thread become sleeping, waiting,
     * deleted or if it exits the sleeping or waiting status
     */
    static void IRQwaitStatusHook(Thread *thread) {}

    /**
     * \internal
     * Called when a thread transitions from waiting/sleeping to ready.
     * Must not be called if the thread is already ready.
     */
    static void IRQwokenThread(Thread* thread);

    /**
     * \internal
     * This function is used only by the kernel code to run the scheduler.
     * It finds the next thread in READY status. If the kernel is paused,
     * does nothing. It's behaviour is to modify the global variable
     * miosix::runningThread which always points to the currently running thread.
     */
    #ifdef WITH_SMP
    static void IRQrunScheduler(unsigned char coreId);
    #else //WITH_SMP
    static void IRQrunScheduler();
    #endif //WITH_SMP

    /**
     * \internal
     * On single core CPUs, the hardware timer is set considering both the
     * earliest thread wakeup from sleep and and the end of the time quantum
     * (preemption) of the currently running thread. This function returns the
     * next preemption time, if any, of the currently running thread. If there
     * are threads with a wakeup time earlier than the next preemption, they are
     * not considered by this function that only returns the next preemption.
     *
     * On multi core CPUs this function returns the next preemption time for the
     * WAKEUP_HANDLING_CORE, the only core that performs the wakeup from sleep
     * logic. All other cores only set their timer to schedule preemptions, and
     * there is currently no getter for this value.
     *
     * \return the next scheduled preemption set by the scheduler for the
     * WAKEUP_HANDLING_CORE.
     * In case no preemption is set returns numeric_limits<long long>::max()
     */
    static long long IRQgetNextPreemption()
    {
        return nextPreemptionWakeupCore;
    }
    
private:
    /**
     * \param coreId id of the core the preemption needs to be set for
     * \param currentDeadline deadline of the thread that will be scheduled next
     * \return the current time in nanoseconds
     */
    static long long IRQcomputePreemption(unsigned char coreId, long long currentDeadline);

    ///\internal On single core CPUs, end of time quantum (preemption) for the
    ///only core. On multi core CPUs, end of time quantum for WAKEUP_HANDLING_CORE
    static long long nextPreemptionWakeupCore;

    /**
     * Functor needed by TimeSortedQueue to insert the thread in the correct
     * place in the waiting list based on its deadline
     */
    struct GetTime
    {
        long long operator()(Thread *thread)
        {
            return thread->schedData.deadline.get();
        }
    };

    ///\internal Deadline-sorted queue of ready threads with deadline (EDF-scheduled)
    static TimeSortedQueue<Thread,EDFScheduler::GetTime> readyEdfThreads;

    ///\internal FIFO queue of ready threads without deadline (RR scheduled)
    static FifoQueue<Thread> readyRrThreads;

    ///\internal List of threads that are not ready.
    ///Keep the invariant that deleted threads are pushed to the back!
    static IntrusiveList<Thread> notReadyThreads;

    ///\internal idle threads (one per core)
    static Thread *idle[CPU_NUM_CORES];
};

} //namespace miosix

#endif //SCHED_TYPE_EDF
