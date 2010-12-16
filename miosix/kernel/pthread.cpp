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

/*
 * pthread.cpp Part of the Miosix Embedded OS. Provides a mapping of the
 * posix thread API to the Miosix thread API.
 */

#include <pthread.h>
#include <errno.h>
#include <stdexcept>
#include "kernel.h"
#include "sync.h"

//
// Newlib ships with many pthread.h, one "default", one for linux, etc.
// And all pthread types such as pthread_mutex_t change accordingly.
// We need the default pthread.h, the one in newlib/libc/include/pthread.h
// So if compiling this file fails, check if pthread.h is correct
//

//These functions needs to be callable from C
extern "C" {

//
// Thread related API
//

int pthread_create(pthread_t *pthread, const pthread_attr_t *attr,
    void *(*start)(void *), void *arg)
{
    miosix::Thread::Options opt=miosix::Thread::JOINABLE;
    unsigned int stacksize=miosix::STACK_DEFAULT_FOR_PTHREAD;
    if(attr!=NULL)
    {
        if(attr->detachstate==PTHREAD_CREATE_DETACHED)
            opt=miosix::Thread::DEFAULT;
        stacksize=attr->stacksize;
    }
    miosix::Thread *result=miosix::Thread::create(start,stacksize,1,arg,opt);
    if(result==0) return EAGAIN;
    *pthread=reinterpret_cast<pthread_t>(result);
    return 0;
}

int pthread_join(pthread_t pthread, void **value_ptr)
{
    miosix::Thread *t=reinterpret_cast<miosix::Thread*>(pthread);
    if(miosix::Thread::exists(t)==false) return ESRCH;
    if(t==miosix::Thread::getCurrentThread()) return EDEADLK;
    if(t->join(value_ptr)==false) return EINVAL;
    return 0;
}

int pthread_detach(pthread_t pthread)
{
    miosix::Thread *t=reinterpret_cast<miosix::Thread*>(pthread);
    if(miosix::Thread::exists(t)==false) return ESRCH;
    t->detach();
    return 0;
}

pthread_t pthread_self()
{
    return reinterpret_cast<pthread_t>(miosix::Thread::getCurrentThread());
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return t1==t2;
}

int pthread_attr_init(pthread_attr_t *attr)
{
    //We only use these two fields of pthread_attr_t so onle these
    //are initialized
    attr->detachstate=PTHREAD_CREATE_JOINABLE;
    attr->stacksize=miosix::STACK_DEFAULT_FOR_PTHREAD;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    //That was easy
    return 0;
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
    if(stacksize<miosix::STACK_MIN) return EINVAL;
    if((stacksize % 4) !=0) return EINVAL; //Stack size must be divisible by 4
    attr->stacksize=stacksize;
    return 0;
}

//
// Mutex API
//

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    //attr is currently not considered
    miosix::PauseKernelLock lock;
    miosix::Mutex *m=0;
    #ifdef __NO_EXCEPTIONS
    m=new miosix::Mutex;
    #else //__NO_EXCEPTIONS
    try {
        m=new miosix::Mutex;
    } catch(std::bad_alloc&)
    {
        m=0;
    }
    #endif //__NO_EXCEPTIONS
    if(m==0) return ENOMEM;
    *mutex=reinterpret_cast<pthread_mutex_t>(m);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    miosix::PauseKernelLock lock;
    miosix::Mutex *m=reinterpret_cast<miosix::Mutex*>(*mutex);
    if(m->PKisLocked()) return EBUSY;
    delete m;
    return 0;
}

/*
 * Can only be called with kernel paused, to avoid race conditions.
 * \return a Miosix mutex from a pthread mutex. If the pthread mutex value is
 * PTHREAD_MUTEX_INITIALIZER, allocates a new mutex.
 * If the memory allocation fails, return zero however.
 */
