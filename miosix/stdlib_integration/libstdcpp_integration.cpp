/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013 by Terraneo Federico *
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
//// Settings
#include "config/miosix_settings.h"
//// Console
#include "kernel/logging.h"
//// kernel interface
#include "kernel/kernel.h"

using namespace std;

//
// C++ exception support
// =====================

#ifdef __NO_EXCEPTIONS
/*
 * If not using exceptions, ovverride the default new, delete with
 * an implementation that does not throw, to minimze code size
 */
void *operator new(size_t size) throw()
{
    return malloc(size);
}

void *operator new[](size_t size) throw()
{
    return malloc(size);
}

void operator delete(void *p) throw()
{
    free(p);
}

void operator delete[](void *p) throw()
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
void __throw_runtime_error(const char*) { _exit(1); }
void __throw_range_error(const char*) { _exit(1); }
void __throw_overflow_error(const char*) { _exit(1); }
void __throw_underflow_error(const char*) { _exit(1); }
void __throw_ios_failure(const char*) { _exit(1); }
void __throw_system_error(int) { _exit(1); }
void __throw_future_error(int) { _exit(1); }
void __throw_bad_function_call() { _exit(1); }
} //namespace std

#endif //__NO_EXCEPTIONS

namespace miosix {

class CppReentrancyAccessor
{
public:
    static __cxxabiv1::__cxa_eh_globals *getEhGlobals()
    {
        return &miosix::Thread::getCurrentThread()->cppReent.eh_globals;
    }

    #ifndef __ARM_EABI__
    static void *getSjljPtr()
    {
        return miosix::Thread::getCurrentThread()->cppReent.sjlj_ptr;
    }

    static void setSjljPtr(void *ptr)
    {
        miosix::Thread::getCurrentThread()->cppReent.sjlj_ptr=ptr;
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

//This is weird, despite almost everywhere in GCC's documentation it is said
//that __guard is 8 bytes, it is actually only four.
union MiosixGuard
{
    miosix::Thread *owner;
    unsigned int flag;
};

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
    miosix::InterruptDisableLock dLock;
    volatile MiosixGuard *guard=reinterpret_cast<volatile MiosixGuard*>(g);
    for(;;)
    {
        if(guard->flag==1) return 0; //Object already initialized, good
        
        if(guard->flag==0)
        {
            //Object uninitialized, and no other thread trying to initialize it
            guard->owner=miosix::Thread::IRQgetCurrentThread();

            //guard->owner serves the double task of being the thread id of
            //the thread initializing the object, and being the flag to signal
            //that the object is initialized or not. If bit #0 of guard->owner
            //is @ 1 the object is initialized. All this works on the assumption
            //that Thread* pointers never have bit #0 @ 1, and this assetion
            //checks that this condition really holds
            if(guard->flag & 1) miosix::errorHandler(miosix::UNEXPECTED);
            return 1;
        }

        //If we get here, the object is being initialized by another thread
        if(guard->owner==miosix::Thread::IRQgetCurrentThread())
        {
            //Wait, the other thread initializing the object is this thread?!?
            //We have a recursive initialization error. Not throwing an
            //exception to avoid pulling in exceptions even with -fno-exception
            IRQerrorLog("Recursive initialization\r\n");
            _exit(1);
        }

        {
            miosix::InterruptEnableLock eLock(dLock);
            miosix::Thread::yield(); //Sort of a spinlock, a "yieldlock"...
        }
    }
}

/**
 * Called after the thread has successfully initialized the object
 * \param g guard struct
 */
extern "C" void __cxa_guard_release(__guard *g)
{
    miosix::InterruptDisableLock dLock;
    MiosixGuard *guard=reinterpret_cast<MiosixGuard*>(g);
    guard->flag=1;
}

/**
 * Called if an exception was thrown while the object was being initialized
 * \param g guard struct
 */
extern "C" void __cxa_guard_abort(__guard *g)
{
    miosix::InterruptDisableLock dLock;
    MiosixGuard *guard=reinterpret_cast<MiosixGuard*>(g);
    guard->flag=0;
}

} //namespace __cxxabiv1
