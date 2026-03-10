/***************************************************************************
 *   Copyright (C) 2012-2021 by Terraneo Federico                          *
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
/// The C standard library is stack-heavy (iprintf requires 1.5KB) and the
/// STM32F207IG has 128KB of RAM so there is room for a big 4K stack.
const unsigned int MAIN_STACK_SIZE=4*1024;

/// Clock options
enum class OscillatorType { HSI, HSE };
// Supported oscillator types: HSE
constexpr auto oscillatorType=OscillatorType::HSE;
constexpr unsigned int hseFrequency=25000000;
// Supported clock frequencies: 120000000
constexpr unsigned int sysclkFrequency=120000000;

/// Serial port
/// Serial ports 1 to 6 are available
const unsigned int defaultSerial=1;
const unsigned int defaultSerialSpeed=115200;
const bool defaultSerialFlowctrl=false;
const bool defaultSerialDma=true;
// Default serial 1 pins (uncomment when using serial 1)
using defaultSerialTxPin = Gpio<PA,9>;
using defaultSerialRxPin = Gpio<PA,10>;
using defaultSerialRtsPin = Gpio<PA,12>;
using defaultSerialCtsPin = Gpio<PA,11>;
// Default serial 2 pins (uncomment when using serial 2)
//using defaultSerialTxPin = Gpio<PA,2>;
//using defaultSerialRxPin = Gpio<PA,3>;
//using defaultSerialRtsPin = Gpio<PA,1>;
//using defaultSerialCtsPin = Gpio<PA,0>;
// Default serial 3 pins (uncomment when using serial 3)
//using defaultSerialTxPin = Gpio<PB,10>;
//using defaultSerialRxPin = Gpio<PB,11>;
//using defaultSerialRtsPin = Gpio<PB,14>;
//using defaultSerialCtsPin = Gpio<PB,13>;

///\def STDOUT_REDIRECTED_TO_DCC
///If defined, stdout is redirected to the debug communication channel, and
///will be printed if OpenOCD is connected. If not defined, stdout will be
///redirected throug USART1, as usual.
//#define STDOUT_REDIRECTED_TO_DCC

//SD card driver
static const unsigned char sdVoltage=33; //Board powered @ 3.3V
//#define SD_ONE_BIT_DATABUS //Using 4 bit fast bus

/**
 * \}
 */

} //namespace miosix
