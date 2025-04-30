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
#include "kernel/intrusive.h"
#include "kernel/thread.h"

/**
 * \file sched_data_structures.h
 *
 * This file contains core data structures used to implement the schedulers and
 * synchronization primitives.
 * These data structures do not allocate memory on the heap, and can be used
 * also in contexts where the GlobalIrqLock or PauseKernelLock is taken provided
 * that all calls to the member functions are made with the same lock taken,
 * as no locking is performed by the data structures themselves.
 */

namespace miosix {

#ifdef SCHED_TYPE_PRIORITY

/**
 * A priority queue class not meant for general-purpose use but instead
 * optimized for the specific task of being a thread waiting queue to be used
 * as an implementation detail in synchronization primitives when scheduled
 * with SCHED_TYPE_PRIORITY.
 *
 * The number of priorities is not configurable on a per-queue basis and is
 * instead fixed as the system-wide constant NUM_PRIORITIES.
 *
 * It has the following properties:
 * - enqueue is O(1) in the number of enqueued items
 * - dequeueOne() is also O(1) in the number of enqueued items, and it always
 * returns the highest priority item inserted. Additinally, if multiple items
 * with the same priority are enqueued, they are dequeued in FIFO order to
 * prevent starvation.
 * - dequeueAll() is of course O(n)
 * - remove is O(1)
 *
 * To achieve this result, it is implemented as an array of lists, one for each
 * priority level.
 *
 * \tparam T a class that must inherit from IntrusiveListItem, as items are
 * stored in IntrusiveList<T>
 */
template<typename T>
class PriorityQueue
{
public:
    /**
     * \return true if the PriorityQueue is empty
     */
    bool empty() const
    {
        for(int i=NUM_PRIORITIES-1;i>=0;i--) if(queued[i].empty()==false)
            return false;
        return true;
    }

    /**
     * Add an item to the queue.
     * \param item pointer to an item. The queue does not take ownership of the
     * pointer and does not deallocate it in any way when dequeued/removed, this
     * is a responsibility of the caller
     * \param priority the item priority. The priority value must be
     * 0 <= priority < NUM_PRIORITIES
     */
    void enqueue(T *item, Priority priority)
    {
        queued[priority.get()].push_back(item);
    }

    /**
     * Dequeue the highest priority item in the queue, if any
     * \return the highest priority item (if multiple items with the same
     * priority have been added, they are returned in FIFO order) or nullptr if
     * the queue is empty
     */
    T* dequeueOne()
    {
        T *result=nullptr;
        for(int i=NUM_PRIORITIES-1;i>=0;i--)
        {
            if(queued[i].empty()) continue;
            result=queued[i].front();
            queued[i].pop_front();
            break;
        }
        return result;
    }

    /**
     * Efficiently dequeue all items from the queue. After this method is
     * called, the queue will be empty
     * \param op callback operation that will be invoked once for every item
     * found in the queue, passing it as callback parameter
     */
    void dequeueAll(void (*op)(T *))
    {
        for(int i=NUM_PRIORITIES-1;i>=0;i--)
        {
            while(!queued[i].empty())
            {
                op(queued[i].front());
                queued[i].pop_front();
            }
        }
    }

    /**
     * Remove a specific item from the queue
     * \param item item to remove. This function is implemented by calling
     * IntrusiveList<T>::removeFast(), so all the warning related to this
     * function apply here.
     * \param priority item priority, which MUST be the exact same value that
     * was passed when the item was enqueued, as IntrusiveList<T>::removeFast()
     * produces undefined behavior when removing an item from a different list
     * than the one it was inserted.
     */
    void remove(T *item, Priority priority)
    {
        queued[priority.get()].removeFast(item);
    }

private:
    ///\internal Vector of lists of items, there's one list for each priority
    IntrusiveList<T> queued[NUM_PRIORITIES];
};

#endif //SCHED_TYPE_PRIORITY

/**
 * A time-sorted queue class not meant for general-purpose use but instead
 * optimized for the specific task of being a thread waiting queue to be used
 * as an implementation detail in synchronization primitives when scheduled
 * with SCHED_TYPE_EDF.
 *
 * It has the following properties:
 * - enqueue() is worst-case O(n) in the number of enqueued items
 * - dequeueOne() and dequeueTime() are O(1) in the number of enqueued items,
 * and always return the item with the lowest associated time. Additinally, if
 * multiple items with the same associated time are enqueued, they are dequeued
 * in LIFO order as it makes for a faster implementation and if the passed
 * times are deadlines, if the pool is schedulable all deadlines will be met
 * regardless of the order in which items are dequeued.
 * - dequeueAll() is of course O(n)
 * - remove is O(1)
 *
 * \tparam T a class that must inherit from IntrusiveListItem, as items are
 * stored in IntrusiveList<T>. Additionally, the T type must provide a getTime()
 * member function to retrieve the associated time
 */
template<typename T>
class TimeSortedQueue
{
public:
    /**
     * \return true if the TimeSortedQueue is empty
     */
    bool empty() const { return queued.empty(); }

    /**
     * Add an item to the queue.
     * \param item pointer to an item. The queue does not take ownership of the
     * pointer and does not deallocate it in any way when dequeued/removed, this
     * is a responsibility of the caller
     */
    void enqueue(T *item)
    {
        auto t=item->getTime();
        auto it=queued.begin();
        while(it!=queued.end() && *it->getTime()<t) ++it;
        queued.insert(it,item);
    }

    /**
     * Dequeue the item in the queue with the lowest associated time, if any
     * \return the item inserted in the queue with the lowest associated time.
     * If multiple items with the exact same same associated time have been
     * added, they are returned in LIFO order. This function returns nullptr
     * if the queue is empty
     */
    T* dequeueOne()
    {
        if(queued.empty()) return nullptr;
        T *result=queued.front();
        queued.pop_front();
        return result;
    }

    /**
     * Dequeue an item only if its associated time is lower or equal than the
     * specified time
     * \param time time to determine if there are items to dequeue
     * \return the item inserted in the queue with the lowest associated time,
     * if
     */
    T *dequeueTime(long long time)
    {
        if(queued.empty()) return nullptr;
        T *result=queued.front();
        if(time<result->getTime()) return nullptr;
        queued.pop_front();
        return result;
    }

    /**
     * Efficiently dequeue all items from the queue. After this method is
     * called, the queue will be empty
     * \param op callback operation that will be invoked once for every item
     * found in the queue, passing it as callback parameter
     */
    void dequeueAll(void (*op)(T *))
    {
        while(!queued.empty())
        {
            op(queued.front());
            queued.pop_front();
        }
    }

    /**
     * Remove a specific item from the queue
     * \param item item to remove. This function is implemented by calling
     * IntrusiveList<T>::removeFast(), so all the warning related to this
     * function apply here.
     */
    void remove(T *item) { queued.removeFast(item); }

private:
    ///\internal List of items, sorted by wakeup time
    IntrusiveList<T> queued;
};

} //namespace miosix
