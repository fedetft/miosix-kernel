/***************************************************************************
 *   Copyright (C) 2023 by Daniele Cattaneo                                *
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

#include "thread.h"
#include "cpu_time_counter_types.h"
#include "interfaces/cpu_const.h"

#ifdef WITH_CPU_TIME_COUNTER

namespace miosix {

/**
 * \addtogroup Kernel
 * \{
 */

/**
 * CPUTimeCounter provides a low-level method to retrieve information about how
 * much CPU time was used up to now by each thread in the system.
 * It is intended for debugging and evaluation purposes and is enabled only if
 * the symbol `WITH_CPU_TIME_COUNTER` has been defined in
 * config/miosix_settings.h.
 * 
 * The implementation of this class collects this data by intercepting context
 * switch events. Due to the measurement method, some caveats apply to the data
 * returned:
 *  - There is no distinction between time spent in thread code or in the
 *    kernel.
 *  - Time spent in an interrupt is accounted towards the thread that has been
 *    interrupted.
 * 
 * Retrieving the time accounting data for all threads is performed through the
 * iterator returned by IRQbegin(). To prevent the thread list from changing
 * because of a context switch, the iterator must be traversed while holding the
 * global interrupt lock.
 * 
 * To simplify post-processing, the list of thread data information accessible
 * through the iterator always satisfies the following properties:
 *  - The threads are listed in creation order.
 *  - The relative order in which the items are iterated is deterministic and
 *    does not change even after a context switch.
 * These properties allow to compute the difference between two thread data
 * lists collected at different times in O(max(n,m)) complexity. Idle threads
 * do not appear in the lists.
 * 
 * \note This is a very low-level interface. For actual use, a more practical
 * alternative is miosix::CPUProfiler, which provides a top-like display of the
 * amount of CPU used by each thread in a given time interval.
 */
class CPUTimeCounter
{
public:
    /**
     * Struct used to return the time counter data for a specific thread.
     */
    struct Data
    {
        enum {
            NOT_READY=' ',
            READY='r',
            RUNNING='R'
        };
        /// The thread the data belongs to
        Thread *thread;
        /// Amount of CPU time scheduled to the thread in ns for each core
        long long usedCpuTime[CPU_NUM_CORES] = {0};
        /// Flags at the time the data was collected
        char state=NOT_READY;
    };

    /**
     * CPUTimeCounter thread data iterator type
     */
    class iterator
    {
    public:
        inline iterator operator++()
        {
            cur = cur->timeCounterData.next;
            return *this;
        }
        inline iterator operator++(int)
        {
            iterator result = *this;
            cur = cur->timeCounterData.next;
            return result;
        }
        inline Data operator*()
        {
            Data res;
            res.thread=cur;
            if(res.thread->flags.isReady())
            {
                IRQgetReadyThreadData(res);
            } else {
                for(unsigned char i=0;i<CPU_NUM_CORES;i++)
                {
                    res.usedCpuTime[i]=cur->timeCounterData.usedCpuTime[i];
                }
            }
            return res;
        }
        inline bool operator==(const iterator& rhs) { return cur==rhs.cur; }
        inline bool operator!=(const iterator& rhs) { return cur!=rhs.cur; }
    private:
        friend class CPUTimeCounter;

        /**
         * \internal Helper function for handling computation of the amount of
         * CPU run-time consumed up to now by a ready thread.
         */
        void IRQgetReadyThreadData(Data& res);

        Thread *cur;
        long long time;
        iterator(Thread *cur, long long time) : cur(cur), time(time) {}
    };

    /**
     * \returns the number of threads currently alive in the system.
     * \warning This method is only provided for the purpose of reserving enough
     * memory for collecting the time data for all threads. The value it
     * returns may change at any time.
     */
    static inline unsigned int getThreadCount()
    {
        return nThreads;
    }

    /**
     * \returns the begin iterator for the thread data.
     * \param time the current time.
     */
    static iterator IRQbegin(long long time)
    {
        return iterator(head, time);
    }

    /**
     * \returns the end iterator for the thread data.
     */
    static iterator IRQend()
    {
        return iterator(nullptr, 0);
    }

private:
    // The following methods are called from the schedulers to notify
    // CPUTimeCounter of various events.
    template<typename> friend class basic_scheduler;
    friend class PriorityScheduler;
    friend class EDFScheduler;
    friend class ControlScheduler;

    // CPUTimeCounter cannot be constructed
    CPUTimeCounter() = delete;

    /**
     * \internal
     * Add an item to the list of threads tracked by CPUTimeCounter.
     * \param thread The thread to be added.
     */
    static inline void IRQaddThread(Thread *thread)
    {
        if(!head)
        {
            head=tail=thread;
        } else {
            tail->timeCounterData.next=thread;
            tail=thread;
        }
        nThreads++;
    }

    /**
     * \internal
     * Update the list of threads tracked by CPUTimeCounter to remove dead
     * threads.
     */
    static void removeDeadThreads();

    /**
     * Function to be called in the context switch code to profile threads
     * \param prev time count struct of previously running thread
     * \param prev time count struct of thread to be scheduled next
     * \param t (approximate) current time, a time point taken somewhere during
     * the context switch code
     */
    static inline void IRQprofileContextSwitch(Thread *prev, Thread *next,
        long long t, unsigned char coreId)
    {
        prev->timeCounterData.usedCpuTime[coreId]+=t-prev->timeCounterData.lastActivation;
        next->timeCounterData.lastActivation=t;
    }
    
    static Thread *head; ///< Head of the thread list
    static Thread *tail; ///< Tail of the thread list
    static volatile unsigned int nThreads; ///< Number of non-idle threads
};

/**
 * \}
 */

}

#endif // WITH_CPU_TIME_COUNTER
