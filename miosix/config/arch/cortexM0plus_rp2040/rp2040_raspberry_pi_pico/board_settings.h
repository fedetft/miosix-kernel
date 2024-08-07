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

#pragma once

#include "interfaces/gpio.h"

/**
 * \internal
 * Versioning for board_settings.h for out of git tree projects
 */
#define BOARD_SETTINGS_VERSION 300

namespace miosix {

/**
 * \addtogroup Settings
 * \{
 */

/// Size of stack for main().
/// The C standard library is stack-heavy (iprintf requires 1KB) but the
/// RP2040 has 264KB of RAM so there is room for a big 4K stack.
const unsigned int MAIN_STACK_SIZE=4*1024;

/// Serial port
const unsigned int defaultSerialSpeed=115200;
const bool defaultSerialFlowctrl=false;
// Uncomment to use UART 0
#define DEFAULT_SERIAL_ID 0
using uart_tx = Gpio<GPIOA_BASE, 0>;
using uart_rx = Gpio<GPIOA_BASE, 1>;
using uart_cts = Gpio<GPIOA_BASE, 2>; // Used only with flow control active
using uart_rts = Gpio<GPIOA_BASE, 3>; // Used only with flow control active
// Uncomment to use UART 1
//#define DEFAULT_SERIAL_ID 1
//using uart_tx = Gpio<GPIOA_BASE, 4>;
//using uart_rx = Gpio<GPIOA_BASE, 5>;
//using uart_cts = Gpio<GPIOA_BASE, 6>; // Used only with flow control active
//using uart_rts = Gpio<GPIOA_BASE, 7>; // Used only with flow control active

/**
 * \}
 */

} //namespace miosix
