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

#ifndef __NVIC_PRIO_BITS
#error "__NVIC_PRIO_BITS undefined"
#endif //__NVIC_PRIO_BITS

/// Default interrupt priority. All interrupt priorities are set at boot to this
/// value. ARM Cortex use 0 for the highest priority and (1<<__NVIC_PRIO_BITS)-1
/// for the lowest one. We chose to use the top 3/4 of the range for higher than
/// default priority and the bottom 1/4 of the range for lower than default.
/// With 4 bit priorities the default is 11
/// With 3 bit priorities the default is 5
/// With 2 bit priorities the default is 2
constexpr int defaultIrqPriority=(0.75f*(1<<__NVIC_PRIO_BITS))-1;

/// Minimum interrupt priority that the hardware provides
constexpr int minimumIrqPriority=(1<<__NVIC_PRIO_BITS)-1;

inline void fastDisableIrq() noexcept
{
    //Since this function is inline there's the need for a memory barrier to
    //avoid aggressive reordering
    asm volatile("cpsid i \n\t"
                 "dsb     \n\t":::"memory");
}

inline void fastEnableIrq() noexcept
{
    //Since this function is inline there's the need for a memory barrier to
    //avoid aggressive reordering
    asm volatile("cpsie i \n\t"
                 "dsb     \n\t":::"memory");
}

inline bool areInterruptsEnabled() noexcept
{
    int i;
    asm volatile("mrs   %0, primask    \n\t":"=r"(i));
    if(i!=0) return false;
    return true;
}

/**
 * \}
 */

} //namespace miosix
