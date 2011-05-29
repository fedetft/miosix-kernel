/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011 by Terraneo Federico             *
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
 * syscalls.cpp Part of the Miosix Embedded OS. Provides an implementation
 * of the functions reqired to interface newlib and libstdc++ with an OS
 */

#include "syscalls.h"
#include "syscalls_types.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <reent.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <cxxabi.h>
// Settings
#include "config/miosix_settings.h"
// Filesystem
#include "filesystem/filesystem.h"
// Console
#include "logging.h"
#include "interfaces/console.h"
// kernel interface
#include "kernel.h"
#include "interfaces/bsp.h"

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
 * The default version of this function provided with libstdc++ requires
 * exception support. This means that a program using pure virtual functions
 * incurs in the code size penalty of exception support even when compiling
 * without exceptions. By replacing the default implementation with this one the
 * problem is fixed.
 */
extern "C" void __cxa_pure_virtual(void)
{
    miosix::errorLog("\r\n***Pure virtual method called\r\n");
    _exit(1);//Not calling abort() since it pulls in malloc.
}

#else //__NO_EXCEPTIONS

/*
 * If compiling with exception support, make it thread safe
 */

namespace miosix {

class ExceptionHandlingAccessor
{
public:
    static __cxxabiv1::__cxa_eh_globals *getEhGlobals()
    {
        return &miosix::Thread::getCurrentThread()->exData.eh_globals;
    }

    #ifndef __ARM_EABI__
    static void *getSjljPtr()
    {
        return miosix::Thread::getCurrentThread()->exData.sjlj_ptr;
    }

