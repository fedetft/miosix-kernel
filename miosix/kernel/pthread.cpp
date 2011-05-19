/***************************************************************************
 *   Copyright (C) 2010, 2011 by Terraneo Federico                         *
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

/*
 * pthread.cpp Part of the Miosix Embedded OS. Provides a mapping of the
 * posix thread API to the Miosix thread API.
 */

#include <pthread.h>
#include <errno.h>
#include <stdexcept>
#include "kernel.h"
#include "error.h"
#include "pthread_private.h"

using namespace miosix;

//
// Newlib's pthread.h has been patched since Miosix 1.68 to contain a definition
// for pthread_mutex_t and pthread_cond_t that allows a fast implementation
// of mutexes and condition variables. This *requires* to use gcc 4.5.2 with
// Miosix specific patches.
//

//These functions needs to be callable from C
extern "C" {

//
// Thread related API
//

int pthread_create(pthread_t *pthread, const pthread_attr_t *attr,
    void *(*start)(void *), void *arg)
{
    Thread::Options opt=Thread::JOINABLE;
    unsigned int stacksize=STACK_DEFAULT_FOR_PTHREAD;
    if(attr!=NULL)
    {
        if(attr->detachstate==PTHREAD_CREATE_DETACHED)
            opt=Thread::DEFAULT;
        stacksize=attr->stacksize;
    }
    Thread *result=Thread::create(start,stacksize,1,arg,opt);
    if(result==0) return EAGAIN;
    *pthread=reinterpret_cast<pthread_t>(result);
    return 0;
}

int pthread_join(pthread_t pthread, void **value_ptr)
{
    Thread *t=reinterpret_cast<Thread*>(pthread);
    if(Thread::exists(t)==false) return ESRCH;
    if(t==Thread::getCurrentThread()) return EDEADLK;
    if(t->join(value_ptr)==false) return EINVAL;
    return 0;
}

int pthread_detach(pthread_t pthread)
{
    Thread *t=reinterpret_cast<Thread*>(pthread);
    if(Thread::exists(t)==false) return ESRCH;
    t->detach();
    return 0;
}

pthread_t pthread_self()
{
    return reinterpret_cast<pthread_t>(Thread::getCurrentThread());
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return t1==t2;
}

int pthread_attr_init(pthread_attr_t *attr)
{
    //We only use two fields of pthread_attr_t so initialize only these two
    attr->detachstate=PTHREAD_CREATE_JOINABLE;
    attr->stacksize=STACK_DEFAULT_FOR_PTHREAD;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    return 0; //That was easy
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
    *detachstate=attr->detachstate;
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    if(detachstate!=PTHREAD_CREATE_JOINABLE &&
       detachstate!=PTHREAD_CREATE_DETACHED) return EINVAL;
    attr->detachstate=detachstate;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
    *stacksize=attr->stacksize;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if(stacksize<STACK_MIN) return EINVAL;
    if((stacksize % 4) !=0) return EINVAL; //Stack size must be divisible by 4
    attr->stacksize=stacksize;
    return 0;
}

//
// Mutex API
//

int	pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    attr->recursive=PTHREAD_MUTEX_DEFAULT;
    return 0;
}

