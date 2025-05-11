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

/**
 * \internal
 * A priority queue class not meant for general-purpose use but instead
 * optimized for the specific task of being a thread waiting queue to be used
 * as an implementation detail in synchronization primitives when scheduled
 * with SCHED_TYPE_PRIORITY.
 *
 * It has the following properties:
 * - enqueue is O(1) in the number of enqueued items
 * - dequeueOne() is also O(1) in the number of enqueued items, and it always
 *   returns the highest priority item inserted. Additinally, if multiple items
 *   with the same priority are enqueued, they are dequeued in FIFO order to
 *   prevent starvation.
 * - dequeueAll() is of course O(n)
 * - remove is O(1)
 *
 * To achieve this result, it is implemented as an array of lists, one for each
 * priority level.
 *
 * \tparam T a class that must inherit from IntrusiveListItem, as items are
 * stored in IntrusiveList<T>
 * \tparam numPriorities number of priorities the queue needs to support.
 * Vald priority values rage from 0 to numPriorities-1. Note that the memory
 * occupied by this class is proportional to the number of priorities
 */
template<typename T, unsigned numPriorities>
class PriorityQueue
{
public:
    /**
     * \return true if the PriorityQueue is empty
     */
    bool empty() const
    {
        for(int i=numPriorities-1;i>=0;i--)
            if(queued[i].empty()==false)
                return false;
        return true;
    }

    /**
     * \return the first item that would be woken up if dequeueOne() was called.
     * Causes undefined behavior if called when the queue is empty
     */
    T *front()
    {
        for(int i=numPriorities-1;i>=0;i--)
        {
            if(queued[i].empty()) continue;
            return queued[i].front();
        }
        return nullptr;
    }

    /**
     * Add an item to the queue.
     * \param item pointer to an item. The queue does not take ownership of the
     * pointer and does not deallocate it in any way when dequeued/removed, this
     * is a responsibility of the caller
     * \param priority the item priority. The priority value must be
     * 0 <= priority < numPriorities
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
        for(int i=numPriorities-1;i>=0;i--)
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
     * found in the queue, passing the item as callback parameter
     */
    void dequeueAll(void (*op)(T *))
    {
        for(int i=numPriorities-1;i>=0;i--)
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
    IntrusiveList<T> queued[numPriorities];
};

/**
 * \internal
 * A time-sorted queue class not meant for general-purpose use but instead
 * optimized for the specific task of being a thread waiting queue to be used
 * as an implementation detail in synchronization primitives when scheduled
 * with SCHED_TYPE_EDF.
 *
 * It has the following properties:
 * - enqueue() is worst-case O(n) in the number of enqueued items
 * - dequeueOne() and dequeueTime() are O(1) in the number of enqueued items,
 * and always return the item with the lowest associated time. Additionally, if
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
 * \tparam getTime functor that returns the time associated with an item T
 */
template<typename T, typename GetTime>
class TimeSortedQueue
{
public:
    /**
     * \return true if the TimeSortedQueue is empty
     */
    bool empty() const { return queued.empty(); }

    /**
     * \return the first item that would be woken up if dequeueOne() was called.
     * Causes undefined behavior if called when the queue is empty
     */
    T *front() { return queued.front(); }

