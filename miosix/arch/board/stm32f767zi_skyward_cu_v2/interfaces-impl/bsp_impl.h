/***************************************************************************
 *   Copyright (C) 2023-2026 by Davide Mor, Raul Radu                      *
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
#pragma once

#include "config/miosix_settings.h"
#include "interfaces/gpio.h"

namespace miosix
{

/**
\addtogroup Hardware
\{
*/

/**
 * \internal
 * Called by stage_1_boot.cpp to enable the SDRAM before initializing .data/.bss
 * Requires the CPU clock to be already configured (running from the PLL)
 */
void configureSdram();

/**
 * \internal
 * Board pin definition
 */
typedef Gpio<PC,15> userLed1;
typedef Gpio<PC,2> userLed2;
typedef Gpio<PC,13> userLed3;
typedef Gpio<PC,14> userLed4;

inline void led1On() { userLed1::high(); }

inline void led1Off() { userLed1::low(); }

inline void led2On() { userLed2::high(); }

inline void led2Off() { userLed2::low(); }

inline void led3On() { userLed3::high(); }

inline void led3Off() { userLed3::low(); }

inline void led4On() { userLed4::high(); }

inline void led4Off() { userLed4::low(); }

inline void ledOn()
{
    led1On();
    led2On();
    led3On();
    led4On();
}

inline void ledOff()
{
    led1Off();
    led2Off();
    led3Off();
    led4Off();
}

/**
 * Polls the SD card sense GPIO.
 *
 * This board has no SD card whatsoever, but a card can be connected to the
 * following GPIOs:
 * TODO: never tested
 *
 * \return true. As there's no SD card sense switch, let's pretend that
 * the card is present.
 */
inline bool sdCardSense() { return true; }

/**
\}
*/

}  // namespace miosix

