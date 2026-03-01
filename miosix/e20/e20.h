/***************************************************************************
 *   Copyright (C) 2012 - 2023 by Terraneo Federico                        *
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

//Miosix event based API

#pragma once

#include <list>
#include <functional>
#include <miosix.h>
#include "callback.h"

namespace miosix {

/**
 * A variable sized event queue.
 * 
 * Makes use of heap allocations and as such it is not possible to post events
 * from within interrupt service routines. For this, use FixedEventQueue.
 * 
 * This class acts as a synchronization point, multiple threads can post
 * events, and multiple threads can call run() or runOne() (thread pooling).
 * 
 * Events are function that are posted by a thread through post() but executed
 * in the context of the thread that calls run() or runOne()
 */
class EventQueue
{
public:
    /**
     * Constructor
     */
    EventQueue() {}

    /**
     * Post an event to the queue. This function never blocks.
     * 
     * \param event function function to be called in the thread that calls
     * run() or runOne(). Bind can be used to bind parameters to the function.
     * \throws std::bad_alloc if there is not enough heap memory
     */
    void post(std::function<void ()> event);

    /**
     * This function blocks waiting for events being posted, and when available
     * it calls the event function. To return from this event loop an event
     * function must throw an exception.
     * 
     * \throws any exception that is thrown by the event functions
     */
    void run();

    /**
     * Run at most one event. This function does not block.
     * 
     * \throws any exception that is thrown by the event functions
     */
    void runOne();

    /**
     * \return the number of events in the queue
     */
    unsigned int size() const
    {
        Lock<KernelMutex> l(m);
        return events.size();
    }
    
    /**
     * \return true if the queue has no events
     */
    bool empty() const
    {
        Lock<KernelMutex> l(m);
        return events.empty();
    }

    EventQueue(const EventQueue&) = delete;
    EventQueue& operator= (const EventQueue&) = delete;

private:
    std::list<std::function<void ()>> events; ///< Event queue
    mutable KernelMutex m; ///< Mutex for synchronisation
    ConditionVariable cv; ///< Condition variable for synchronisation
};

/**
 * \internal
 * This class is to extract from FixedEventQueue code that
 * does not depend on the NumSlots template parameters.
 */
template<unsigned SlotSize>
class FixedEventQueueBase
{
protected:
    /**
     * Constructor.
     */
    FixedEventQueueBase() {}

    /**
     * Post an event. Blocks if event queue is full.
     * \param event event to post
     * \param events pointer to event queue
     * \param size event queue size
     */
    void postImpl(Callback<SlotSize>&& event, Callback<SlotSize> *events,
            unsigned int size);

    /**
     * Post an event from an interrupt, or with interrupts disabled.
     * \param event event to post
     * \param events pointer to event queue
     * \param size event queue size
     * \return false if there was no space in the queue
     */
    bool IRQpostImpl(Callback<SlotSize>&& event, Callback<SlotSize> *events,
            unsigned int size);

    /**
     * This function blocks waiting for events being posted, and when available
     * it calls the event function. To return from this event loop an event
     * function must throw an exception.
     * 
     * \param events pointer to event queue
     * \param size event queue size
     * \throws any exception that is thrown by the event functions
     */
    void runImpl(Callback<SlotSize> *events, unsigned int size);

    /**
     * Run at most one event. This function does not block.
     * 
     * \param events pointer to event queue
     * \param size event queue size
     * \throws any exception that is thrown by the event functions
     */
    void runOneImpl(Callback<SlotSize> *events, unsigned int size);

    /**
     * \return the number of events in the queue
     */
    unsigned int sizeImpl() const
    {
        FastGlobalIrqLock dLock;
        return n;
    }

private:
    /**
     * \internal Element of a thread waiting list
     */
    class WaitToken : public IntrusiveListItem
    {
    public:
        WaitToken(Thread *thread) : thread(thread) {}
        Thread *thread; ///<\internal Waiting thread and spurious wakeup token
    };

    unsigned int put=0; ///< Put position into events
    unsigned int get=0; ///< Get position into events
    unsigned int n=0;   ///< Number of occupied event slots
    IntrusiveList<WaitToken> waitingGet, waitingPut; ///< Waiting on get/put
};

template<unsigned SlotSize>
void FixedEventQueueBase<SlotSize>::postImpl(Callback<SlotSize>&& event,
        Callback<SlotSize> *events, unsigned int size)
{
    FastGlobalIrqLock dLock;
    while(IRQpostImpl(std::move(event),events,size)==false)
    {
        WaitToken w(Thread::IRQgetCurrentThread());
        waitingPut.push_back(&w);
        //w.thread must be set to nullptr to protect against spurious wakeups
        while(w.thread) Thread::IRQglobalIrqUnlockAndWait(dLock);
    }
}

template<unsigned SlotSize>
bool FixedEventQueueBase<SlotSize>::IRQpostImpl(Callback<SlotSize>&& event,
        Callback<SlotSize> *events, unsigned int size)
{
    if(n>=size) return false;
    events[put]=std::move(event);
    if(++put>=size) put=0;
    n++;
    if(waitingGet.empty()==false)
    {
        Thread *t=waitingGet.front()->thread;
        waitingGet.front()->thread=nullptr;
        waitingGet.pop_front();
        t->IRQwakeup();
    }
    return true;
}

