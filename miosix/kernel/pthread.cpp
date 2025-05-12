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

/*
 * pthread.cpp Part of the Miosix Embedded OS. Provides a mapping of the
 * posix thread API to the Miosix thread API.
 */

#include <sched.h>
#include <errno.h>
#include <stdexcept>
#include <algorithm>
#include <cxxabi.h> //for __cxxabiv1::__guard
#include "thread.h"
#include "sync.h"
#include "error.h"
#include "pthread_private.h"
#include "kercalls/libc_integration.h"

using namespace std;
using namespace miosix;

//
// Newlib's pthread.h has been patched since Miosix 1.68 to contain a definition
// for pthread_mutex_t and pthread_cond_t that allows a fast implementation
// of mutexes and condition variables. This *requires* to use an up-to-date gcc.
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
    Priority priority=DEFAULT_PRIORITY;
    if(attr!=nullptr)
    {
        if(attr->detachstate==PTHREAD_CREATE_DETACHED) opt=Thread::DETACHED;
        stacksize=attr->stacksize;
        #ifndef SCHED_TYPE_EDF
        priority=max(0,min(NUM_PRIORITIES-1,attr->schedparam.sched_priority));
        #endif //SCHED_TYPE_EDF
    }
    Thread *result=Thread::create(start,stacksize,priority,arg,opt);
    if(result==0) return EAGAIN;
    *pthread=reinterpret_cast<pthread_t>(result);
    return 0;
}

int pthread_join(pthread_t pthread, void **value_ptr)
{
    Thread *t=reinterpret_cast<Thread*>(pthread);
    if(t==Thread::getCurrentThread()) return EDEADLK;
    if(t->join(value_ptr)==false) return EINVAL;
    return 0;
}

int pthread_detach(pthread_t pthread)
{
    Thread *t=reinterpret_cast<Thread*>(pthread);
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
    //We only use three fields of pthread_attr_t so initialize only these
    attr->detachstate=PTHREAD_CREATE_JOINABLE;
    attr->stacksize=STACK_DEFAULT_FOR_PTHREAD;
    #ifndef SCHED_TYPE_EDF
    attr->schedparam.sched_priority=DEFAULT_PRIORITY;
    #endif //SCHED_TYPE_EDF
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
    attr->stacksize=stacksize;
    return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t *attr,
                               struct sched_param *param)
{
    *param = attr->schedparam;
    return 0;
}

int pthread_attr_setschedparam(pthread_attr_t *attr,
                               const struct sched_param *param)
{
    attr->schedparam = *param;
    return 0;
}

#ifndef SCHED_TYPE_EDF
int sched_get_priority_max(int policy)
{
    (void)policy;
    // The value for NUM_PRIORITIES is configured in miosix_settings.h
    return NUM_PRIORITIES - 1;
}

int sched_get_priority_min(int policy)
{
    (void)policy;
    return 0;
}
#endif //SCHED_TYPE_EDF

void pthread_yield()
{
    Thread::yield();
}

int sched_yield()
{
    Thread::yield();
    return 0;
}

//
// Mutex API
//

//The pthread_cond_t API is implemented simply as a wrapper around the native
//Miosix C++ Mutex or FastMutex. Therefore the memory layout of pthread_cond_t
//should be enough to hold either of these plus an int to differentiate what
//kind of mutex it is
static_assert(sizeof(pthread_mutex_t)>=sizeof(FastMutex)+sizeof(int),"Invalid pthread_mutex_t size");
static_assert(sizeof(pthread_mutex_t)>=sizeof(Mutex)+sizeof(int),"Invalid pthread_mutex_t size");

/**
 * Decide whether the pthread_mutex_t has priority inheritance depending on
 * compile-time overrides or the dynamic (run-time) type. We rely on compiler
 * optimizations to remove dead code when using compile-time overrides.
 * \param type dynamic type, either PTHREAD_PRIO_NONE or PTHREAD_PRIO_INHERIT
 * \return true if the mutex has priority inheritance
 */
static inline bool hasPriorityInheritance(int type)
{
    if(pthreadMutexProtocolOverride==PthreadMutexProtocol::FORCE_PRIO_INHERIT)
        return true;
    else if(pthreadMutexProtocolOverride==PthreadMutexProtocol::FORCE_PRIO_NONE)
        return false;
    else return type!=PTHREAD_PRIO_NONE;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    attr->recursive=PTHREAD_MUTEX_DEFAULT;
    attr->prio=PTHREAD_PRIO_NONE;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
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
        case PTHREAD_MUTEX_NORMAL:
        case PTHREAD_MUTEX_DEFAULT:
        case PTHREAD_MUTEX_RECURSIVE:
            attr->recursive=kind;
            return 0;
        default:
            return EINVAL;
    }
}