int	pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    return 0; //Do nothing
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *kind)
{
    *kind=attr->recursive;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind)
{
    switch(kind)
    {
        case PTHREAD_MUTEX_DEFAULT:
            attr->recursive=PTHREAD_MUTEX_DEFAULT;
            return 0;
        case PTHREAD_MUTEX_RECURSIVE:
            attr->recursive=PTHREAD_MUTEX_RECURSIVE;
            return 0;
        default:
            return EINVAL;
    }
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    mutex->owner=0;
    mutex->first=0;
    //No need to initialize mutex->last
    if(attr!=0)
    {
        mutex->recursive= attr->recursive==PTHREAD_MUTEX_RECURSIVE ? 0 : -1;
    } else mutex->recursive=-1;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if(mutex->owner!=0) return EBUSY;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    FastInterruptDisableLock dLock;
    IRQdoMutexLock(mutex,dLock);
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    FastInterruptDisableLock dLock;
    void *p=reinterpret_cast<void*>(Thread::IRQgetCurrentThread());
    if(mutex->owner==0)
    {
        mutex->owner=p;
        return 0;
    }
    if(mutex->owner==p && mutex->recursive>=0)
    {
        mutex->recursive++;
        return 0;
    }
    return EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    #ifndef SCHED_TYPE_EDF
    FastInterruptDisableLock dLock;
    IRQdoMutexUnlock(mutex);
    #else //SCHED_TYPE_EDF
    bool hppw;
    {
        FastInterruptDisableLock dLock;
        hppw=IRQdoMutexUnlock(mutex);
    }
    if(hppw) Thread::yield(); //If the woken thread has higher priority, yield
    #endif //SCHED_TYPE_EDF
    return 0;
}

//
// Condition variable API
//

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    //attr is currently not considered
    cond->first=0;
    //No need to initialize cond->last
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    if(cond->first!=0) return EBUSY;
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    FastInterruptDisableLock dLock;
    Thread *p=Thread::IRQgetCurrentThread();
    WaitingList waiting; //Element of a linked list on stack
    waiting.thread=reinterpret_cast<void*>(p);
    waiting.next=0; //Putting this thread last on the list (lifo policy)
    if(cond->first==0)
    {
        cond->first=&waiting;
        cond->last=&waiting;
    } else {
        cond->last->next=&waiting;
        cond->last=&waiting;
    }
    p->flags.IRQsetCondWait(true);

    IRQdoMutexUnlock(mutex);
    {
        FastInterruptEnableLock eLock(dLock);
        Thread::yield(); //Here the wait becomes effective
    }
    IRQdoMutexLock(mutex,dLock);
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    #ifdef SCHED_TYPE_EDF
    bool hppw=false;
    #endif //SCHED_TYPE_EDF
    {
        FastInterruptDisableLock dLock;
        if(cond->first==0) return 0;

        Thread *t=reinterpret_cast<Thread*>(cond->first->thread);
        t->flags.IRQsetCondWait(false);
        cond->first=cond->first->next;

        #ifdef SCHED_TYPE_EDF
        if(t->IRQgetPriority() >Thread::IRQgetCurrentThread()->IRQgetPriority())
            hppw=true;
        #endif //SCHED_TYPE_EDF
    }
    #ifdef SCHED_TYPE_EDF
    //If the woken thread has higher priority, yield
    if(hppw) Thread::yield();
    #endif //SCHED_TYPE_EDF
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    #ifdef SCHED_TYPE_EDF
    bool hppw=false;
    #endif //SCHED_TYPE_EDF
    {
        FastInterruptDisableLock lock;
        while(cond->first!=0)
        {
            Thread *t=reinterpret_cast<Thread*>(cond->first->thread);
            t->flags.IRQsetCondWait(false);
            cond->first=cond->first->next;

            #ifdef SCHED_TYPE_EDF
            if(t->IRQgetPriority() >
                    Thread::IRQgetCurrentThread()->IRQgetPriority()) hppw=true;
            #endif //SCHED_TYPE_EDF
        }
    }
    #ifdef SCHED_TYPE_EDF
    //If at least one of the woken thread has higher, yield
    if(hppw) Thread::yield();
    #endif //SCHED_TYPE_EDF
    return 0;
}

//
// Once API
//

int	pthread_once(pthread_once_t *once, void (*func)())
{
    if(once==0 || func==0 || once->is_initialized!=1) return EINVAL;
    bool shouldWeRunFunc=false;
    {
        FastInterruptDisableLock dLock;
        if(once->init_executed==0)
        {
            once->init_executed=1;
            shouldWeRunFunc=true;
        }
    }
    if(shouldWeRunFunc) func();
    return 0;
}

} //extern "C"
