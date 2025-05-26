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

/// \def WITH_SMP
/// Enables multicore support
#define WITH_SMP

/// Size of stack for main().
/// The C standard library is stack-heavy (iprintf requires 1KB) but the
/// RP2040 has 264KB of RAM so there is room for a big 4K stack.
const unsigned int MAIN_STACK_SIZE=4*1024;

/// Clock options
enum class OscillatorType { XOSC }; //Only one supported for now
constexpr auto oscillatorType=OscillatorType::XOSC;
constexpr unsigned int oscillatorFrequency=12000000;
// Supported clock frequencies: 125000000, 133000000, 200000000
constexpr unsigned int cpuFrequency=200000000;
constexpr unsigned int peripheralFrequency=cpuFrequency;

/// Serial port
const unsigned int defaultSerial=0; // 0 or 1
const unsigned int defaultSerialSpeed=115200;
const bool defaultSerialFlowctrl=false;
const bool defaultSerialDma=true;
// Pin mapping for usart0, uncomment if defaultSerial==0
using defaultSerialTxPin = Gpio<P0, 0>;
using defaultSerialRxPin = Gpio<P0, 1>;
using defaultSerialRtsPin = Gpio<P0, 15>; // Gpio<P0, 3> conflicts with SD default
using defaultSerialCtsPin = Gpio<P0, 14>; // Gpio<P0, 2> conflicts with SD default
// Pin mapping for usart1, uncomment if defaultSerial==1
//using defaultSerialTxPin = Gpio<P0, 8>; // Gpio<P0, 4> conflicts with SD default
//using defaultSerialRxPin = Gpio<P0, 9>; // Gpio<P0, 5> conflicts with SD default
//using defaultSerialRtsPin = Gpio<P0, 7>;
//using defaultSerialCtsPin = Gpio<P0, 6>;

// SD card
const bool enableSdCard=true;
// Dma and NoDma use the SPI device and have pin constraints, Software does not
// The NoDma driver is faster, the Dma driver is slightly slower but uses way
// less CPU, and the Software driver is the slowest but can be used if other
// devices are using the SPI peripherals.
enum class SdCardDriverType { Dma, NoDma, Software };
const SdCardDriverType defaultSdCardDriver=SdCardDriverType::Dma;
const unsigned int defaultSdCardSPI=0; // 0 or 1 (only for non-Software drivers)
// Pin mapping for spi0, uncomment if defaultSdCardSPI==0
using defaultSdCardSPISckPin = Gpio<P0, 2>; // SD CLK
using defaultSdCardSPISoPin = Gpio<P0, 3>; // SD CMD
using defaultSdCardSPISiPin = Gpio<P0, 4>; // SD D0
using defaultSdCardSPICsPin = Gpio<P0, 5>; // SD D3
// Pin mapping for spi1, uncomment if defaultSdCardSPI==1
//using defaultSdCardSPISckPin = Gpio<P0, 10>; // SD CLK
//using defaultSdCardSPISoPin = Gpio<P0, 11>; // SD CMD
//using defaultSdCardSPISiPin = Gpio<P0, 12>; // SD D0
//using defaultSdCardSPICsPin = Gpio<P0, 13>; // SD D3

/**
 * \}
 */

} //namespace miosix
