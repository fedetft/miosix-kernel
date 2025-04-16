/***************************************************************************
 *   Copyright (C) 2008-2019 by Terraneo Federico                          *
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

#include "libstdcpp_integration.h"
#include <cstdlib>
#include <unistd.h>
#include <cxxabi.h>
#include <thread>
//// Settings
#include "config/miosix_settings.h"
//// Console
#include "kernel/logging.h"
//// kernel interface
#include "kernel/thread.h"

using namespace std;

//
// C++ exception support
// =====================

#if __cplusplus >= 201703L
#warning: TODO: Override new with alignment (libsupc++/new_opa.cc, new_opv.cc, ...
#warning: TODO: FIX __gthread_key_t in libstdc++/include/std/memory_resource
#endif

#ifdef __NO_EXCEPTIONS
/*
 * If not using exceptions, ovverride the default new, delete with
 * an implementation that does not throw, to minimze code size
 */
void *operator new(size_t size) noexcept
{
    return malloc(size);
}

void *operator new(size_t size, const std::nothrow_t&) noexcept
{
    return malloc(size);
}

void *operator new[](size_t size) noexcept
{
    return malloc(size);
}

void *operator new[](size_t size, const std::nothrow_t&) noexcept
{
    return malloc(size);
}

void operator delete(void *p) noexcept
{
    free(p);
}

void operator delete[](void *p) noexcept
{
    free(p);
}

/**
 * \internal
 * The default version of these functions provided with libstdc++ require
 * exception support. This means that a program using pure virtual functions
 * incurs in the code size penalty of exception support even when compiling
 * without exceptions. By replacing the default implementations with these one
 * the problem is fixed.
 */
extern "C" void __cxxabiv1::__cxa_pure_virtual(void)
{
    errorLog("\n***Pure virtual method called\n");
    _exit(1);
}

extern "C" void __cxxabiv1::__cxa_deleted_virtual(void)
{
    errorLog("\n***Deleted virtual method called\n");
    _exit(1);
}

namespace std {
void terminate()  noexcept { _exit(1); }
void unexpected() noexcept { _exit(1); }
/*
 * This one comes from thread.cc, the need to call the class destructor makes it
 * call __cxa_end_cleanup which pulls in exception code.
 */
extern "C" void* execute_native_thread_routine(void* __p)
{
    thread::_State_ptr __t{ static_cast<thread::_State*>(__p) };
    __t->_M_run();
    return nullptr;
}
} //namespace std

/*
 * If not using exceptions, ovverride these functions with
 * an implementation that does not throw, to minimze code size
 */
namespace std {
void __throw_bad_exception() { _exit(1); }
void __throw_bad_alloc()  { _exit(1); }
void __throw_bad_cast() { _exit(1); }
void __throw_bad_typeid()  { _exit(1); }
void __throw_logic_error(const char*) { _exit(1); }
void __throw_domain_error(const char*) { _exit(1); }
void __throw_invalid_argument(const char*) { _exit(1); }
void __throw_length_error(const char*) { _exit(1); }
void __throw_out_of_range(const char*) { _exit(1); }
void __throw_out_of_range_fmt(const char*, ...) { exit(1); }
void __throw_runtime_error(const char*) { _exit(1); }
void __throw_range_error(const char*) { _exit(1); }
void __throw_overflow_error(const char*) { _exit(1); }
void __throw_underflow_error(const char*) { _exit(1); }
void __throw_system_error(int) { _exit(1); }
void __throw_future_error(int) { _exit(1); }
void __throw_bad_function_call() { _exit(1); }
} //namespace std

namespace __cxxabiv1 {
extern "C" void __cxa_throw_bad_array_length() { exit(1); }
extern "C" void __cxa_bad_cast() { exit(1); }
extern "C" void __cxa_bad_typeid() { exit(1); }
extern "C" void __cxa_throw_bad_array_new_length() { exit(1); }
} //namespace __cxxabiv1

#endif //__NO_EXCEPTIONS

namespace miosix {

class CppReentrancyAccessor
{
public:
    static __cxxabiv1::__cxa_eh_globals *getEhGlobals()
    {
        return &miosix::Thread::getCurrentThread()->cppReentrancyData.eh_globals;
    }

    #ifndef __ARM_EABI__
    static void *getSjljPtr()
    {
        return miosix::Thread::getCurrentThread()->cppReentrancyData.sjlj_ptr;
    }

    static void setSjljPtr(void *ptr)
    {
        miosix::Thread::getCurrentThread()->cppReentrancyData.sjlj_ptr=ptr;
    }
    #endif //__ARM_EABI__
};

} //namespace miosix

/*
 * If exception support enabled, ensure it is thread safe.
 * The functions __cxa_get_globals_fast() and __cxa_get_globals() need to
 * return a per-thread data structure. Given that thread local storage isn't
 * implemented in Miosix, libstdc++ was patched to make these functions syscalls
 */
namespace __cxxabiv1
{

extern "C" __cxa_eh_globals* __cxa_get_globals_fast()
{
    return miosix::CppReentrancyAccessor::getEhGlobals();
}

extern "C" __cxa_eh_globals* __cxa_get_globals()
{
    return miosix::CppReentrancyAccessor::getEhGlobals();
}

#ifndef __ARM_EABI__
extern "C" void _Miosix_set_sjlj_ptr(void* ptr)
{
    miosix::CppReentrancyAccessor::setSjljPtr(ptr);
}

extern "C" void *_Miosix_get_sjlj_ptr()
{
    return miosix::CppReentrancyAccessor::getSjljPtr();
}
#endif //__ARM_EABI__

} //namespace __cxxabiv1

