/***************************************************************************
 *   Copyright (C) 2018 by Filippi Nicole, Padalino Luca                   *
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
#include "interfaces/arch_registers.h"

#ifndef __CORTEX_M
#error This MPU implementation works only on ARM CORTEX M
#endif

namespace miosix {

/**
 * To be called in boot.cpp to configure the MPU for kernel-level W^X and
 * cacheability (if caches are present).
 * This function must be called if the board has and MPU or caches, but in
 * ARM CPUs all architectures with caches also have the MPU.
 * 
 * If the board has an external RAM attached, the XRAM base address and
 * size must be passed as parameters to this function to extend the W^X
 * protection and cacheability configuration to the XRAM as well, otherwise
 * pass the default nullptr and 0.
 * 
 * \param xramBase base address of external memory, if present, otherwise nullptr
 * \param xramSize size of external memory, if present, otherwise 0
 */
void IRQconfigureMPU(const unsigned int *xramBase=nullptr, unsigned int xramSize=0);


#if __CORTEX_M != 33U
/**
 * \internal
 * ARMv7M and lower CPUs only allow MPU regions whose size is a power of two,
 * and encode the size as the log2 of the actual size with an offset. From ARMv8
 * onward, any region size that is a multiple of 32 bytes is supported, so this
 * function is no longer required.
 *
 * This function convert a memory region size to a bit pattern that can be
 * written in ARMv7 or lowwer MPU registers.
 * \param size in bytes. Must be at least 32
 * \return a value that can be written to MPU->RASR to represent that size
 */
unsigned int sizeToMpu(unsigned int size);
#endif

} //namespace miosix
