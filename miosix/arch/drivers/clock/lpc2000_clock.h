/***************************************************************************
 *   Copyright (C) 2008-2026 by Terraneo Federico                          *
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
 *   by the GNU General Public License. However the suorce code for this   *
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
 * \internal
 * Constants to pass to setPllFreq(), and values returned by getPllFreq()
 * This enum is used by the system and should not be called by user code.
 */
enum PllValues
{
    PLL_4X=4,   ///<\internal AHB clock frequency = xtal freq. multiplied by 4
    PLL_2X=2,   ///<\internal AHB clock frequency = xtal freq. multiplied by 2
    PLL_OFF=0,  ///<\internal AHB clock frequency = xtal freq.
    PLL_UNDEF=-1///<\internal Returned by getPllFreq() in case of errors
};

/**
 * \internal
 * Set PLL frequency
 * This is is only tested with a 14.7456MHz xtal
 * Changing PLL frequency after boot may cause problems to hardware drivers
 * \param r the desired PLL settings
 *
 * This function is used by the system and should not be called by user code.
 */
void IRQsetPllFreq(PllValues r);

/**
 * \internal
 * Get PLL settings
 * This is is only tested with a 14.7456MHz xtal
 * \return the current PLL settings
 *
 * This function is used by the system and should not be called by user code.
 */
PllValues IRQgetPllFreq();

/**
 * \internal
 * Constants to pass to set_apb_ratio(), and values returned by get_apb_ratio()
 * This enum is used by the system and should not be called by user code.
 */
enum APBValues
{
    APB_DIV_1=1,///<\internal AHB and APB run @ same speed
    APB_DIV_2=2,///<\internal APB runs at 1/2 of AHB frequency
    APB_DIV_4=0 ///<\internal APB runs at 1/4 of AHB frequency
};

/**
 * \internal
 * Set apb ratio (how much peripherals are slowed down with respect to cpu)
 * Changing apb ratio after boot may cause problems to hardware drivers
 * \param r desired apb ratio
 *
 * This function is used by the system and should not be called by user code.
 */
inline void IRQsetApbRatio(APBValues r)
{
    VPBDIV=r;
}

/**
 * \internal
 * Get apb ratio (how much peripherals are slowed down with respect to cpu)
 * \return current apb ratio
 *
 *This function is used by the system and should not be called by user code.
 */
inline APBValues IRQgetApbRatio()
{
    return static_cast<APBValues>(VPBDIV & 0x3);
}

} //namespace miosix
