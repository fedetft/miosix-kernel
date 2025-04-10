/***************************************************************************
 *   Copyright (C) 2011-2025 by Terraneo Federico                          *
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

//This file contains private implementation details of mutexes, it's not
//meant to be used by end users

#pragma once

#include <pthread.h>
#include "thread.h"
#include "intrusive.h"
#include "sync.h"

namespace miosix {

/**
 * \internal
 * Implementation code to lock a mutex. Must be called with the kernel paused
 * \param mutex mutex to be locked
 * \param d The instance of FastPauseKernelLock
 */
static inline void PKdoMutexLock(pthread_mutex_t *mutex, FastPauseKernelLock& d)
{
    void *p=reinterpret_cast<void*>(Thread::PKgetCurrentThread());
    if(mutex->owner==nullptr)
    {
        mutex->owner=p;
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the wait is enclosed in a
    //while(owner!=p) which is immeditely false.
    if(mutex->owner==p)
    {
        if(mutex->recursive>=0)
        {
            mutex->recursive++;
            return;
        } else errorHandler(MUTEX_DEADLOCK); //Bad, deadlock
    }

    WaitingList waiting; //Element of a linked list on stack
    waiting.thread=p;
    waiting.next=nullptr; //Putting this thread last on the list (lifo policy)
    if(mutex->first==nullptr)
    {
        mutex->first=&waiting;
        mutex->last=&waiting;
    } else {
        mutex->last->next=&waiting;
        mutex->last=&waiting;
    }

    //The while is necessary to protect against spurious wakeups
    while(mutex->owner!=p) Thread::PKrestartKernelAndWait(d);
}

/**
 * \internal
 * Implementation code to lock a mutex to a specified depth level.
 * Must be called with the kernel paused. If the mutex is not recursive the
 * mutex is locked only one level deep regardless of the depth value.
 * \param mutex mutex to be locked
 * \param d The instance of FastPauseKernelLock
 * \param depth recursive depth at which the mutex will be locked. Zero
 * means the mutex is locked one level deep (as if lock() was called once),
 * one means two levels deep, etc. 
 */
static inline void PKdoMutexLockToDepth(pthread_mutex_t *mutex,
        FastPauseKernelLock& d, unsigned int depth)
{
    void *p=reinterpret_cast<void*>(Thread::PKgetCurrentThread());
    if(mutex->owner==nullptr)
    {
        mutex->owner=p;
        if(mutex->recursive>=0) mutex->recursive=depth;
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the wait is enclosed in a
    //while(owner!=p) which is immeditely false.
    if(mutex->owner==p)
    {
        if(mutex->recursive>=0)
        {
            mutex->recursive=depth;
            return;
        } else errorHandler(MUTEX_DEADLOCK); //Bad, deadlock
    }

    WaitingList waiting; //Element of a linked list on stack
    waiting.thread=p;
    waiting.next=nullptr; //Putting this thread last on the list (lifo policy)
    if(mutex->first==nullptr)
    {
        mutex->first=&waiting;
        mutex->last=&waiting;
    } else {
        mutex->last->next=&waiting;
        mutex->last=&waiting;
    }

    //The while is necessary to protect against spurious wakeups
    while(mutex->owner!=p) Thread::PKrestartKernelAndWait(d);
    if(mutex->recursive>=0) mutex->recursive=depth;
}

/**
 * \internal
 * Implementation code to unlock all depth levels of a mutex.
 * Must be called with the kernel paused
 * \param mutex mutex to unlock
 * \return the mutex recursive depth (how many times it was locked by the
 * owner). Zero means the mutex is locked one level deep (lock() was called
 * once), one means two levels deep, etc. 
 */
static inline unsigned int PKdoMutexUnlockAllDepthLevels(pthread_mutex_t *mutex)
{
//    Safety check removed for speed reasons
//    if(mutex->owner!=reinterpret_cast<void*>(Thread::PKgetCurrentThread()))
//        return false;
    if(mutex->first!=nullptr)
    {
        Thread *t=reinterpret_cast<Thread*>(mutex->first->thread);
        t->PKwakeup();
        mutex->owner=mutex->first->thread;
        mutex->first=mutex->first->next;

        if(mutex->recursive<0) return 0;
        unsigned int result=mutex->recursive;
        mutex->recursive=0;
        return result;
    }
    
    mutex->owner=nullptr;
    
    if(mutex->recursive<0) return 0;
    unsigned int result=mutex->recursive;
    mutex->recursive=0;
    return result;
}

#ifdef WITH_PTHREAD_EXIT

/**
 * Exception type thrown when pthread_exit is called.
 *
 * NOTE: This type should not derive from std::exception on purpose, as it would
 * be bad if a \code catch(std::exception& e) \endcode would match this
 * exception.
 */
class PthreadExitException
{
public:
    /**
     * Constructor
     * \param returnValue thread return value passed to pthread_exit
     */
    PthreadExitException(void *returnValue) : returnValue(returnValue) {}

    /**
     * \return the thread return value passed to pthread_exit
     */
    void *getReturnValue() const { return returnValue; }

private:
    void *returnValue;
};

#endif //WITH_PTHREAD_EXIT

#ifdef WITH_PTHREAD_KEYS
/**
 * Called at thread exit to call destructors for pthread keys
 * \param pthreadKeyValues thread-local pthread_key values
 */
void callPthreadKeyDestructors(void *pthreadKeyValues[MAX_PTHREAD_KEYS]);
#endif //WITH_PTHREAD_KEYS

} //namespace miosix