int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)
{
    switch(protocol)
    {
        case PTHREAD_PRIO_NONE:
        case PTHREAD_PRIO_INHERIT:
            attr->prio=protocol;
            return 0;
        default:
            return EINVAL;
    }
}

int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, int *protocol)
{
    *protocol=attr->prio;
    return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    auto option=MutexOptions::DEFAULT;
    if(attr->recursive==PTHREAD_MUTEX_RECURSIVE) option=MutexOptions::RECURSIVE;
    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        if(hasPriorityInheritance(attr->prio))
        {
            mutex->type=PTHREAD_PRIO_INHERIT;
            new (mutex) Mutex(option);
        } else {
            mutex->type=PTHREAD_PRIO_NONE;
            new (mutex) FastMutex(option);
        }
    #ifndef __NO_EXCEPTIONS
    } catch(bad_alloc&) {
        return ENOMEM; //Implementation may allocate for some scheduler type
    }
    #endif //__NO_EXCEPTIONS
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if(hasPriorityInheritance(mutex->type))
    {
        auto *impl=reinterpret_cast<Mutex*>(mutex);
        if(impl->isLocked()) return EBUSY;
        impl->~Mutex(); //Call destructor manually
    } else {
        auto *impl=reinterpret_cast<FastMutex*>(mutex);
        if(impl->isLocked()) return EBUSY;
        impl->~FastMutex(); //Call destructor manually
    }
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        //NOTE: Handling FastMutex first speeds up the likely case
        if(hasPriorityInheritance(mutex->type)==false)
            return reinterpret_cast<FastMutex*>(mutex)->lock();
        else return reinterpret_cast<Mutex*>(mutex)->lock();
    #ifndef __NO_EXCEPTIONS
    } catch(bad_alloc&) {
        return ENOMEM; //Implementation may allocate for some scheduler type
    }
    #endif //__NO_EXCEPTIONS
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    //NOTE: Handling FastMutex first speeds up the likely case
    if(hasPriorityInheritance(mutex->type)==false)
         return reinterpret_cast<FastMutex*>(mutex)->tryLock() ? 0 : EBUSY;
    else return reinterpret_cast<Mutex*>(mutex)->tryLock() ? 0 : EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    //NOTE: Handling FastMutex first speeds up the likely case
    if(hasPriorityInheritance(mutex->type)==false)
         return reinterpret_cast<FastMutex*>(mutex)->unlock();
    else return reinterpret_cast<Mutex*>(mutex)->unlock();
}

//
// Condition variable API
//

//The pthread_cond_t API is implemented simply as a wrapper around the native
//Miosix C++ ConditionVariable. Therefore the memory layout of pthread_cond_t
//and of ConditionVariable must be enough to hold a ConditionVariable.
static_assert(sizeof(pthread_cond_t)>=sizeof(ConditionVariable),"Invalid pthread_cond_t size");

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    //attr is currently not considered
    //NOTE: pthread_condattr_setclock is not supported, the only clock supported
    //for pthread_cond_timedwait is CLOCK_MONOTONIC
    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        new (cond) ConditionVariable; //Placement new as cond is a C type
        return 0;
    #ifndef __NO_EXCEPTIONS
    } catch(bad_alloc&) {
        return ENOMEM; //Implementation may allocate for some scheduler type
    }
    #endif //__NO_EXCEPTIONS
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    auto *impl=reinterpret_cast<ConditionVariable*>(cond);
    if(!impl->empty()) return EBUSY;
    impl->~ConditionVariable(); //Call destructor manually
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    auto *impl=reinterpret_cast<ConditionVariable*>(cond);
    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        //NOTE: Handling FastMutex first speeds up the likely case
        if(hasPriorityInheritance(mutex->type)==false)
            impl->wait(*reinterpret_cast<FastMutex*>(mutex));
        else impl->wait(*reinterpret_cast<Mutex*>(mutex));
        return 0;
    #ifndef __NO_EXCEPTIONS
    } catch(bad_alloc&) {
        return ENOMEM; //Implementation may allocate for some scheduler type
    }
    #endif //__NO_EXCEPTIONS
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
    auto *impl=reinterpret_cast<ConditionVariable*>(cond);
    TimedWaitResult res;
    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        //NOTE: Handling FastMutex first speeds up the likely case
        if(hasPriorityInheritance(mutex->type)==false)
            res=impl->timedWait(*reinterpret_cast<FastMutex*>(mutex),timespec2ll(abstime));
        else res=impl->timedWait(*reinterpret_cast<Mutex*>(mutex),timespec2ll(abstime));
        return res == TimedWaitResult::Timeout ? ETIMEDOUT : 0;
    #ifndef __NO_EXCEPTIONS
    } catch(bad_alloc&) {
        return ENOMEM; //Implementation may allocate for some scheduler type
    }
    #endif //__NO_EXCEPTIONS
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    reinterpret_cast<ConditionVariable*>(cond)->signal();
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    reinterpret_cast<ConditionVariable*>(cond)->broadcast();
    return 0;
}

