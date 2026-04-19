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

//TODO: despite being a private interface, it is included by kernel/lock.h

#include "miosix_settings.h"

/**
 * \addtogroup Interfaces
 * \{
 */

 /**
 * \file smp_locks.h
 * This file defines the functions required to support locking on multi-core
 * platforms.
 * These functions only exist if the WITH_SMP macro is defined in the kernel
 * settings.
 */

namespace miosix {

#ifdef WITH_SMP

/**
 * \name Inter-processor hardware locking
 * \{
 */

/// \internal
/// Definition of statically allocated hardware locks
/// In a namespace to allow extension while keeping enum-class-like syntax
namespace HwLocks
{
    using ID = unsigned char;
    enum
    {
        GIL = 0,        /// Global interrupt lock
        PK,             /// Pause kernel lock
        KernelMax
    };
}

/**
 * \internal
 * Acquire an Hardware Irq Lock, meant to synchronize between threads and
 * interrupt handler on SMP platforms. This will be typically implemented in a
 * spinlock-like way.
 * This function can be called:
 *  - outside an interrupt handler, but with local interrupts disabled,
 *  - inside an interrupt handler.
 * \param i The ID of the lock.
 */
inline void irqDisabledHwIrqLockAcquire(HwLocks::ID i) noexcept;

/**
 * \internal
 * Releases the specified Hardware Irq Lock.
 * \param i The ID of the lock.
 */
inline void irqDisabledHwIrqLockRelease(HwLocks::ID i) noexcept;


/**
 * \internal
 * Acquire an Hardware Lock, meant to synchronize between threads only
 * on SMP platforms. This will be typically implemented in a
 * spinlock-like way. The difference from a Hardware *Irq* Lock is that
 * this lock allows peripheral interrupts to be served during the wait.
 * This function can be called only outside an interrupt handler, but with
 * local interrupts disabled.
 * \param i The ID of the lock.
 */
inline void irqDisabledHwLockAcquire(HwLocks::ID i) noexcept;

/**
 * \internal
 * Releases the specified Hardware  Lock.
 * \param i The ID of the lock.
 */
inline void irqDisabledHwLockRelease(HwLocks::ID i) noexcept;


/**
 * \internal
 * Same as irqDisabledHwLockAcquire but local interrupts do not need to be
 * disabled.
 */
inline void hwLockAcquire(HwLocks::ID i) noexcept;

/**
 * \internal
 * Same as irqDisabledHwLockRelease but local interrupts do not need to be
 * disabled.
 */
inline void hwLockRelease(HwLocks::ID i) noexcept;

/**
 * \}
 */

#endif //WITH_SMP

} // namespace miosix

/**
 * \}
 */

#ifdef WITH_SMP
#include "interfaces-impl/smp_locks_impl.h"
#endif
    
