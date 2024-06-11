/***************************************************************************
 *   Copyright (C) 2012-2024 by Terraneo Federico                          *
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
#include <sys/time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <reent.h>
#include <cxxabi.h>

constexpr int numAtexitEntries=2; ///< Number of entries per AtexitBlock

/**
 * Struct to store global destructors
 */
struct AtexitBlock
{
    void (*fns[numAtexitEntries])(void*); ///< Destructor, nullptr if empty
    void *args[numAtexitEntries];         ///< Destructor arg, if any
    AtexitBlock *next;                    ///< Next block, nullptr if last
};

static AtexitBlock head = {}; ///< Head of the AtexitBlock list

extern "C" {

/**
 * \internal
 * This function is called from crt0.s when syscalls returning a 32 bit int fail.
 * It should set errno to the corresponding error code
 * \param ec the failed syscall error code
 * \return -1, the syscall return value upon failure
 */
int __seterrno32(int ec)
{
    errno=-ec;
    return -1;
}

/**
 * \internal
 * This function is called from crt0.s when syscalls returning a 64 bit int fail.
 * It should set errno to the corresponding error code
 * \param ec the failed syscall error code
 * \return -1, the syscall return value upon failure
 */
long long __seterrno64(long long ec)
{
    errno=-ec;
    return -1;
}

/**
 * \internal
 * getcwd returns nullptr on failure instead of -1, so __seterrno32 can't be used
 */
char *__getcwdfailed(int ec)
{
    errno=-ec;
    return nullptr;
}

/**
 * \internal
 * isatty is weird, errors do not return -1 but 0. The case in which the file
 * descriptor is valid but is not a TTY can be distinguished from the error
 * case by looking at errno.
 */
int __isattyfailed(int ec)
{
    if(ec==0) errno=ENOTTY;
    else errno=-ec;
    return 0;
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
int __register_exitproc(int type, void (*fn)(void*), void *arg, void *d)
{
    //iprintf("__register_exitproc(%d,%p,%p,%p)\n",type,fn,arg,d);
    AtexitBlock *walk=&head;
    int index=0;
    while(walk->fns[index])
    {
        if(++index<numAtexitEntries) continue;
        index=0;
        void *ptr=calloc(1,sizeof(AtexitBlock));
        if(ptr==nullptr) return 1;
        walk->next=(AtexitBlock *)ptr;
        break;
    }
    walk->fns[index]=fn;
    walk->args[index]=arg;
    return 0;
}

/**
 * Called by exit() to call functions registered through atexit()
 * \param code the exit code, for example with exit(1), code==1
 * \param d __dso_handle, see __register_exitproc
 */
void __call_exitprocs(int code, void *d)
{
    //NOTE: we are not deallocating the linked list not to pull in free if the
    //program never calls __register_exitproc, and also because the process is
    //going to terminate anyway
    int index=0;
    for(AtexitBlock *walk=&head;walk && walk->fns[index];)
    {
        walk->fns[index](walk->args[index]);
        if(++index<numAtexitEntries) continue;
        index=0;
        walk=walk->next;
    }
}

/**
 * \internal
 * Required by C++ standard library.
 * See http://lists.debian.org/debian-gcc/2003/07/msg00057.html
 */
void *__dso_handle=(void*) &__dso_handle;

// initialized in crt0.s, used also by memoryprofiling
const char *__processHeapEnd;
const char *__processStackEnd;
// used by memoryprofiling
const char *__maxUsedHeap=nullptr;

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
    if(curHeapEnd+incr>__processHeapEnd)
    {
        //bad, heap overflow
        #ifdef __NO_EXCEPTIONS
        // When exceptions are disabled operator new would return nullptr, which
        // would cause undefined behaviour. So when exceptions are disabled,
        // a heap overflow terminates the process.
        write(STDERR_FILENO,"Heap overflow\n",14);
        _exit(1);
        #else //__NO_EXCEPTIONS
        return reinterpret_cast<void*>(-1);
        #endif //__NO_EXCEPTIONS
    }
    curHeapEnd+=incr;
    if(curHeapEnd>__maxUsedHeap) __maxUsedHeap=curHeapEnd;
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






int _open_r(struct _reent *ptr, const char *name, int flags, int mode)
{
    return open(name,flags,mode);
}

int _close_r(struct _reent *ptr, int fd)
{
    return close(fd);
}

ssize_t _read_r(struct _reent *ptr, int fd, void *buf, size_t size)
{
    return read(fd,buf,size);
}

ssize_t _write_r(struct _reent *ptr, int fd, const void *buf, size_t size)
{
    return write(fd,buf,size);
}

off_t _lseek_r(struct _reent *ptr, int fd, off_t pos, int whence)
{
    return lseek(fd,pos,whence);
}

int _stat_r(struct _reent *ptr, const char *file, struct stat *pstat)
{
    return stat(file,pstat);
}

// int _lstat_r(struct _reent *ptr, const char *file, struct stat *pstat)
// {
//     return lstat(file,pstat);
// }

int _fstat_r(struct _reent *ptr, int fd, struct stat *pstat)
{
    return fstat(fd,pstat);
}

int _fcntl_r(struct _reent *ptr, int fd, int cmd, int opt)
{
    return fcntl(fd,cmd,opt);
}

int _ioctl_r(struct _reent *ptr, int fd, int cmd, void *arg)
{
    return ioctl(fd,cmd,arg);
}

int _isatty_r(struct _reent *ptr, int fd)
{
    return isatty(fd);
}

char *_getcwd_r(struct _reent *ptr, char *buf, size_t size)
{
    return getcwd(buf,size);
}

int _chdir_r(struct _reent *ptr, const char *path)
{
    return chdir(path);
}

// int _getdents_r(struct _reent *ptr, int fd, struct dirent *buf, unsigned int size)
// {
//     return getdents(fd,buf,size);
// }

int _mkdir_r(struct _reent *ptr, const char *path, int mode)
{
    return mkdir(path,mode);
}

int _rmdir_r(struct _reent *ptr, const char *path)
{
    return rmdir(path);
}

int _link_r(struct _reent *ptr, const char *f_old, const char *f_new)
{
    return link(f_old,f_new);
}

int _unlink_r(struct _reent *ptr, const char *file)
{
    return unlink(file);
}

int _symlink_r(struct _reent *ptr, const char *target, const char *linkpath)
{
    return symlink(target,linkpath);
}

ssize_t _readlink_r(struct _reent *ptr, const char *path, char *buf, size_t size)
{
    return readlink(path,buf,size);
}

int _rename_r(struct _reent *ptr, const char *f_old, const char *f_new)
{
    return rename(f_old,f_new);
}

clock_t _times_r(struct _reent *ptr, struct tms *tim)
{
    return times(tim);
}

int _gettimeofday_r(struct _reent *ptr, struct timeval *tv, void *tz)
{
    return gettimeofday(tv,tz);
}

// int _kill_r(struct _reent* ptr, int pid, int sig) { return -1; }

int _wait_r(struct _reent *ptr, int *status)
{
    return waitpid(-1,status,0);
}

int _getpid_r(struct _reent* ptr)
{
    return getpid();
}

clock_t times(struct tms *tim)
{
    struct timespec tp;
    //No CLOCK_PROCESS_CPUTIME_ID support, use CLOCK_MONOTONIC
    if(clock_gettime(CLOCK_MONOTONIC,&tp)) return static_cast<clock_t>(-1);
    constexpr int divFactor=1000000000/CLOCKS_PER_SEC;
    clock_t utime=tp.tv_sec*CLOCKS_PER_SEC + tp.tv_nsec/divFactor;

    //Unfortunately, the behavior of _times_r is poorly specified and ambiguous.
    //The return value is either tim.utime or -1 on failure, but clock_t is
    //unsigned. If someone calls _times_r in an unlucky moment where tim.utime
    //is 0xffffffff it could be interpreted as the -1 error code even if there
    //is no error.
    //This is not as unlikely as it seems because CLOCKS_PER_SEC is a relatively
    //huge number (100 for Miosix's implementation).
    //To solve the ambiguity Miosix never returns 0xffffffff except in case of
    //error. If tim.utime happens to be 0xffffffff, _times_r returns 0 instead.
    //We also implement the Linux extension where tim can be NULL.
    if(tim!=nullptr)
    {
        tim->tms_utime=utime;
        tim->tms_stime=0;
        tim->tms_cutime=0;
        tim->tms_cstime=0;
    }
    return utime==static_cast<clock_t>(-1)?0:utime;
}

int gettimeofday(struct timeval *tv, void *tz)
{
    if(tv==nullptr || tz!=nullptr) return -1;
    struct timespec tp;
    if(clock_gettime(CLOCK_REALTIME,&tp)) return -1;
    tv->tv_sec=tp.tv_sec;
    tv->tv_usec=tp.tv_nsec/1000;
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    return clock_nanosleep(CLOCK_MONOTONIC,0,req,rem);
}

pid_t wait(int *status)
{
    return waitpid(-1,status,0);
}

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
