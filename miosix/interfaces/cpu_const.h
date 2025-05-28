/***************************************************************************
 *   Copyright (C) 2024-2025 by Terraneo Federico                          *
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

#include "config/miosix_settings.h"

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file cpu_const.h
 * The implementation of this file should contain the following constants
 * describing CPU properties:
 *
 * Size in words of vector to store CPU context during context switch.
 * const unsigned char CTXSAVE_SIZE=...;
 *
 * Size of additional context saved on the stack during context switch.
 * If zero, this architecture does not save anything on stack during context
 * save. Size is in bytes, not words. MUST be divisible by 4. This constant is
 * used to increase the stack size by the size of context save frame.
 * const unsigned int CTXSAVE_ON_STACK=...;
 *
 * Stack alignment required by the CPU
 * const unsigned int CTXSAVE_STACK_ALIGNMENT=...;
 *
 * Offset in words to retrieve the thread stack pointer in ctxsave
 * const unsigned int STACK_OFFSET_IN_CTXSAVE=...;
 * 
 * Number of cores (only if WITH_SMP is defined, otherwise defaults to 1)
 * const unsigned char CPU_NUM_CORES=...;
 *
 * The type used to store a set of CPUs for scheduler affinity (only if WITH_SMP
 * is defined, otherwise defaults to unsigned char)
 * using CpuSet=...;
 */

namespace miosix {

/**
 * Returns a numeric ID for the current core. This function shall:
 * - Always return 0 on single core platforms
 * - Always return 0 when executed on the core which handles the boot process
 * - Never return a number greater or equal than CPU_NUM_CORES
 *
 * \warning Although this function is safe to be called without taking any lock,
 * using the result may not be, as without taking either the GlobalIrqLock or
 * the PauseKernelLock, the thread calling this function may be preempted and
 * migrated to another core at any time.
 * Consider this code snippet executed without taking any lock, that did cause
 * trouble during Miosix 3 development:
 * \code
 * Thread *currentThread=runningThreads[getCurrentCoreId()];
 * \endcode
 * Most of the time it works, however, if the thread is preempted and migrated
 * after the getCurrentCoreId() but before the array indexing, currentThread
 * points to another thread running on another core!
 *
 * For this reason, any code calling getCurrentCoreId() without taking the
 * GlobalIrqLock or PauseKernelLock should be carefully checked not to cause
 * race conditions on thread migrations.
 */
inline unsigned char getCurrentCoreId();

#ifndef WITH_SMP
const unsigned char CPU_NUM_CORES=1; //Only one core

using CpuSet=unsigned char;

inline unsigned char getCurrentCoreId() { return 0; }
#endif

} // namespace miosix

#include "interfaces-impl/cpu_const_impl.h"

#ifdef WITH_SMP
#include "interfaces-impl/cpu_const_smp_impl.h"
#endif

namespace miosix {

/**
 * Selects which core is given the task to handle the wakeup from the list of
 * sleeping threads. We pick the last core for heuristic load balancing:
 * Core 0 already handles most peripheral interrupts, so we choose another one,
 * unless we are on a single core architecture, in which case it will be core 0.
 */
const unsigned char WAKEUP_HANDLING_CORE=CPU_NUM_CORES-1;

/**
 * Constant representing an affinity mask posing no restriction on scheduling
 * a thread on any core
 */
constexpr CpuSet unrestrictedAffinityMask=(1<<CPU_NUM_CORES)-1;

} // namespace miosix

/**
 * \}
 */
