/***************************************************************************
 *   Copyright (C) 2012-2021 by Terraneo Federico                          *
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
/// STM32F415VG only has 192KB of RAM so there is room for a big 4K stack.
const unsigned int MAIN_STACK_SIZE=4*1024;

/// Serial port
//Serial ports 1 to 6 are available, serial 2 is brought over to unsoldered
//headers on the PCB, serial 6 is connected to the STLink USB ACM adapter,
//and serial 3 is connected to the WiFi module (not fitted)
const unsigned int defaultSerial=6;
const unsigned int defaultSerialSpeed=115200;
const bool defaultSerialFlowctrl=false;
const bool defaultSerialDma=true;
// Default serial 2 pins (uncomment when using serial 2)
//using defaultSerialTxPin = Gpio<PA,2>;
//using defaultSerialRxPin = Gpio<PA,3>;
//using defaultSerialRtsPin = Gpio<PA,1>;
//using defaultSerialCtsPin = Gpio<PA,0>;
// Default serial 6 pins (uncomment when using serial 6)
using defaultSerialTxPin = Gpio<PC,6>;
using defaultSerialRxPin = Gpio<PC,7>;
using defaultSerialRtsPin = Gpio<PG,12>; //unused
using defaultSerialCtsPin = Gpio<PG,13>; //unused

//SD card driver
//TODO: this board does not have an SD card connector and does not have any
//GPIO breakout pins for an external connector, so it makes no sense to support
//filesystem functionality for it
static const unsigned char sdVoltage=33; //Board powered @ 3.3V
#define SD_ONE_BIT_DATABUS //Can't use 4 bit databus due to pin conflicts

/**
 * \}
 */

} //namespace miosix
