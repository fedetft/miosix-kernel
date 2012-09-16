/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012 by Terraneo Federico       *
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

#ifndef MIOSIX_SETTINGS_H
#define MIOSIX_SETTINGS_H

/**
 * \file miosix_settings.h
 * NOTE: this file contains ONLY configuration options that are not dependent
 * on architecture specific details. The other options are in the following
 * files which are included here:
 * miosix/arch/architecture name/common/arch_settings.h
 * miosix/arch/architecture name/board name/board_settings.h
 */
#include "arch_settings.h"
#include "board_settings.h"

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

//
// Filesystem options
//

/// \def WITH_FILESYSTEM
/// Allows to enable/disable filesystem support.
/// By default it is defined (filesystem support is enabled)
//#define WITH_FILESYSTEM
    
/// \def SYNC_AFTER_WRITE
/// Increases filesystem write robustness. After each write operation the
/// filesystem is synced so that a power failure happens data is not lost
/// (unless power failure happens exactly between the write and the sync)
/// Unfortunately write latency and throughput becomes twice as worse
/// By default it is defined (slow but safe)
#define SYNC_AFTER_WRITE

/// Maximum number of open files. Trying to open more will fail.
const unsigned char MAX_OPEN_FILES=8;

//
// C/C++ standard library I/O (stdin, stdout and stderr related)
//

/// \def WITH_STDIN_BUFFER
/// If defined, incoming data fron the serial port
/// will be buffered prior to being delivered to stdin related functions
/// (scanf, fgets, cin...). Buffer is flushed on newline or when full.
/// This has a behaviour more similar to desktop operating systems but
/// requires 1KB of RAM for the buffer.
/// Despite counterintuitive, if you never use stdin related functions
/// defining WITH_STDIN_BUFFER will reduce code size.
/// By default it is defined (stdin is buffered)
#define WITH_STDIN_BUFFER

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

/**
 * \def JTAG_DISABLE_SLEEP
 * JTAG debuggers lose communication with the device if it enters sleep
 * mode, so to use debugging it is necessary to disble sleep in the idle thread.
 * By default it is not defined (idle thread calls sleep).
 */
//#define JTAG_DISABLE_SLEEP

/// Minimum stack size (MUST be divisible by 4)
const unsigned int STACK_MIN=256;

/// \internal Size of idle thread stack.
/// Should be >=STACK_MIN (MUST be divisible by 4)
const unsigned int STACK_IDLE=256;

/// Default stack size for pthread_create.
/// The chosen value is enough to call C standard library functions
/// such as printf/fopen which are stack-heavy
const unsigned int STACK_DEFAULT_FOR_PTHREAD=2048;

/// Number of priorities (MUST be >1)
/// PRIORITY_MAX-1 is the highest priority, 0 is the lowest. -1 is reserved as
/// the priority of the idle thread.
/// The meaning of a thread's priority depends on the chosen scheduler.
#ifdef SCHED_TYPE_PRIORITY
//Can be modified, but a high value makes context switches more expensive
const short int PRIORITY_MAX=4;
#elif defined(SCHED_TYPE_CONTROL_BASED)
//Don't touch, the limit is due to the fixed point implementation
//It's not needed for if floating point is selected, but kept for consistency
const short int PRIORITY_MAX=64;
#else //SCHED_TYPE_EDF
//Doesn't exist for this kind of scheduler
#endif

/// Priority of main()
/// The meaning of a thread's priority depends on the chosen scheduler.
const unsigned char MAIN_PRIORITY=1;



//
// Other low level kernel options. There is usually no need to modify these.
//

/// \internal Length of wartermark (in bytes) to check stack overflow.
/// MUST be divisible by 4 and can also be zero.
/// A high value increases context switch time.
const unsigned char WATERMARK_LEN=16;

/// \internal Used to fill watermark
const unsigned int WATERMARK_FILL=0xaaaaaaaa;

/// \internal Used to fill stack (for checking stack usage)
const unsigned int STACK_FILL=0xbbbbbbbb;

/**
 * \}
 */

} //namespace miosix

#endif //MIOSIX_SETTINGS_H
