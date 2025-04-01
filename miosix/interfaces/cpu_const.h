/***************************************************************************
 *   Copyright (C) 2024 by Terraneo Federico                               *
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
 * Number of cores (must not be defined when WITH_SMP is not defined)
 * const unsigned char CPU_NUM_CORES=...;
 *
 * Which core is given the task to handle the wakeup from the list of sleeping
 * threads (must not be defined when WITH_SMP is not defined)
 * const unsigned char WAKEUP_HANDLING_CORE=...;
 */

namespace miosix {

#ifndef WITH_SMP
const unsigned char CPU_NUM_CORES=1;

const unsigned char WAKEUP_HANDLING_CORE=0;
#endif

/**
 * Returns a numeric ID for the current core. This function shall:
 * - Always return 0 on single core platforms
 * - Always return 0 when executed on the core which handles the boot process
 * - Never return a number greater or equal than CPU_NUM_CORES
 */
inline unsigned char getCurrentCoreId();
#ifndef WITH_SMP
inline unsigned char getCurrentCoreId() { return 0; }
#endif

} // namespace miosix

/**
 * \}
 */

#include "interfaces-impl/cpu_const_impl.h"

#ifdef WITH_SMP
#include "interfaces-impl/cpu_const_smp_impl.h"
#endif