    /**
     * Add an item to the queue.
     * \param item pointer to an item. The queue does not take ownership of the
     * pointer and does not deallocate it in any way when dequeued/removed, this
     * is a responsibility of the caller
     */
    void enqueue(T *item)
    {
        GetTime getTime;
        long long t=getTime(item);
        auto it=queued.begin();
        while(it!=queued.end() && getTime(*it)<t) ++it;
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
        GetTime getTime;
        if(queued.empty()) return nullptr;
        T *result=queued.front();
        if(time<getTime(result)) return nullptr;
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

/**
 * \internal
 * A FIFO queue class not meant for general-purpose use but instead
 * optimized for the specific task of being a thread waiting queue to be used
 * as an implementation detail in synchronization primitives.
 *
 * It has the following properties:
 * - enqueue() is O(1) in the number of enqueued items
 * - dequeueOne() is O(1) in the number of enqueued items, and returns items in
 *   FIFO order
 * - dequeueAll() is of course O(n)
 * - remove is O(1)
 *
 * \tparam T a class that must inherit from IntrusiveListItem, as items are
 * stored in IntrusiveList<T>.
 */
template<typename T>
class FifoQueue
{
public:
    /**
     * \return true if the TimeSortedQueue is empty
     */
    bool empty() const { return queued.empty(); }

    /**
     * \return the first item that would be woken up if dequeueOne() was called.
     * Causes undefined behavior if called when the queue is empty
     */
    T *front() { return queued.front(); }

    /**
     * Add an item to the queue.
     * \param item pointer to an item. The queue does not take ownership of the
     * pointer and does not deallocate it in any way when dequeued/removed, this
     * is a responsibility of the caller
     */
    void enqueue(T *item)
    {
        queued.push_back(item);
    }

    /**
     * Dequeue the item in the queue with the lowest associated time, if any
     * \return an item previously inserted in FIFO order or nullptr if the
     * queue is empty
     */
    T* dequeueOne()
    {
        if(queued.empty()) return nullptr;
        T *result=queued.front();
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

/**
 * List of possible ways a WaitQueue can handle the priority of waiting threads
 */
enum class PriorityPolicy
{
    ConsiderInheritedPriority, ///< Threads awakened in order of actual priority
    IgnoreInheritedPriority    ///< Threads awakened in order of "saved" priority
};

/**
 * \internal
 * A queue class for holding threads waiting on a synchronization primitive
 * such as a Mutex or ConditionVariable.
 *
 * The queue implementation is scheduler-dependent so as to correctly prioritize
 * threads wakeups depending on the chosen scheduler
 *
 * NOTE: since this class is only meant to implement synchronization primitives
 * and in Miosix from version 3 onwards synchronization primitives are
 * implemented using PauseKernelLock, this class uses PK prefixed member
 * functions to access thread properties, and assumes the class is only accessed
 * while holding the PauseKernelLock
 *
 * \tparam pp priority policy. Can be one of two values:
 * - IgnoreInheritedPriority: for schedulers where priority-based wakeup is
 *   needed, wakeup waiting threads based on their original "saved" priority,
 *   thus not considering the inherited priority due to priority inheritance.
 * - ConsiderInheritedPriority: for schedulers where priority-based wakeup is
 *   needed, wakeup waiting threads based on their actual priority, thus
 *   considering the inherited priority. Note that you can't just set this
 *   option and be done. If you set this option, your synchronization primitive
 *   must be part of the thread's locked list so the synchronization primitive
 *   is notified every time a priority boost happens and can remove the waiting
 *   thread from the waitQueue, then boost the priority and then place it back.
 *   Failing to do so will not only defeat the purpose of waking threads
 *   considering the inherited priority but also cause undefined behavior for
 *   some WaitQueue implementations, namely those with an array of lists
 *   indexed by priority.
 */
template<PriorityPolicy pp>
class WaitQueue
{
public:
    static_assert(pp==PriorityPolicy::ConsiderInheritedPriority
               || pp==PriorityPolicy::IgnoreInheritedPriority,"");

    #if defined(SCHED_TYPE_PRIORITY) && NUM_PRIORITIES>1
    WaitQueue() : queue(new PriorityQueue<WaitToken,NUM_PRIORITIES>) {}
    #endif

    /**
     * \return true if the WaitQueue is empty
     */
    bool PKempty() const
    {
        #if defined(SCHED_TYPE_PRIORITY) && NUM_PRIORITIES>1
        if(queue==nullptr) return true;
        return queue->empty();
        #else
        return queue.empty();
        #endif
    }

    /**
     * \return the first thread that would be woken up if PKwakeOne() was called.
     * Causes undefined behavior if called when the queue is empty
     */
    Thread *PKfront()
    {
        #if defined(SCHED_TYPE_PRIORITY) && NUM_PRIORITIES>1
        return queue->front()->t;
        #else
        return queue.front()->t;
        #endif
    }

    /**
     * Add a thread to the queue.
     * \param item pointer to an item. The queue does not take ownership of the
     * pointer and does not deallocate it in any way when dequeued/removed, this
     * is a responsibility of the caller
     */
    void PKenqueue(Thread *t)
    {
        #if defined(SCHED_TYPE_PRIORITY) && NUM_PRIORITIES>1
        //We allocate here only for statically allocated pthread_* objects,
        //in all other cases allocation happens in the constructor
        if(queue==nullptr) queue=new PriorityQueue<WaitToken,NUM_PRIORITIES>;
        if(pp==PriorityPolicy::ConsiderInheritedPriority)
             queue->enqueue(&t->waitQueueItem,t->PKgetPriority());
        else queue->enqueue(&t->waitQueueItem,t->savedPriority.get());
        #else
        queue.enqueue(&t->waitQueueItem);
        #endif
    }

    /**
     * Wake the highest priority item in the queue, if any
     * \return the woken thread, or nullptr if the queue was empty
     */
    Thread *PKwakeOne()
    {
        #if defined(SCHED_TYPE_PRIORITY) && NUM_PRIORITIES>1
        if(queue==nullptr) return nullptr;
        WaitToken *item=queue->dequeueOne();
        #else
        WaitToken *item=queue.dequeueOne();
        #endif
        if(item==nullptr) return nullptr;
        //Also sets pendingWakeup if higher priority thread woken
        item->t->PKwakeup();
        return item->t;
    }

    /**
     * Wake all items from the queue. After this method is called, the queue
     * will be empty
     */
    void PKwakeAll()
    {
        #if defined(SCHED_TYPE_PRIORITY) && NUM_PRIORITIES>1
        if(queue==nullptr) return;
        queue->dequeueAll([](WaitToken *item){
            //Also sets pendingWakeup if higher priority thread woken
            item->t->PKwakeup();
        });
        #else
        queue.dequeueAll([](WaitToken *item){
            //Also sets pendingWakeup if higher priority thread woken
            item->t->PKwakeup();
        });
        #endif
    }

    /**
     * Remove a specific item from the queue
     * \param item item to remove. This function is implemented by calling
     * IntrusiveList<T>::removeFast(), so all the warning related to this
     * function apply here.
     */
    void PKremove(Thread *t)
    {
        #if defined(SCHED_TYPE_PRIORITY) && NUM_PRIORITIES>1
        if(queue==nullptr) return;
        if(pp==PriorityPolicy::ConsiderInheritedPriority)
             queue->remove(&t->waitQueueItem,t->PKgetPriority());
        else queue->remove(&t->waitQueueItem,t->savedPriority.get());
        #else
        queue.remove(&t->waitQueueItem);
        #endif
    }

    #if defined(SCHED_TYPE_PRIORITY) && NUM_PRIORITIES>1
    ~WaitQueue() { if(queue) delete queue; }
    #endif

private:
    #if defined(SCHED_TYPE_PRIORITY) && NUM_PRIORITIES>1
    /*
     * That's the hard case. Using a single list leaves no way to efficiently
     * implement this queue. We could inserion-sort into the list by
     * priority like with EDF, but doing so in the use case where more threads
     * in the list have the same priority causes wakeup in LIFO order which may
     * result in starvation. Doing inserion-sort into the list but adding
     * another loop to put the thread at the end of the threads with the same
     * priority already in the list allows prioritization and FIFO order, but
     * causes O(n) insertion in the use case where all threads in the list have
     * the same priority.
     * The only way out is to have an array of lists, one for each priority
     * level, but that adds more complications that we address in our code:
     * - The array has to be dynamically allocated otherwise the size of the
     *   synchronization primitive object would depend on the number of priority
     *   levels selected when compiling the kernel. This is not possible since
     *   the memory layout of pthread_* synchronization primitives is fixed when
     *   the C standard library is compiled while the number of priorities is
     *   tunable when the kernel is compiled.
     * - Moreover, for pthread_* synchronization primitives statically
     *   initialized with PTHREAD_*_INITIALIZER the constructor is not called,
     *   and all fields are set to 0/nullptr (see Miosix compiler patches).
     *   Thus we must check for nullptr and allocate it on first use. Thankfully
     *   deallocation is easy as statically constructed objects have a lifetime
     *   equal to the entire program thus no deallocation is needed, while
     *   pthread_*_destroy calls the destructor so no memory leaks are possible.
     * - Finally, once we have an array of lists, one for each priority, we must
     *   be absolutely sure that removeFromWaitQueue will try to remove the
     *   thread from the same list we are inserting it here, otherise memory
     *   corruption occurs in IntrusiveList::removeFast. That's made easier by
     *   the fact that in Miosix a thread can only change its own priority, and
     *   of course while waiting on a synchronization primitive a thread isn't
     *   running and thus cannot call Thread::setPriority() but there's a catch:
     *   priority inheritance, which may cause a thread's priority to change
     *   even while waiting. We solve this by adding a template parameter,
     *   PriorityPolicy to consider either the actual (possibly inherited) or
     *   orignal "saved" priority.
     */
    PriorityQueue<WaitToken,NUM_PRIORITIES> *queue;
    #elif defined(SCHED_TYPE_PRIORITY)
    /**
     * Priority scheduler with only 1 priority level is a pure round-robin,
     * thus wakeup threads in fifo order
     */
    FifoQueue<WaitToken> queue;
    #elif defined(SCHED_TYPE_EDF)
    /**
     * Functor needed by TimeSortedQueue to insert the thread in the correct
     * place in the waiting list based on its deadline
     */
    struct GetTime
    {
        long long operator()(WaitToken *item)
        {
            if(pp==PriorityPolicy::ConsiderInheritedPriority)
                return item->t->PKgetPriority().get();
            else return item->t->savedPriority.get();
        }
    };
    /*
     * With EDF there is a simple implementation: the priority is the absolute
     * deadline time in nanoseconds, thus we can just use a single list sorted
     * by deadline and not care about threads having the same priority, just
     * like sleepingList. If more threads with the exact same deadline are
     * inserted, they will be awakened in LIFO order but it doesn't matter since
     * if the task pool is schedulable they will still complete before their
     * deadline.
     */
    TimeSortedQueue<WaitToken,WaitQueue::GetTime> queue;
    #elif defined(SCHED_TYPE_CONTROL_BASED)
    FifoQueue<WaitToken> queue; //TODO: support prioritization with mutexLessOp?
    #endif
};

} //namespace miosix