static inline miosix::Mutex *PKgetMutex(pthread_mutex_t *mutex)
{
    if(*mutex!=PTHREAD_MUTEX_INITIALIZER)
    {
        return reinterpret_cast<miosix::Mutex*>(*mutex);
    } else {
        miosix::Mutex *m=0;
        #ifdef __NO_EXCEPTIONS
        m=new miosix::Mutex;
        #else //__NO_EXCEPTIONS
        try {
            m=new miosix::Mutex;
        } catch(std::bad_alloc&)
        {
            m=0;
        }
        #endif //__NO_EXCEPTIONS
        //Assign the correct id to the mutex
        if(m!=0) *mutex=reinterpret_cast<pthread_mutex_t>(m);
        return m;
    }
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    miosix::PauseKernelLock lock;
    miosix::Mutex *m=PKgetMutex(mutex);
    if(m==0) return EINVAL;
    m->PKlock(lock);
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    miosix::PauseKernelLock lock;
    miosix::Mutex *m=PKgetMutex(mutex);
    if(m==0) return EINVAL;
    if(m->PKtryLock(lock)==false) return EBUSY;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    miosix::PauseKernelLock lock;
    miosix::Mutex *m=PKgetMutex(mutex);
    if(m==0) return EINVAL;
    m->PKunlock(lock);
    return 0;
}

//
// Condition variable API
//

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    //attr is currently not considered
    miosix::PauseKernelLock lock;
    miosix::ConditionVariable *c=0;
    #ifdef __NO_EXCEPTIONS
    c=new miosix::ConditionVariable;
    #else //__NO_EXCEPTIONS
    try {
        c=new miosix::ConditionVariable;
    } catch(std::bad_alloc&)
    {
        c=0;
    }
    #endif //__NO_EXCEPTIONS
    if(c==0) return ENOMEM;
    *cond=reinterpret_cast<pthread_cond_t>(c);
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    miosix::PauseKernelLock lock;
    miosix::ConditionVariable *c=
            reinterpret_cast<miosix::ConditionVariable*>(*cond);
    if(c->PKisEmpty()==false) return EBUSY;
    delete c;
    return 0;
}

/*
 * Can only be called with kernel paused, to avoid race conditions.
 * \return a Miosix condition variable from a pthread condition variable.
 * If the pthread mutex value is PTHREAD_MUTEX_INITIALIZER, allocates a new
 * mutex. If the memory allocation fails, return zero however.
 */
static inline miosix::ConditionVariable *PKgetCondVar(pthread_cond_t *cond)
{
    if(*cond!=PTHREAD_COND_INITIALIZER)
    {
        return reinterpret_cast<miosix::ConditionVariable*>(*cond);
    } else {
        miosix::ConditionVariable *c=0;
        #ifdef __NO_EXCEPTIONS
        c=new miosix::ConditionVariable;
        #else //__NO_EXCEPTIONS
        try {
            c=new miosix::ConditionVariable;
        } catch(std::bad_alloc&)
        {
            c=0;
        }
        #endif //__NO_EXCEPTIONS
        //Assign the correct id to the condition variable
        if(c!=0) *cond=reinterpret_cast<pthread_mutex_t>(c);
        return c;
    }
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    miosix::PauseKernelLock lock;
    miosix::ConditionVariable *c=PKgetCondVar(cond);
    if(c==0) return EINVAL;
    if(*mutex==PTHREAD_MUTEX_INITIALIZER)
    {
        //This is an error, since the mutex is supposed to be locked
        //prior to waiting on a condition variable
        return EINVAL;
    }
    miosix::Mutex *m=reinterpret_cast<miosix::Mutex*>(*mutex);
    c->PKwait(*m,lock);
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    //We call PKgetCondVar with kernel paused to avoid the race condition
    //of doubly initializing the condition variable if its value is
    //PTHREAD_COND_INITIALIZER
    miosix::ConditionVariable *c=0;
    {
        miosix::PauseKernelLock lock;
        c=PKgetCondVar(cond);
    }
    if(c==0) return EINVAL;
    //signal is instead called without kernel paused, since it might yield
    c->signal();
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    //We call PKgetCondVar with kernel paused to avoid the race condition
    //of doubly initializing the condition variable if its value is
    //PTHREAD_COND_INITIALIZER
    miosix::ConditionVariable *c=0;
    {
        miosix::PauseKernelLock lock;
        c=PKgetCondVar(cond);
    }
    if(c==0) return EINVAL;
    //broadcast is instead called without kernel paused, since it might yield
    c->broadcast();
    return 0;
}

} //extern "C"