namespace __gnu_cxx {

/**
 * \internal
 * Replaces the default verbose terminate handler.
 * Avoids the inclusion of code to demangle C++ names, which saves code size
 * when using exceptions.
 */
void __verbose_terminate_handler()
{
    errorLog("\n***Unhandled exception thrown\n");
    _exit(1);
}

} //namespace __gnu_cxx




//
// C++ static constructors support, to achieve thread safety
// =========================================================

// __guard serves the double task of being the waiting list head, and being the
// flag to signal that the object is initialized or not.
// This is because, despite almost everywhere in GCC's documentation it is said
// that __guard is 8 bytes, it is actually only 4 so we fit a two bit flag in
// the two least significant bits and a pointer in the other bits.
// Additionally, the compiler adds some instructions to check __guard too, so we
// MUST use bit 0 of __guard to signal that the object is initialized (1) or
// not (0). This implementation works on the assumption that the pointer to list
// item don't use bits 0 and 1, hence the alignas requirement to be extra sure
struct alignas(4) MiosixGuardListItem
{
    miosix::Thread *t;
    MiosixGuardListItem *next;
};

static unsigned int flagGet(__cxxabiv1::__guard *g)
{
    return *reinterpret_cast<unsigned int*>(g) & 0b11;
}

static void flagSet(__cxxabiv1::__guard *g, unsigned int flag)
{
    // NOTE: in the code path following a __cxa_guard_abort the first thread in
    // the list is awakened, but there may be more, so use read-modify-write to
    // set flag bits without losing the list head
    unsigned int *data=reinterpret_cast<unsigned int*>(g);
    *data = (*data & ~0b11) | (flag & 0b11);
}

static void flagSetAndClearListHead(__cxxabiv1::__guard *g, unsigned int flag)
{
    unsigned int *data=reinterpret_cast<unsigned int*>(g);
    *data = flag & 0b11;
}

static MiosixGuardListItem *listHeadGet(__cxxabiv1::__guard *g)
{
    unsigned int *data=reinterpret_cast<unsigned int*>(g);
    return reinterpret_cast<MiosixGuardListItem*>((*data) & ~0b11);
}

static void listHeadSet(__cxxabiv1::__guard *g, MiosixGuardListItem *head)
{
    unsigned int *data=reinterpret_cast<unsigned int*>(g);
    *data = (*data & 0b11) | (reinterpret_cast<unsigned int>(head) & ~0b11);
}

namespace __cxxabiv1
{
/**
 * Used to initialize static objects only once, in a threadsafe way
 * \param g guard struct
 * \return 0 if object already initialized, 1 if this thread has to initialize
 * it, or lock if another thread has already started initializing it
 */
extern "C" int __cxa_guard_acquire(__guard *g)
{
    miosix::PauseKernelLock dLock;
    for(;;)
    {
        auto flag=flagGet(g);
        if(flag==1) return 0; // Object already initialized, good
        
        if(flag==0)
        {
            // Object uninitialized, and no other thread trying to initialize it
            flagSet(g,2);
            return 1;
        }
        // If we get here, the object is being initialized by another thread,
        // so we need to wait.
        MiosixGuardListItem item;
        item.t=miosix::Thread::PKgetCurrentThread();
        // NOTE: we could sort the list of waiting threads by priority but the
        // most common case is that the object initialization completes
        // successfully and in this case all waiting threads are awakend at the
        // same time, so lifo order is chosen to have O(1) insertion/removal.
        // Moreover, if in the future we really want to sort the list by
        // priority we must re-sort it if a thread waiting here has also locked
        // a mutex with priority inheritance and its priority got boosted...
        item.next=listHeadGet(g);
        listHeadSet(g,&item);
        // While loop to protect against spurious wakeups
        while(item.t!=nullptr) item.t->PKrestartKernelAndWait(dLock);
    }
}

/**
 * Called after the thread has successfully initialized the object
 * \param g guard struct
 */
extern "C" void __cxa_guard_release(__guard *g) noexcept
{
    miosix::PauseKernelLock dLock;
    for(auto *walk=listHeadGet(g);walk!=nullptr;walk=walk->next)
    {
        walk->t->PKwakeup();
        walk->t=nullptr;
    }
    flagSetAndClearListHead(g,1); //All threads awakened, also clear list ptr
}

/**
 * Called if an exception was thrown while the object was being initialized
 * \param g guard struct
 */
extern "C" void __cxa_guard_abort(__guard *g) noexcept
{
    miosix::PauseKernelLock dLock;
    auto *head=listHeadGet(g);
    if(head!=nullptr)
    {
        head->t->PKwakeup();
        head->t=nullptr;
        listHeadSet(g,head->next);
    }
    flagSet(g,0);
}

} //namespace __cxxabiv1

//
// libatomic support, to provide thread safe atomic operation fallbacks
// ====================================================================

// Not using the fast version, as these may be used before the kernel is started

extern "C" unsigned int libat_quick_lock_n(void *ptr)
{
    miosix::globalIrqLock();
    return 0;
}

extern "C" void libat_quick_unlock_n(void *ptr, unsigned int token)
{
    miosix::globalIrqUnlock();
}

// These are to implement "heavy" atomic operations, which are not used in
// libstdc++. For now let's keep them disbaled.

// extern "C" void libat_lock_n(void *ptr, size_t n)
// {
//     miosix::pauseKernel();
// }
// 
// extern "C" void libat_unlock_n(void *ptr, size_t n)
// {
//     miosix::restartKernel();
// }
