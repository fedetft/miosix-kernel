/***************************************************************************
 *   Copyright (C) 2010-2024 by Terraneo Federico                          *
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

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file bsp.h
 * This file provides architecture specific initialization code that the kernel
 * will call during boot.
 */

namespace miosix {

/**
 * \internal
 * First part of BSP initialization. This function should perform the initial
 * board initialization. This function is called during boot before the kernel
 * is started, while interrupts are still disabled.
 *
 * After this function is called, the kernel will start printing boot logs, so
 * the console device should be initialized here, typically writing to a serial
 * port.
 */
void IRQbspInit();

/**
 * \internal
 * Second part of BSP initialization. This function should complete the board
 * initialization with the steps that requires the kernel to be started and
 * interrupts to be enabled.
 *
 * Typically, filesystem initialization and mounting partitions goes here.
 */
void bspInit2();

} //namespace miosix

/**
 * \}
 */
