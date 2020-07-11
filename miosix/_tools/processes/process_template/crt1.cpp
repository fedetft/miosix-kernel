/***************************************************************************
 *   Copyright (C) 2012-2020 by Terraneo Federico                          *
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <reent.h>
#include <cxxabi.h>

extern "C" {

/**
 * \internal
 * This function is called from crt0.s when syscalls fail.
 * It should set errno to the corresponding error code
 * \param ec the failed syscall error code
 * \return -1, the syscall return value upon failure
 */
int __seterrno(int ec)
{
    errno=-ec;
    return -1;
}




//
// C atexit support, for thread safety and code size optimizations
// ===============================================================

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
    //FIXME: implement me
    return 0;
}

/**
 * Called by exit() to call functions registered through atexit()
 * \param code the exit code, for example with exit(1), code==1
 * \param d __dso_handle, see __register_exitproc
 */
void __call_exitprocs(int code, void *d)
{
    //FIXME: implement me
}

/**
 * \internal
 * Required by C++ standard library.
 * See http://lists.debian.org/debian-gcc/2003/07/msg00057.html
 */
void *__dso_handle=(void*) &__dso_handle;




/**
 * \internal
 * _sbrk_r, allocates memory dynamically
 */
void *_sbrk_r(struct _reent *ptr, ptrdiff_t incr)
{
    //This is the absolute start of the heap
    extern char _end asm("_end"); //defined in the linker script
    //This holds the current end of the heap (static)
    static char *curHeapEnd=&_end;
    //This holds the previous end of the heap
    char *prevHeapEnd;
    
    prevHeapEnd=curHeapEnd;
    curHeapEnd+=incr;
    return reinterpret_cast<void*>(prevHeapEnd);
}

/**
 * \internal
 * __malloc_lock, called by malloc to ensure memory allocation thread safety
 */
void __malloc_lock()
{
    // For now processes can't have threads, so do nothing
}

/**
 * \internal
 * __malloc_unlock, called by malloc after performing operations on the heap
 */
void __malloc_unlock()
{
    // For now processes can't have threads, so do nothing
}

/**
 * \internal
 * __getreent(), return the reentrancy structure of the current thread.
 * Used by newlib to make the C standard library thread safe
 */
struct _reent *__getreent()
{
    // For now processes can't have threads, so always return the global one
    return _GLOBAL_REENT;
}






int _open_r(struct _reent *ptr, const char *name, int flags, int mode) { return -1; }
int _close_r(struct _reent *ptr, int fd) { return -1; }

int _write(int fd, const void *buf, size_t cnt)
{
    return write(fd,buf,cnt);
}

int _write_r(struct _reent *ptr, int fd, const void *buf, size_t cnt)
{
    return write(fd,buf,cnt);
}

int _read(int fd, void *buf, size_t cnt)
{
    return read(fd,buf,cnt);
}

int _read_r(struct _reent *ptr, int fd, void *buf, size_t cnt)
{
    return read(fd,buf,cnt);
}

int _fstat(int fd, struct stat *pstat)
{
    switch(fd)
    {
        case STDIN_FILENO:
        case STDOUT_FILENO:
        case STDERR_FILENO:
            memset(pstat,0,sizeof(struct stat));
            pstat->st_mode=S_IFCHR;//Character device
            pstat->st_blksize=0; //Defualt file buffer equals to BUFSIZ
            return 0;
        default:
            return -1;
    }
}

int _fstat_r(struct _reent *ptr, int fd, struct stat *pstat)
{
    return _fstat(fd, pstat);
}

int isatty(int fd) { return 1; }
int _isatty(int fd) { return 1; }

off_t _lseek(int fd, off_t pos, int whence) { return -1; }
off_t _lseek_r(struct _reent *ptr, int fd, off_t pos, int whence) { return -1; }
int _stat_r(struct _reent *ptr, const char *file, struct stat *pstat) { return -1; }
int _isatty_r(struct _reent *ptr, int fd) { return -1; }
int mkdir(const char *path, mode_t mode) { return -1; }
int _unlink_r(struct _reent *ptr, const char *file) { return -1; }
clock_t _times_r(struct _reent *ptr, struct tms *tim) { return -1; }
int _link_r(struct _reent *ptr, const char *f_old, const char *f_new) { return -1; }
int _kill(int pid, int sig) { return -1; }
int _kill_r(struct _reent* ptr, int pid, int sig) { return -1; }
int _getpid() { return 1; }
int _getpid_r(struct _reent* ptr) { return 1; }
int _fork_r(struct _reent *ptr) { return -1; }
int _wait_r(struct _reent *ptr, int *status) { return -1; }

// TODO: implement when processes can spawn threads
int pthread_mutex_unlock(pthread_mutex_t *mutex)  { return 0; }
int pthread_mutex_lock(pthread_mutex_t *mutex)    { return 0; }
int pthread_mutex_destroy(pthread_mutex_t *mutex) { return 0; }
int pthread_setcancelstate(int state, int *oldstate) { return 0; }

int pthread_once(pthread_once_t *once, void (*func)())
{
    //TODO: make thread-safe when processes can spawn threads
    if(once==nullptr || func==nullptr || once->is_initialized!=1) return EINVAL;
    if(once->init_executed==0)
    {
        func();
        once->init_executed=2; //We succeeded
    }
    return 0;
}

} // extern "C"

union MiosixGuard
{
    //miosix::Thread *owner;
    unsigned int flag;
};

namespace __cxxabiv1
{

struct __cxa_exception; //A forward declaration of this one is enough

/*
 * This struct was taken from libsupc++/unwind-cxx.h Unfortunately that file
 * is not deployed in the gcc installation so we can't just #include it.
 * It is required on a per-thread basis to make C++ exceptions thread safe.
 */
struct __cxa_eh_globals
{
    __cxa_exception *caughtExceptions;
    unsigned int uncaughtExceptions;
    //Should be __ARM_EABI_UNWINDER__ but that's only usable inside gcc
    #ifdef __ARM_EABI__
    __cxa_exception* propagatingExceptions;
    #endif //__ARM_EABI__
};

static __cxa_eh_globals eh = { 0 };

extern "C" __cxa_eh_globals* __cxa_get_globals_fast()
{
    return &eh; //TODO: one per-thread when processes can spawn threads
}

extern "C" __cxa_eh_globals* __cxa_get_globals()
{
    return &eh; //TODO: one per-thread when processes can spawn threads
}

extern "C" int __cxa_guard_acquire(__guard *g)
{
    //TODO: make thread-safe when processes can spawn threads
    volatile MiosixGuard *guard=reinterpret_cast<volatile MiosixGuard*>(g);
    if(guard->flag==1) return 0; //Object already initialized, good
    return 1;
}

extern "C" void __cxa_guard_release(__guard *g) noexcept
{
    //TODO: make thread-safe when processes can spawn threads
    volatile MiosixGuard *guard=reinterpret_cast<volatile MiosixGuard*>(g);
    guard->flag=1;
}

extern "C" void __cxa_guard_abort(__guard *g) noexcept
{
    //TODO: make thread-safe when processes can spawn threads
    volatile MiosixGuard *guard=reinterpret_cast<volatile MiosixGuard*>(g);
    guard->flag=0;
}

} //namespace __cxxabiv1
