/***************************************************************************
 *   Copyright (C) 2017 by Terraneo Federico                               *
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

#ifndef BOARD_SETTINGS_H
#define	BOARD_SETTINGS_H

#include "util/version.h"

/**
 * \internal
 * Versioning for board_settings.h for out of git tree projects
 */
#define BOARD_SETTINGS_VERSION 100

namespace miosix {

/**
 * \addtogroup Settings
 * \{
 */

/// Size of stack for main().
/// The C standard library is stack-heavy (iprintf requires 1KB) but the
/// STM32F100C8 only has 8KB of RAM so the stack is only 1.5KB.
const unsigned int MAIN_STACK_SIZE=1024+512;

/// Frequency of tick (in Hz). The frequency of the STM32F100CB timer in the
/// stm32vldiscovery board can be divided by 1000. This allows to use a 1KHz
/// tick and the minimun Thread::sleep value is 1ms
/// For the priority scheduler this is also the context switch frequency
const unsigned int TICK_FREQ=1000;

///\internal Aux timer run @ 100KHz
///Note that since the timer is only 16 bits this imposes a limit on the
///burst measurement of 655ms. If due to a pause_kernel() or
///disable_interrupts() section a thread runs for more than that time, a wrong
///burst value will be measured
const unsigned int AUX_TIMER_CLOCK=100000;
const unsigned int AUX_TIMER_MAX=0xffff; ///<\internal Aux timer is 16 bits

/// Serial port
const unsigned int defaultSerial=1;
const unsigned int defaultSerialSpeed=19200;
const bool defaultSerialFlowctrl=false;
#define SERIAL_1_DMA
//#define SERIAL_2_DMA //Serial 1 is not used, so not enabling DMA
//#define SERIAL_3_DMA //Serial 1 is not used, so not enabling DMA

/**
 * \}
 */

} //namespace miosix

#endif	/* BOARD_SETTINGS_H */
