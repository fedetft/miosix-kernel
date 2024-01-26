/***************************************************************************
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
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

/***********************************************************************
* bsp_impl.h Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#ifndef BSP_IMPL_H
#define BSP_IMPL_H

#include "config/miosix_settings.h"
#include "interfaces/gpio.h"

namespace miosix {

/**
\addtogroup Hardware
\{
*/


#ifdef PICO_DEFAULT_LED_PIN
/**
 * \internal
 * used by the ledOn() and ledOff() implementation
 * \note Doesn't work on Pico W, as the LED is controlled by the WiFi chip.
 */
using led = Gpio<GPIOA_BASE, PICO_DEFAULT_LED_PIN>;
#endif

/**
 * Turn on the board LED.
 * \note Doesn't work on Pico W, as the LED is controlled by the WiFi chip.
 */
inline void ledOn()
{
#ifdef PICO_DEFAULT_LED_PIN
    led::high();
#endif
}

/**
 * Turn off the board LED.
 * \note Doesn't work on Pico W, as the LED is controlled by the WiFi chip.
 */
inline void ledOff()
{
#ifdef PICO_DEFAULT_LED_PIN
    led::low();
#endif
}

/**
\}
*/

} //namespace miosix

#endif //BSP_IMPL_H
