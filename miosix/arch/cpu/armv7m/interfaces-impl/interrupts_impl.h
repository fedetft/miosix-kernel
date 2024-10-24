/***************************************************************************
 *   Copyright (C) 2010-2024 by Terraneo Federico                          *
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

#include "interfaces/arch_registers.h"

namespace miosix {

/**
 * \addtogroup Interfaces
 * \{
 */

inline void fastDisableInterrupts() noexcept
{
    // Documentation says __disable_irq() disables all interrupts with
    // configurable priority, so also SysTick and SVC.
    // No need to disable faults with __disable_fault_irq()
    __disable_irq();
    //The new fastDisableInterrupts/fastEnableInterrupts are inline, so there's
    //the need for a memory barrier to avoid aggressive reordering
    asm volatile("":::"memory");
}

inline void fastEnableInterrupts() noexcept
{
    __enable_irq();
    //The new fastDisableInterrupts/fastEnableInterrupts are inline, so there's
    //the need for a memory barrier to avoid aggressive reordering
    asm volatile("":::"memory");
}

inline bool areInterruptsEnabled() noexcept
{
    register int i;
    asm volatile("mrs   %0, primask    \n\t":"=r"(i));
    if(i!=0) return false;
    return true;
}

/**
 * \}
 */

} //namespace miosix