//
// Once API
//

int pthread_once(pthread_once_t *once, void (*func)())
{
    if(once==nullptr || func==nullptr || once->is_initialized!=1) return EINVAL;

    // Miosix provides a custom optimized implementation of the __cxa_guard_*
    // functions to initialize static C++ objects, which happens to fit also
    // for this use case, so reuse it
    if(once->init_executed==1) return 0;
    auto *guard=reinterpret_cast<__cxxabiv1::__guard*>(&once->init_executed);
    if(__cxxabiv1::__cxa_guard_acquire(guard)==0) return 0;
    #ifdef __NO_EXCEPTIONS
    func();
    #else //__NO_EXCEPTIONS
    try {
        func();
    } catch(...) {
        __cxxabiv1::__cxa_guard_abort(guard);
        throw;
    }
    #endif //__NO_EXCEPTIONS
    __cxxabiv1::__cxa_guard_release(guard);
    return 0;
}

int pthread_setcancelstate(int state, int *oldstate) { return 0; } //Stub

#ifdef WITH_PTHREAD_EXIT

void pthread_exit(void *returnValue)
{
    throw PthreadExitException(returnValue);
}

#endif //WITH_PTHREAD_EXIT

#ifdef WITH_PTHREAD_KEYS

typedef void (*destructor_type)(void *);

static FastMutex pthreadKeyMutex;
static bool keySlotUsed[MAX_PTHREAD_KEYS]={false};
static destructor_type keyDestructor[MAX_PTHREAD_KEYS]={nullptr};

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    Lock<FastMutex> l(pthreadKeyMutex);
    for(unsigned int i=0;i<MAX_PTHREAD_KEYS;i++)
    {
        if(keySlotUsed[i]) continue;
        keySlotUsed[i]=true;
        keyDestructor[i]=destructor;
        *key=i;
        return 0;
    }
    return EAGAIN;
}

int pthread_key_delete(pthread_key_t key)
{
    Lock<FastMutex> l(pthreadKeyMutex);
    if(keySlotUsed[key]==false) return EINVAL;
    keySlotUsed[key]=false;
    keyDestructor[key]=nullptr;
    return 0;
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    //This cast because POSIX misunderstood how const works
    auto v = const_cast<void * const>(value);
    return Thread::getCurrentThread()->setPthreadKeyValue(key,v);
}

void *pthread_getspecific(pthread_key_t key)
{
    return Thread::getCurrentThread()->getPthreadKeyValue(key);
}

#endif //WITH_PTHREAD_KEYS

} //extern "C"

#ifdef WITH_PTHREAD_KEYS

namespace miosix {

void callPthreadKeyDestructors(void *pthreadKeyValues[MAX_PTHREAD_KEYS])
{
    for(unsigned int i=0;i<MAX_PTHREAD_KEYS;i++)
    {
        if(pthreadKeyValues[i]==nullptr) continue; //No value, nothing to do
        //POSIX wants destructor called after key value is set to nullptr
        auto temp=pthreadKeyValues[i];
        pthreadKeyValues[i]=nullptr;
        destructor_type destructor=nullptr;
        {
            Lock<FastMutex> l(pthreadKeyMutex);
            destructor=keyDestructor[i];
        }
        if(destructor) destructor(temp);
    }
    //NOTE: the POSIX spec state that calling a destructor may set another key
    //and we should play whack-a-mole calling again destructors till all values
    //become nulllptr, which may lead to an infinite loop or we may choose to
    //stop after PTHREAD_DESTRUCTOR_ITERATIONS. For now we don't do it, and act
    //as if PTHREAD_DESTRUCTOR_ITERATIONS is 1 on Miosix
}

} //namespace miosix

#endif //WITH_PTHREAD_KEYS
