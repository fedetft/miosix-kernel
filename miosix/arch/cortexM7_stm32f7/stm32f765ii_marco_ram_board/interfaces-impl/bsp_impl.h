/***************************************************************************
 *   Copyright (C) 2018 by Terraneo Federico                               *
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

/***************************************************************************
 * bsp_impl.h Part of the Miosix Embedded OS.
 * Board support package, this file initializes hardware.
 ***************************************************************************/

#ifndef BSP_IMPL_H
#define BSP_IMPL_H

#include "config/miosix_settings.h"
#include "interfaces/gpio.h"

namespace miosix {

/**
\addtogroup Hardware
\{
*/

/**
 * \internal
 * Board pin definition
 */
using led1 = Gpio<GPIOI_BASE,8>;
using led2 = Gpio<GPIOI_BASE,11>;
using btn0 = Gpio<GPIOF_BASE,9>; // SW504
using btn1 = Gpio<GPIOF_BASE,8>; // SW503
using btn2 = Gpio<GPIOF_BASE,7>; // SW502
using btn3 = Gpio<GPIOF_BASE,6>; // SW501
using sdmmcCD = Gpio<GPIOC_BASE,6>;
using sdmmcWP = Gpio<GPIOC_BASE,7>;

inline void ledOn()
{
    led1::high();
}

inline void ledOff()
{
    led1::low();
}

inline void led1On() { led1::high(); }

inline void led1Off() { led1::low(); }

inline void led2On() { led2::high(); }

inline void led2Off() { led2::low(); }

/**
 * Polls the SD card sense GPIO.
 *
 * This board has a Hirose DM1A SD card connector with external pullup for the
 * card detect pin, which is shorted to ground when a card is inserted.
 */
inline bool sdCardSense()
{
    return !sdmmcCD::value();
}

/**
\}
*/

}  // namespace miosix

#endif //BSP_IMPL_H
