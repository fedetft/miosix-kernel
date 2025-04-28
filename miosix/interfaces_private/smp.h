/***************************************************************************
 *   Copyright (C) 2025 by Daniele Cattaneo                                *
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
#include "interfaces_private/smp_locks.h"
#include "kernel/lock.h"

#ifndef COMPILING_MIOSIX
#error "This is header is private, it can't be used outside Miosix itself."
#error "If your code depends on a private header, it IS broken."
#endif //COMPILING_MIOSIX

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file smp.h
 * This file defines the functions required to support multi-core platforms.
 * These functions only exist if the WITH_SMP macro is defined in the kernel
 * settings.
 */

namespace miosix {

#ifdef WITH_SMP

/**
 * \name SMP setup and teardown
 * \{
 */

/**
 * \internal
 * Starts symmetric multi-processing (SMP) support, enabling all cores and
 * initializing their state. This function must be called while holding the
 * Global Irq Lock (GIL). The main functions for the other cores will start
 * executing with interrupts disabled, but with the GIL not taken.
 * 
 * \param stackPtrs An array with one initial process stack pointer
 *                  for each core except core 0.
 * \param mains     An array with one pointer to a main function
 *                  for each core except core 0.
 */
void IRQinitSMP(void *const stackPtrs[], void (*const mains[])()) noexcept;

/**
 * \internal
 * Stops SMP support, for example because an unrecoverable system error
 * happened, by stopping all cores except the current one.
 * This function may be also called with interrupts locally disabled and the
 * GIL not yet taken.
 */
void IRQlockupOtherCores() noexcept;

/**
 * \}
 */

/**
 * \name Inter-processor operations
 * \{
 */

/**
 * \internal
 * Asynchronously call IRQinvokeScheduler on the specified core.
 * Note that if this function is called very frequently and the target processor
 * is very busy, multiple calls to IRQinvokeScheduler may be coalesced into one.
 */
void IRQinvokeSchedulerOnCore(unsigned char core) noexcept;

/**
 * \internal
 * Synchronously executes a given function on a specific core within an
 * interrupt context.
 * This function must be called while holding the global interrupt lock (GIL)
 * but it will release the GIL while waiting for the function to complete.
 * The function executed will also be run while holding the GIL.
 * 
 * \param lock A GlobalIrqLock (GIL) lock that must be already taken here.
 * \param core The core ID where to execute the function.
 * \param f    The function to execute on the core.
 * \param arg  The argument to pass to the function.
 */
void IRQcallOnCore(unsigned char core, void (*f)(void *), void *arg) noexcept;

/**
 * \}
 */

#endif //WITH_SMP

} // namespace miosix

/**
 * \}
 */
