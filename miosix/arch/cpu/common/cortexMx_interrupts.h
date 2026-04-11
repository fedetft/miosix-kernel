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

#include "miosix_settings.h"
#include "interfaces/arch_registers.h"
#include "interfaces/cpu_const.h"

namespace miosix {

#ifdef WITH_SMP

/**
 * \internal Register an IRQ handler on the current core.
 * 
 * \note This function is specific for Cortex-M processors. In multicore
 * microcontrollers with Cortex-M processors, the interrupt controllers are
 * NVICs, as opposed to the GICs found in Cortex-A cores. Therefore,
 * IRQregisterIrqOnCore functions are implemented by using IRQcallOnCore to
 * dispatch a call to this function to the correct core. But, if we already
 * know which processor we are running on, it is more appropriate to use
 * the *OnCurrentCore functions directly, and this is why they are made
 * available here.
 */
void IRQregisterIrqOnCurrentCore(unsigned int id, void (*handler)(void*), void *arg) noexcept;

/**
 * \internal Unregister an IRQ handler on the current core.
 * 
 * \note This function is specific for Cortex-M processors. In multicore
 * microcontrollers with Cortex-M processors, the interrupt controllers are
 * NVICs, as opposed to the GICs found in Cortex-A cores. Therefore,
 * IRQregisterIrqOnCore functions are implemented by using IRQcallOnCore to
 * dispatch a call to this function to the correct core. But, if we already
 * know which processor we are running on, it is more appropriate to use
 * the *OnCurrentCore functions directly, and this is why they are made
 * available here.
 */
void IRQunregisterIrqOnCurrentCore(unsigned int id, void (*handler)(void*), void *arg) noexcept;

#endif // WITH_SMP

} //namespace miosix