    static void setSjljPtr(void *ptr)
    {
        miosix::Thread::getCurrentThread()->exData.sjlj_ptr=ptr;
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
    return miosix::ExceptionHandlingAccessor::getEhGlobals();
}

extern "C" __cxa_eh_globals* __cxa_get_globals()
{
    return miosix::ExceptionHandlingAccessor::getEhGlobals();
}

#ifndef __ARM_EABI__
extern "C" void _Miosix_set_sjlj_ptr(void* ptr)
{
    miosix::ExceptionHandlingAccessor::setSjljPtr(ptr);
}

extern "C" void *_Miosix_get_sjlj_ptr()
{
    return miosix::ExceptionHandlingAccessor::getSjljPtr();
}
#endif //__ARM_EABI__

} //namespace __cxxabiv1
#endif //__NO_EXCEPTIONS

namespace __gnu_cxx {

/**
 * \internal
 * Replaces the default verbose terminate handler.
 * Avoids the inclusion of code to demangle C++ names, which saves code size
 * when using exceptions.
 */
void __verbose_terminate_handler()
{
    miosix::errorLog("\r\n***Unhandled exception thrown\r\n");
    _exit(1);
}

}//namespace __gnu_cxx




//
// C++ static constructors support, to achieve thread safety
// =========================================================

//MUST be 8 bytes in size, because that's the size of __guard
struct MiosixGuard
{
	char initialized;
    char padding[3];
	miosix::Thread *owner;
};

namespace __cxxabiv1
{
/**
 * Used to initialize static objects only once, in a threadsafe way
 * \param g guard struct, 8 bytes in size
 * \return 0 if object already initialized, 1 if this thread has to initialize
 * it, or lock if another thread has already started initializing it
 */
extern "C" int __cxa_guard_acquire(__guard *g)
{
    miosix::InterruptDisableLock dLock;
    volatile MiosixGuard *guard=reinterpret_cast<volatile MiosixGuard*>(g);
    for(;;)
    {
        if(guard->initialized) return 0; //Object already initialized, good
        
        if(guard->owner==0)
        {
            //Object uninitialized, and no other thread trying to initialize it
            guard->owner=miosix::Thread::IRQgetCurrentThread();
            return 1;
        }

        //If we get here, the object is being initialized by another thread
        if(guard->owner==miosix::Thread::IRQgetCurrentThread())
        {
            //Wait, the other thread initializing the object is this thread?!?
            //We have a recursive initialization error. Not throwing an
            //exception to avoid pulling in exceptions even with -fno-exception
            miosix::Console::IRQwrite("Recursive initialization\r\n");
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
 * \param g guard struct, 8 bytes in size
 */
extern "C" void __cxa_guard_release(__guard *g)
{
    miosix::InterruptDisableLock dLock;
    MiosixGuard *guard=reinterpret_cast<MiosixGuard*>(g);
    guard->initialized=1;
    guard->owner=0;
}

/**
 * Called if an exception was thrown while the object was being initialized
 * \param g guard struct, 8 bytes in size
 */
extern "C" void __cxa_guard_abort(__guard *g)
{
    miosix::InterruptDisableLock dLock;
    MiosixGuard *guard=reinterpret_cast<MiosixGuard*>(g);
    //Leaving guard->initialized @ 0
    guard->owner=0;
}

} //namespace __cxxabiv1




//
// C atexit support, for thread safety and code size optimizations
// ===============================================================

#ifdef __cplusplus
extern "C" {
#endif

// Prior to Miosix 1.58 atexit was effectively unimplemented, but its partial
// support in newlib used ~384bytes of RAM. Currently it is still unimplemented
// but newlib has been patched so that no RAM is wasted. In future, by
// implementing those function below it would be possible with a #define in
// miosix_settings.h to configure Miosix to enable atexit
// (and pay for the RAM usage), or leave it disabled.

/**
 * Function called by atexit(), on_exit() and __cxa_atexit() to register
 * C functions/C++ destructors to be run at program termintion.
 * It is called in this way:
 * atexit():       __register_exitproc(__et_atexit, fn, 0,   0)
 * on_exit():      __register_exitproc(__et_onexit, fn, arg, 0)
 * __cxa_atexit(): __register_exitproc(__et_cxa,    fn, arg, d)
 * \param type to understand if the function was called by atexit, on_exit, ...
 * \param fn pointer to function to be called
 * \param arg 0 in case of atexit, function argument in case of on_exit,
 * "this" parameter for C++ destructors registered with __cxa_atexit
 * \param d __dso_handle used to selectively call C++ destructors of a shared
 * library loaded dynamically, unused since Miosix does not support shared libs
 * \return 0 on success
 */
int __register_exitproc(int type, void (*fn)(void), void *arg, void *d)
{
    return 0;
}

/**
 * Called by exit() to call functions registered through atexit()
 * \param code the exit code, for example with exit(1), code==1
 * \param d __dso_handle, see __register_exitproc
 */
void __call_exitprocs(int code, void *d)
{

}

/**
 * \internal
 * Required by C++ standard library.
 * See http://lists.debian.org/debian-gcc/2003/07/msg00057.html
 */
void *__dso_handle=(void*) &__dso_handle;




//
// C/C++ system calls, to support malloc, printf, fopen, etc.
// ==========================================================

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/**
 * \internal
 * _exit, restarts the system
 */
void _exit(int n)
{
    miosix::reboot();
    //Never reach here
    for(;;) ; //Required to avoid a warning about noreturn functions
}

// This holds the max heap usage since the program started.
// It is written by _sbrk_r and read by getMaxHeap()
static unsigned int max_heap_end=0;

unsigned int getMaxHeap()
{
    return max_heap_end;
}

void *_sbrk_r(struct _reent *ptr, ptrdiff_t incr)
{
    //This is the absolute start of the heap
    extern char _end asm("_end"); //defined in the linker script
    //This is the absolute end of the heap
    extern char _heap_end asm("_heap_end"); //defined in the linker script
    //This holds the current end of the heap (static)
    static char *cur_heap_end=NULL;
    //This holds the previous end of the heap
    char *prev_heap_end;

    //Check if it's first time called
    if(cur_heap_end==NULL) cur_heap_end=&_end;

    prev_heap_end=cur_heap_end;
    if((cur_heap_end+incr)>&_heap_end)
    {
        //bad, heap overflow
        #ifdef __NO_EXCEPTIONS
        // When exceptions are disabled operator new would return 0, which would
        // cause undefined behaviour. So When exceptions are disabled, a heap
        // overflow causes a reboot.
        miosix::errorLog("\r\n***Heap overflow\r\n");
        _exit(1);
        #else //__NO_EXCEPTIONS
        return reinterpret_cast<void*>(-1);
        #endif //__NO_EXCEPTIONS
    }
    cur_heap_end+=incr;

    if(reinterpret_cast<unsigned int>(cur_heap_end) > max_heap_end)
        max_heap_end=reinterpret_cast<unsigned int>(cur_heap_end);
    
    return reinterpret_cast<void*>(prev_heap_end);
}

/**
 * \internal
 * __malloc_lock, called by malloc to ensure no context switch happens during
 * memory allocation (the heap is global and shared between the threads, so
 * memory allocation should not be interrupted by a context switch)
 *
 *	WARNING:
 *	pauseKernel() does not stop interrupts, so interrupts may occur
 *	during memory allocation. So NEVER use malloc inside an interrupt!
 *	Also beware that some newlib functions, like printf, iprintf...
 *	do call malloc, so you must not use them inside an interrupt.
 */
void __malloc_lock()
{
    miosix::pauseKernel();
}

/**
 * \internal
 * __malloc_unlock, called by malloc after performing operations on the heap
 */
void __malloc_unlock()
{
    miosix::restartKernel();
}

/**
 * \internal
 * _open_r, Open a file
 */
int _open_r(struct _reent *ptr, const char *name, int flags, int mode)
{
    #ifdef WITH_FILESYSTEM
    flags++;//Increment to allow checking read/write mode, see fcntl.h
    return miosix::Filesystem::instance().openFile(name,flags,mode);
    #else //WITH_FILESYSTEM
    miosix::errorLog("\r\n***Kernel compiled without filesystem support\r\n");
    return -1;
    #endif //WITH_FILESYSTEM
}

/**
 * \internal
 * _close_r, Close a file
 */
int _close_r(struct _reent *ptr, int fd)
{
    #ifdef WITH_FILESYSTEM
    if(fd<3) return -1; //Can't close stdin, stdout and stderr
    return miosix::Filesystem::instance().closeFile(fd);
    #else //WITH_FILESYSTEM
    miosix::errorLog("\r\n***Kernel compiled without filesystem support\r\n");
    return -1;
    #endif //WITH_FILESYSTEM
}

/**
 * \internal
 * _write (for C++ library)
 */
int _write(int fd, const void *buf, size_t cnt)
{ 
    switch(fd)
    {
        case STDOUT_FILENO:
        case STDERR_FILENO:
        {
            const char *buffer=static_cast<const char*>(buf);
            const char *chunkstart=buffer;
            //Writing to standard output/error is redirected to the Console
            unsigned int i;
            //Try to write data in chunks, stop at every \n to replace with \r\n
            for(i=0;i<cnt;i++)
            {
                if(buffer[i]=='\n')
                {
                    if(chunkstart!=&buffer[i])
                       miosix::Console::write(chunkstart,&buffer[i]-chunkstart);
                    miosix::Console::write("\r\n",2);//Add \r\n
                    chunkstart=&buffer[i]+1;
                }
            }
            if(chunkstart!=&buffer[i])
                miosix::Console::write(chunkstart,&buffer[i]-chunkstart);
            return cnt;
        }
        case STDIN_FILENO:
            miosix::errorLog("\r\n***Attempting to write to stdin\r\n");
            return -1;
        default:
            //Writing to files
            #ifdef WITH_FILESYSTEM
            return miosix::Filesystem::instance().writeFile(fd,buf,cnt);
            #else //WITH_FILESYSTEM
            miosix::errorLog(
                    "\r\n***Kernel compiled without filesystem support\r\n");
            return -1;
            #endif //WITH_FILESYSTEM   
    }
}

/**
 * \internal
 * _write_r, write data to files, or the standard output/ standard error
 */
int _write_r(struct _reent *ptr, int fd, const void *buf, size_t cnt)
{
    return _write(fd,buf,cnt);
}

/**
 * \internal
 * _read (for C++ library)
 */
int _read(int fd, void *buf, size_t cnt)
{
    switch(fd)
    {
        case STDIN_FILENO:
        {
            //iprintf("read: buf %d\r\n",(int)cnt);//To check read buffer size
            char *buffer=(char*)buf;
            //Trying to be compatible with terminals that output \r, \n or \r\n
            //When receiving \r skip_terminate is set to true so we skip the \n
            //if it comes right after the \r
            static bool skip_terminate=false;
            int i;
            for(i=0;i<(int)cnt;i++)
            {
                buffer[i]=miosix::Console::readChar();
                switch(buffer[i])
                {
                    case '\r':
                        buffer[i]='\n';
                        miosix::Console::write("\r\n");//Echo
                        skip_terminate=true;
                        return i+1;
                        break;    
                    case '\n':
                        if(skip_terminate)
                        {
                            skip_terminate=false;
                            //Discard the \n because comes right after \r
                            //Note that i may become -1, but it is acceptable.
                            i--;
                        } else {
                            miosix::Console::write("\r\n");//Echo
                            return i+1;
                        }
                        break;
                    default:
                        skip_terminate=false;
                        //Echo
                        miosix::Console::write((const char *)&buffer[i],1);
                }
            }
            return cnt;
        }
        case STDOUT_FILENO:
        case STDERR_FILENO:
            miosix::errorLog("\r\n***Attempting to read from stdout\r\n");
            return -1;
        default:
            //Reading from files
            #ifdef WITH_FILESYSTEM
            return miosix::Filesystem::instance().readFile(fd,buf,cnt);
            #else //WITH_FILESYSTEM
            miosix::errorLog(
                    "\r\n***Kernel compiled without filesystem support\r\n");
            return -1;
            #endif //WITH_FILESYSTEM
    }
}

/**
 * \internal
 * _read_r, read data from files or the standard input
 */
int _read_r(struct _reent *ptr, int fd, void *buf, size_t cnt)
{
    return _read(fd,buf,cnt);
}

/**
 * \internal
 * _lseek (for C++ library)
 */
off_t _lseek(int fd, off_t pos, int whence)
{
    #ifdef WITH_FILESYSTEM
    return miosix::Filesystem::instance().lseekFile(fd,pos,whence);
    #else //WITH_FILESYSTEM
    miosix::errorLog("\r\n***Kernel compiled without filesystem support\r\n");
    return -1;
    #endif //WITH_FILESYSTEM
}

/**
 * \internal
 * _lseek_r, seek trough a file
 */
off_t _lseek_r(struct _reent *ptr, int fd, off_t pos, int whence)
{
    return _lseek(fd,pos,whence);
}

/**
 * \internal
 * _fstat (for C++ library)
 */
int _fstat(int fd, struct stat *pstat)
{
    if(fd<0) return -1;
    if(fd<3)
    {
        memset(pstat,0,sizeof(struct stat));
        pstat->st_mode=S_IFCHR;//Character device
        pstat->st_blksize=1024;
        return 0;
    } else {
        #ifdef WITH_FILESYSTEM
        return miosix::Filesystem::instance().fstatFile(fd,pstat);
        #else //WITH_FILESYSTEM
        return -1;
        #endif //WITH_FILESYSTEM
    }
}

/**
 * \internal
 * _fstat_r, collect data about a file
 */
int _fstat_r(struct _reent *ptr, int fd, struct stat *pstat)
{
    return _fstat(fd,pstat);
}

/**
 * \internal
 * _stat_r, collect data about a file
 */
int _stat_r(struct _reent *ptr, const char *file, struct stat *pstat)
{
    #ifdef WITH_FILESYSTEM
    return miosix::Filesystem::instance().statFile(file,pstat);
    #else //WITH_FILESYSTEM
    return -1;
    #endif //WITH_FILESYSTEM
}

/**
 * \internal
 * isatty, returns 1 if fd is associated with a terminal
 */
int isatty(int fd)
{
    switch(fd)
    {
        case STDIN_FILENO:
        case STDOUT_FILENO:
        case STDERR_FILENO:
            return 1;
        default:
            return 0;
    }
}

/**
 * \internal
 * _isatty, required by codesourcery gcc
 */
int _isatty(int fd)
{
    return isatty(fd);
}

/**
 * \internal
 * mkdir, create a directory
 */
int mkdir(const char *path, mode_t mode)
{
    #ifdef WITH_FILESYSTEM
    return miosix::Filesystem::instance().mkdir(path,mode);
    #else //WITH_FILESYSTEM
    return -1;
    #endif //WITH_FILESYSTEM
}

/**
 * \internal
 * _unlink_r, remove a file
 */
int _unlink_r(struct _reent *ptr, const char *file)
{
    #ifdef WITH_FILESYSTEM
    return miosix::Filesystem::instance().unlink(file);
    #else //WITH_FILESYSTEM
    return -1;
    #endif //WITH_FILESYSTEM
}

/**
 * \internal
 * _link_r, used by the C standard library function rename().
 * rename(old,new) calls link(old,new) and if link returns 0 it calls
 * unlink(old)
 * FIXME: implement me
 */
int _link_r(struct _reent *ptr, const char *f_old, const char *f_new)
{
    return -1;
}

/**
 * _kill, it looks like abort() calls _kill instead of exit, this implementation
 * calls _exit() so that calling abort() really terminates the program
 */
int _kill(int pid, int sig)
{
    if(pid==1) _exit(1); //pid=1 means the only running process
    else return -1;
}

/**
 * \internal
 * it looks like abort() calls _kill instead of exit, this implementation
 * calls _exit() so that calling abort() really terminates the program
 */
int _kill_r(struct _reent* ptr, int pid, int sig)
{
    return _kill(pid,sig);
}

/**
 * \internal
 * _getpid, there is only one process in Miosix (but multiple threads)
 */
int _getpid()
{
    return 1;
}

/**
 * \internal
 * _getpid_r, there is only one process in Miosix (but multiple threads)
 */
int _getpid_r(struct _reent* ptr)
{
    return _getpid();
}

/**
 * \internal
 * _fork_r, unimplemented because processes are not supported in Miosix
 */
int _fork_r(struct _reent *ptr)
{
    return -1;
}

/**
 * \internal
 * _wait_r, unimpemented because processes are not supported in Miosix
 */
int _wait_r(struct _reent *ptr, int *status)
{
    return -1;
}

#ifdef __cplusplus
}
#endif



//
// Check that newlib has been configured correctly
// ===============================================

#ifndef _WANT_REENT_SMALL
#warning "_WANT_REENT_SMALL not defined"
#endif //_WANT_REENT_SMALL

#ifndef _REENT_SMALL
#error "_REENT_SMALL not defined"
#endif //_REENT_SMALL

#ifndef _POSIX_THREADS
#error "_POSIX_THREADS not defined"
#endif //_POSIX_THREADS
