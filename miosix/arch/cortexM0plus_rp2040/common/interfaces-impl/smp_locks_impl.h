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

#ifdef WITH_SMP

#include "hw_spinlock.h"

namespace miosix {

inline void irqDisabledHwIrqLockAcquire(HwLocks::ID i) noexcept
{
    IRQhwSpinlockAcquire(i);
}

inline void irqDisabledHwIrqLockRelease(HwLocks::ID i) noexcept
{
    IRQhwSpinlockRelease(i);
}

inline void irqDisabledHwLockAcquire(HwLocks::ID i) noexcept
{
    irqDisabledHwSpinlockAcquire(i);
}

inline void irqDisabledHwLockRelease(HwLocks::ID i) noexcept
{
    hwSpinlockRelease(i);
}

inline void hwLockAcquire(HwLocks::ID i) noexcept
{
    fastDisableIrq();
    irqDisabledHwSpinlockAcquire(i);
    fastEnableIrq();
}

inline void hwLockRelease(HwLocks::ID i) noexcept
{
    hwSpinlockRelease(i);
}

} // namespace miosix

#endif // WITH_SMP
