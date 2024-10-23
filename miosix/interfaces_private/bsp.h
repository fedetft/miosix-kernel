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
 * will call.
 * It must provide these functions:
 *
 * IRQbspInit(), to initialize the board to a known state early in the boot
 * process (before the kernel is started, and when interrupts are disabled)
 *
 * bspInit2(), to perform the remaining part of the initialization, once the
 * kernel is started
 */

namespace miosix {

/**
 * \internal
 * Initializes the I/O pins, and put system peripherals to a known state.<br>
 * Must be called before starting kernel. Interrupts must be disabled.<br>
 * This function is used by the kernel and should not be called by user code.
 */
void IRQbspInit();

/**
 * \internal
 * Performs the part of initialization that must be done after the kernel is
 * started.<br>
 * This function is used by the kernel and should not be called by user code.
 */
void bspInit2();

} //namespace miosix

/**
 * \}
 */
