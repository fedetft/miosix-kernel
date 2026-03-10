/***************************************************************************
 *   Copyright (C) 2018-2021 by Terraneo Federico                          *
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

/// Use RTC as os_timer. This requires a 32kHz crystal to be connected to the
/// board, reduces timing resolution to only 16kHz and makes context switches
/// much slower (due to RTC limitations, minimum time beween two context
/// switches becomes 91us), but the os can keep precise time even when the CPU
/// is clocked with an RC oscillator and time is kept across deep sleep
//#define WITH_RTC_AS_OS_TIMER

/// Size of stack for main().
/// The C standard library is stack-heavy (iprintf requires 1KB)
const unsigned int MAIN_STACK_SIZE=2048;

/// Clock options
enum class OscillatorType { HSI, HSE };
// Supported oscillator types: HSI, HSE
constexpr auto oscillatorType=OscillatorType::HSI;
constexpr unsigned int hseFrequency=8000000;
// Supported clock frequencies: 24000000, 36000000, 48000000, 56000000, 72000000
constexpr unsigned int sysclkFrequency=24000000;

/// Serial port
/// Serial ports 1 to 3 are available
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

/**
 * \}
 */

} //namespace miosix