template<unsigned SlotSize>
void FixedEventQueueBase<SlotSize>::runImpl(Callback<SlotSize> *events,
        unsigned int size)
{
    FastGlobalIrqLock dLock;
    for(;;)
    {
        while(n<=0)
        {
            WaitToken w(Thread::IRQgetCurrentThread());
            waitingGet.push_back(&w);
            //w.thread must be set to nullptr to protect against spurious wakeups
            while(w.thread) Thread::IRQglobalIrqUnlockAndWait(dLock);
        }
        Callback<SlotSize> f=std::move(events[get]);
        if(++get>=size) get=0;
        n--;
        if(waitingPut.empty()==false)
        {
            waitingPut.front()->thread->IRQwakeup();
            waitingPut.front()->thread=nullptr;
            waitingPut.pop_front();
        }
        {
            FastGlobalIrqUnlock eLock(dLock);
            f();
        }
    }
}

template<unsigned SlotSize>
void FixedEventQueueBase<SlotSize>::runOneImpl(Callback<SlotSize> *events,
        unsigned int size)
{
    Callback<SlotSize> f;
    {
        FastGlobalIrqLock dLock;
        if(n<=0) return;
        f=std::move(events[get]);
        if(++get>=size) get=0;
        n--;
        if(waitingPut.empty()==false)
        {
            waitingPut.front()->thread->IRQwakeup();
            waitingPut.front()->thread=nullptr;
            waitingPut.pop_front();
        }
    }
    f();
}

/**
 * A fixed size event queue.
 * 
 * This guarantees it makes no use of the heap, therefore events can be posted
 * also from within interrupt handlers. This simplifies the development of
 * device drivers.
 * 
 * This class acts as a synchronization point, multiple threads (and IRQs) can
 * post events, and multiple threads can call run() or runOne()
 * (thread pooling).
 * 
 * Events are function that are posted by a thread through post() but executed
 * in the context of the thread that calls run() or runOne()
 * 
 * \param NumSlots maximum queue length
 * \param SlotSize size of the Callback objects. This limits the maximum number
 * of parameters that can be bound to a function. If you get compile-time
 * errors in callback.h, consider increasing this value. The default is 20
 * bytes, which is enough to bind a member function pointer, a "this" pointer
 * and two byte or pointer sized parameters.
 */
template<unsigned NumSlots, unsigned SlotSize=20>
class FixedEventQueue : private FixedEventQueueBase<SlotSize>
{
public:
    /**
     * Constructor.
     */
    FixedEventQueue() {}

    /**
     * Post an event, blocking if the event queue is full.
     * 
     * \param event The function to be invoked in the thread that calls
     * run() or runOne(), in the form of a functor object.
     * Bind can be used to bind parameters to the function and make a suitable
     * functor.
     * The type of the functor must be move-constructible, to ensure it is
     * callable from inside a GlobalIrqLock without causing memory allocations.
     * Additionally their move constructor must not perform things that are
     * forbidden in interrupt context such as open files, print, ...
     */
    template<typename T>
    void post(T&& event)
    {
        this->postImpl(Callback<SlotSize>(std::move(event)),events,NumSlots);
    }
    
    /**
     * Post an event in the queue, or return if the queue was full.
     * 
     * \param event The function to be invoked in the thread that calls
     * run() or runOne(), in the form of a functor object.
     * Bind can be used to bind parameters to the function and make a suitable
     * functor.
     * The type of the functor must be move-constructible, to ensure it is
     * callable from inside a GlobalIrqLock without causing memory allocations.
     * Additionally their move constructor must not perform things that are
     * forbidden in interrupt context such as open files, print, ...
     * \return false if there was no space in the queue
     */
    template<typename T>
    bool postNonBlocking(T&& event)
    {
        GlobalIrqLock dLock;
        return this->IRQpostImpl(Callback<SlotSize>(std::move(event)),
                                 events,NumSlots);
    }

    /**
     * Post an event in the queue, or return if the queue was full.
     * Can be called only with interrupts disabled or within an interrupt
     * handler, allowing device drivers to post an event to a thread.
     * 
     * \param event The function to be invoked in the thread that calls
     * run() or runOne(), in the form of a functor object.
     * Bind can be used to bind parameters to the function and make a suitable
     * functor.
     * The type of the functor must be move-constructible, to ensure it is
     * callable from inside a GlobalIrqLock without causing memory allocations.
     * Additionally their move constructor must not perform things that are
     * forbidden in interrupt context such as open files, print, ...
     * \return false if there was no space in the queue
     */
    template<typename T>
    bool IRQpost(T&& event)
    {
        return this->IRQpostImpl(Callback<SlotSize>(std::move(event)),
                                 events,NumSlots);
    }

    /**
     * This function blocks waiting for events being posted, and when available
     * it calls the event function. To return from this event loop an event
     * function must throw an exception.
     * 
     * \throws any exception that is thrown by the event functions
     */
    void run()
    {
        this->runImpl(events,NumSlots);
    }

    /**
     * Run at most one event. This function does not block.
     * 
     * \throws any exception that is thrown by the event functions
     */
    void runOne()
    {
        this->runOneImpl(events,NumSlots);
    }
    
    /**
     * \return the number of events in the queue
     */
    unsigned int size() const
    {
        return this->sizeImpl();
    }
    
    /**
     * \return true if the queue has no events
     */
    unsigned int empty() const
    {
        return this->sizeImpl()==0;
    }

    FixedEventQueue(const FixedEventQueue&) = delete;
    FixedEventQueue& operator= (const FixedEventQueue&) = delete;

private:
    Callback<SlotSize> events[NumSlots]; ///< Fixed size queue of events
};

} //namespace miosix
