/***************************************************************************
 *   Copyright (C) 2008-2021 by Terraneo Federico                          *
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

#pragma once

/**
 * \file miosix_settings.h
 * NOTE: this file contains ONLY configuration options that are not dependent
 * on architecture specific details. The other options are in the following
 * files which are included here:
 * miosix/config/arch/architecture name/board name/board_settings.h
 */
#include "board_settings.h"
#include <limits>

/**
 * \internal
 * Versioning for miosix_settings.h for out of git tree projects
 */
#define MIOSIX_SETTINGS_VERSION 300

namespace miosix {

/**
 * \addtogroup Settings
 * \{
 */

//
// Scheduler options
//

/// \def SCHED_TYPE_PRIORITY
/// If uncommented selects the priority scheduler
/// \def SCHED_TYPE_CONTROL_BASED
/// If uncommented selects the control based scheduler
/// \def SCHED_TYPE_EDF
///If uncommented selects the EDF scheduler
//Uncomment only *one* of those

#define SCHED_TYPE_PRIORITY
//#define SCHED_TYPE_CONTROL_BASED
//#define SCHED_TYPE_EDF

/// \def WITH_THREAD_AFFINITY
/// Enable support for setting the core affinity of threads in the scheduler
//#define WITH_THREAD_AFFINITY

/// \def WITH_CPU_TIME_COUNTER
/// Allows to enable/disable CPUTimeCounter to save code size and remove its
/// overhead from the scheduling process. By default it is not defined
/// (CPUTimeCounter is disabled).
//#define WITH_CPU_TIME_COUNTER

//
// Filesystem options
//

/// \def WITH_FILESYSTEM
/// Allows to enable/disable filesystem support to save code size
/// By default it is defined (filesystem support is enabled)
#define WITH_FILESYSTEM

// The following options make sense only when filesystem is enabled, so they are
// always left undefined otherwise
#ifdef WITH_FILESYSTEM

/// \def WITH_DEVFS
/// Allows to enable/disable DevFs support to save code size
/// By default it is defined (DevFs is enabled)
#define WITH_DEVFS

/// \def WITH_FATFS
/// Allows to enable/disable FATFS support to save code size
/// By default it is defined (FATFS is enabled)
#define WITH_FATFS
/// Maxium number of files that can be opened on a mounted FATFS partition.
/// Must be greater than 0
constexpr unsigned char FATFS_MAX_OPEN_FILES=8;
/// The truncate/ftruncate operations, and seeking past the end of the file are
/// two patterns for zero-filling a file. This requires a buffer to be done
/// efficiently, and the size of the buffer impacts performance. To save RAM the
/// suggested value is 512 byte, for performance 4096 or even 16384 are better.
/// Note that no buffer is allocated unless required, the buffer is deallocated
/// afterwards, and the worst case memory required is one buffer per mounted
/// FATFS partition if one concurrent truncate/write past the end per partition
/// occurs.
constexpr unsigned int FATFS_EXTEND_BUFFER=512;

/// \def WITH_LITTLEFS
/// Allows to enable/disable LittleFS support to save code size
/// By default it is not defined (LittleFS is disabled)
//#define WITH_LITTLEFS

/// \def WITH_ROMFS
/// Allows to enable/disable RomFS support to save code size
/// By default it is not defined (RomFS is disabled)
//#define WITH_ROMFS

/// \def SYNC_AFTER_WRITE
/// Increases filesystem write robustness. After each write operation the
/// filesystem is synced so that a power failure happens data is not lost
/// (unless power failure happens exactly between the write and the sync)
/// Unfortunately write latency and throughput becomes twice as worse
/// By default it is defined (slow but safe)
//#define SYNC_AFTER_WRITE

/// Maximum number of files a single process (or the kernel) can open. This
/// constant is used to size file descriptor tables. Individual filesystems can
/// introduce futher limitations. Cannot be less than 3, as the first three are
/// stdin, stdout, stderr, and in this case no additional files can be opened.
const unsigned char MAX_OPEN_FILES=8;

/// \def WITH_PROCESSES
/// If uncommented enables support for processes as well as threads.
/// This enables the dynamic loader to load elf programs, the extended system
/// call service and, if the hardware supports it, the MPU to provide memory
/// isolation of processes
//#define WITH_PROCESSES

/// RomFS is enabled by default when using processes. Comment the following
/// lines if you want to use processes without RomFS.
#if defined(WITH_PROCESSES) && !defined(WITH_ROMFS)
#define WITH_ROMFS
#endif

#endif // WITH_FILESYSTEM

//
// C/C++ standard library I/O (stdin, stdout and stderr related)
//

/// \def WITH_BOOTLOG
/// Uncomment to print bootlogs on stdout.
/// By default it is defined (bootlogs are printed)
#define WITH_BOOTLOG

/// \def WITH_ERRLOG
/// Uncomment for debug information on stdout.
/// By default it is defined (error information is printed)
#define WITH_ERRLOG



//
// Kernel related options (stack sizes, priorities)
//

/// Enable additional assertions useful for debugging but adding more run-time
/// overhead. Three levels are implemented
/// None: Do not add extra checks, leading to fastest code. This is the default.
/// Application: Only add checks that are useful to debug kernel-level
/// application programming errors. Useful during application code debugging.
/// Kernel: Also add checks that are useful to debug kernel code. Useful
/// during kernel code debugging. Adds significant run-time overhead!
enum class ExtraChecks { None, Application, Kernel };
constexpr auto extraChecks=ExtraChecks::None;

/// \def WITH_SLEEP
/// Enable sleep support. If enabled, the idle thread will stop the CPU whenever
/// no ready thread exists to save power.
/// In general, you should keep this option enabled, the only reason to disable
/// this option is that on some architectures debuggers lose communication with
/// the device if it enters sleep mode, so to use debugging it is necessary to
/// disable sleep support. For this reason, the option used to be called JTAG_DISABLE_SLEEP
#define WITH_SLEEP

/// \def WITH_DEEP_SLEEP 
/// Adds interfaces and required variables to support entering deep sleep and
/// thus turning off also peripherals when possible. Saves much more energy but
/// requires device drivers to support this option.
//#define WITH_DEEP_SLEEP

#if defined(WITH_DEEP_SLEEP) && !defined(WITH_SLEEP)
#error Deep sleep requires sleep support
#endif //defined(WITH_DEEP_SLEEP) && !defined(WITH_SLEEP)

/// Minimum stack size (MUST be divisible by 4)
const unsigned int STACK_MIN=256;

/// \internal Size of idle thread stack.
/// Should be >=STACK_MIN (MUST be divisible by 4)
const unsigned int STACK_IDLE=256;

/// Default stack size for pthread_create.
/// The chosen value is enough to call C standard library functions
/// such as printf/fopen which are stack-heavy (MUST be divisible by 4)
const unsigned int STACK_DEFAULT_FOR_PTHREAD=2048;

/// Maximum size of the RAM image of a process. If a program requires more
/// the kernel will not run it (MUST be divisible by 4)
const unsigned int MAX_PROCESS_IMAGE_SIZE=64*1024;

/// Minimum size of the stack for a process. If a program specifies a lower
/// size the kernel will not run it (MUST be divisible by 4)
const unsigned int MIN_PROCESS_STACK_SIZE=1024;

/// Every userspace thread has two stacks, one for when it is running in
/// userspace and one for when it is running in kernelspace (that is, while it
/// is executing system calls). This is the size of the stack for when the
/// thread is running in kernelspace (MUST be divisible by 4)
const unsigned int SYSTEM_MODE_PROCESS_STACK_SIZE=2048;

/// Maximum number of arguments passed through argv to a process
/// Also maximum number of environment variables passed through envp to a process
const unsigned int MAX_PROCESS_ARGS=16;

/// Maximum size of the memory area at the top of the stack for arguments and
/// environment variables. This area is not considered part of the stack and
/// does not contribute to the stack size.
const unsigned int MAX_PROCESS_ARGS_BLOCK_SIZE=512;

static_assert(STACK_IDLE>=STACK_MIN,"");
static_assert(STACK_DEFAULT_FOR_PTHREAD>=STACK_MIN,"");
static_assert(MIN_PROCESS_STACK_SIZE>=STACK_MIN,"");
static_assert(SYSTEM_MODE_PROCESS_STACK_SIZE>=STACK_MIN,"");

// The meaning of a thread's priority depends on the chosen scheduler.
#ifdef SCHED_TYPE_PRIORITY
/// The constant NUM_PRIORITIES defines the number of priorities the scheduler
/// can handle. Value must be at least 1 and up to 128 since the priority value
/// is stored in a char in the priority scheduler. A higher value makes context
/// switches more expensive and increases the memory required by the scheduler
/// and synchronization primitives (Mutex and ConditionVariable).
/// Can be set to 1 for non-real-time applications, in which case the scheduler
/// becomes a pure round robin without priorities and code size and memory
/// occupation is minimized.
/// NUM_PRIORITIES-1 is the highest priority, 0 is the lowest. -1 is reserved as
/// the priority of the idle thread.
#define NUM_PRIORITIES 4
/// Default priority. Priority of main() and threads created with PTHREAD/C++11
const signed char DEFAULT_PRIORITY=1;
static_assert(NUM_PRIORITIES>=0 && NUM_PRIORITIES<=128,"");
static_assert(DEFAULT_PRIORITY>=0 && DEFAULT_PRIORITY<NUM_PRIORITIES,"");
#elif defined(SCHED_TYPE_CONTROL_BASED)
/// The constant NUM_PRIORITIES defines the number of priorities (MUST be >=1)
/// NUM_PRIORITIES-1 is the highest priority, 0 is the lowest. -1 is reserved as
/// the priority of the idle thread.
/// Don't change this value, the limit is due to the fixed point implementation
/// It's not needed for if floating point is selected, but kept for consistency
const short int NUM_PRIORITIES=64;
/// Default priority. Priority of main() and threads created with PTHREAD/C++11
const short int DEFAULT_PRIORITY=1;
static_assert(DEFAULT_PRIORITY>=0 && DEFAULT_PRIORITY<NUM_PRIORITIES,"");
#else //SCHED_TYPE_EDF
/// The EDF scheduler redefines priorities as the thread absolute deadline.
/// Additionally, the constant DEFAULT_PRIORITY is the default priority value
/// for main() and threads created with PTHREAD/C++11. This priority level
/// corrsponds to non real-time threads, which are scheduled using round-robin.
/// A thread can, at any moment, set its own priority to an absolute deadline
/// time and in this case it is scheduled using EDF. From there, it can either
/// keep setting a new absolute deadline in the future, or return to the non
/// real-time level by setting its priority to DEFAULT_PRIORITY to be scheduled
/// again using round robin. Ready tasks with a deadline will always preempt non
/// real-time threads whose priority is DEFAULT_PRIORITY
const long long DEFAULT_PRIORITY=std::numeric_limits<long long>::max()-2;
#endif

#if defined(SCHED_TYPE_PRIORITY) || defined(SCHED_TYPE_EDF)
/// Maximum thread time slice in nanoseconds, after which preemption occurs
const unsigned int MAX_TIME_SLICE=1000000;
#endif //SCHED_TYPE_PRIORITY

/// \def OS_TIMER_MODEL_UNIFIED
/// If this configuration option is selected, the kernel uses a single timer
/// for both preemption and timekeeping (getTime() and Thread::nanoSleep()).
/// If this option is not selected, the kernel uses one timer for timekeeping
/// for the entire OS plus a separate timer for each CPU core for context
/// switches. In single-core microcontrollers, two timers are needed.
//#define OS_TIMER_MODEL_UNIFIED

/// Select the implementation of pthread_mutex_t. Three options:
/// 1) pthreadMutexProtocolOverride=DYNAMIC;
/// Allow the type of each pthread_mutex_t to be selected on a per-mutex basis
/// with pthread_mutexattr_setprotocol(). This is the implementation POSIX
/// intended, but it has considerable drawbacks:
/// - POSIX does not allow to set the mutex protocol for statically allocated
///   mutexes, only mutexes created with pthread_mutex_init can have inheritance
/// - The C standard library and some other libraries such as OpenMP use
///   mutexes without setting priority inheritance, thus real-time code can
///   incur in priority inversion on those mutexes
/// - There is no way to alter the protocol of C++ std::mutex, although it is
///   implemented on top of pthread_mutex_t
/// - there is a performance penalty in checking the mutex type at every
///   lock/unlock
/// 2) pthreadMutexProtocolOverride=FORCE_PRIO_INHERIT;
/// All pthread_mutex_t, regardless of calls to pthread_mutexattr_setprotocol()
/// are forced to use the priority inheritance implementation. Allows to avoid
/// priority inversion also in the C standard library, using C++ mutexes and
/// OpenMP. Implementation is also slightly faster due to not having to check
/// the mutex type at every lock/unlock
/// 3) pthreadMutexProtocolOverride=FORCE_PRIO_NONE;
/// Backwards compatibility with the Miosix 2 behavior. All pthread_mutex_t do
/// not enforce priority inheritance. The kernel still provides a mutex type
/// with priority inheritance but it is the Miosix-specific claass Mutex.
/// This option provides the fastest pthread_mutex_t implementation.
enum class PthreadMutexProtocol { DYNAMIC, FORCE_PRIO_INHERIT, FORCE_PRIO_NONE };
constexpr auto pthreadMutexProtocolOverride=PthreadMutexProtocol::DYNAMIC;

/// Adding this definition changes the mutex types used by the kernel and device
/// drivers (KernelMutex) to the priority inheritance type. Prevents priority
/// inversion when accessing kernel code and device drivers at the cost of a
/// slightly higher overhead
//#define KERNEL_MUTEX_WITH_PRIORITY_INHERITANCE

/// pthread_exit() is a dangerous function. To understand why, let's first
/// discuss how Linux implements it. On the surface, it looks like it neatly
/// works with C++ as it is implemented by throwing a special ForcedUnwind
/// exception thus calling C++ destructors. However, implementers left a number
/// of unhandled corner cases all leading to unexpected program termination.
/// These are: calling pthread_exit() from code that directly or indirectly...
/// 1) contains a catch(...) that does not rethrow? std::abort() gets called
/// 2) contains a function declared noexcept? std::abort() gets called
/// 3) is called from a destructor? you guessed it, std::abort() again!
/// For this reason pthread_exit() support is by default disabled in Miosix and
/// attempting to call this function is met with a linking error. If you really
/// need it, uncomment the line below. Oh, right, how does Miosix implement it?
/// Throwing a standard C++ exception, so a catch(...) can stop a pthread_exit()
/// which is imho neither better nor worse than Linux.
/// Reference: https://udrepper.livejournal.com/21541.html
//#define WITH_PTHREAD_EXIT

/// Enable support for pthread_key_create/pthread_key_delete/pthread_getspecific
//#define WITH_PTHREAD_KEYS

/// Maximum number of concurrently existing pthread keys
const unsigned int MAX_PTHREAD_KEYS=2;


//
// Other low level kernel options. There is usually no need to modify these.
//

/// \internal Length of wartermark (in bytes) to check stack overflow.
/// MUST be divisible by 4 and can also be zero.
/// A high value increases context switch time.
const unsigned int WATERMARK_LEN=16;

/// \internal Used to fill watermark
const unsigned int WATERMARK_FILL=0xaaaaaaaa;

/// \internal Used to fill stack (for checking stack usage). Must be a single
/// byte value repeated 4 times to fill a word
const unsigned int STACK_FILL=0xbbbbbbbb;

// Compiler version checks
#if !defined(_MIOSIX_GCC_PATCH_MAJOR) || (_MIOSIX_GCC_PATCH_MAJOR < 4)
#error "You are using a too old or unsupported compiler. Get the latest one from https://miosix.org/wiki/index.php?title=Miosix_Toolchain"
#endif
#if _MIOSIX_GCC_PATCH_MAJOR > 4
#warning "You are using a too new compiler, which may not be supported"
#endif

/**
 * \}
 */

} //namespace miosix
