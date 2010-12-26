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

#ifndef PRIORITY_SCHEDULER_H
#define	PRIORITY_SCHEDULER_H

#include "config/miosix_settings.h"

namespace miosix {

class Thread; //Forward declaration

/**
 * This class models the concept of priority for the priority scheduler.
 * In this scheduler the priority is simply a short int with values ranging
 * from 0 to PRIORITY_MAX-1, higher values mean higher priority, and the special
 * value -1 reserved for the idle thread.
 */
class PrioritySchedulerPriority
{
public:
    /**
     * Constructor. Not explicit for backward compatibility.
     * \param priority, the desired priority value.
     */
    PrioritySchedulerPriority(short int priority): priority(priority){}

    /**
     * Default constructor.
     */
    PrioritySchedulerPriority(): priority(MAIN_PRIORITY) {}

    /**
     * \return the priority value
     */
    short int get() const { return priority; }

    /**
     * \return true if this objects represents a valid priority.
     * Note that the value -1 is considered not valid, because it is reserved
     * for the idle thread.
     */
    bool validate() const;

private:
    short int priority;///< The priority value
};

inline bool operator <(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() < b.get();
}

inline bool operator <=(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() <= b.get();
}

inline bool operator >(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() > b.get();
}

inline bool operator >=(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() >= b.get();
}

inline bool operator ==(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() == b.get();
}

inline bool operator !=(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() != b.get();
}

/**
 * \internal
 * An instance of this class is embedded in every Thread class. It contains all
 * the per-thread data required by the scheduler.
 */
class PrioritySchedulerData
{
public:
    ///Thread priority. Used to speed up the implementation of getPriority.<br>
    ///Note that to change the priority of a thread it is not enough to change
    ///this.<br>It is also necessary to move the thread from the old prority
    ///list to the new priority list.
    PrioritySchedulerPriority priority;
    Thread *next;///<Pointer to next thread of the same priority. CIRCULAR list
};

/**
 * \internal
 * Priority scheduler.
 */
class PriorityScheduler
{
public:
    /**
     * \internal
     * Add a new thread to the scheduler.
     * This is called when a thread is created.
     * \param thread a pointer to a valid thread instance.
     * The behaviour is undefined if a thread is added multiple timed to the
     * scheduler, or if thread is NULL.
     * \param priority the priority of the new thread.
     * Priority must be a positive value.
     * Note that the meaning of priority is scheduler specific.
     */
    static void PKaddThread(Thread *thread, PrioritySchedulerPriority priority);

    /**
     * \internal
     * \return true if thread exists, false if does not exist or has been
     * deleted. A joinable thread is considered existing until it has been
     * joined, even if it returns from its entry point (unless it is detached
     * and terminates).
     *
     * Can be called both with the kernel paused and with interrupts disabled.
     */
    static bool PKexists(Thread *thread);

    /**
     * \internal
     * Called when there is at least one dead thread to be removed from the
     * scheduler
     */
    static void PKremoveDeadThreads();

    /**
     * \internal
     * Set the priority of a thread.
     * Note that the meaning of priority is scheduler specific.
     * \param thread thread whose priority needs to be changed.
     * \param newPriority new thread priority.
     * Priority must be a positive value.
     */
    static void PKsetPriority(Thread *thread,
            PrioritySchedulerPriority newPriority);

    /**
     * \internal
     * Get the priority of a thread.
     * Note that the meaning of priority is scheduler specific.
     * \param thread thread whose priority needs to be queried.
     * \return the priority of thread.
     */
    static PrioritySchedulerPriority getPriority(Thread *thread);

    /**
     * \internal
     * Same as getPriority, but meant to be called with interrupts disabled.
     * \param thread thread whose priority needs to be queried.
     * \return the priority of thread.
     */
    static PrioritySchedulerPriority IRQgetPriority(Thread *thread);

    /**
     * \internal
     * This is called before the kernel is started to by the kernel. The given
     * thread is the idle thread, to be run all the times where no other thread
     * can run.
     */
    static void IRQsetIdleThread(Thread *idleThread);

    /**
     * \internal
     * This function is used to develop interrupt driven peripheral drivers.<br>
     * Can be used ONLY inside an IRQ (and not when interrupts are disabled) to
     * find next thread in READY status. If the kernel is paused, does nothing.
     * Can be used for example if an IRQ causes a higher priority thread to be
     * woken, to change context. Note that to use this function the IRQ must
     * use the macros to save/restore context defined in portability.h
     *
     * If the kernel is paused does nothing.
     * It's behaviour is to modify the global variable miosix::cur which always
     * points to the currently running thread.
     */
    static void IRQfindNextThread();

private:

    ///\internal Vector of lists of threads, there's one list for each priority
    ///Each list s a circular list.
    ///(since 0=NULL, using aggregate initialization)
    static Thread *thread_list[PRIORITY_MAX];

    ///\internal idle thread
    static Thread *idle;
};

} //namespace miosix

#endif //PRIORITY_SCHEDULER_H
