/***************************************************************************
 *   Copyright (C) 2018 by Terraneo Federico                               *
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

#ifndef CACHE_CORTEX_MX_H
#define CACHE_CORTEX_MX_H

/**
 * A note about caches. Using the testsuite benchmark, when caches are
 * disabled the STM32F746 @ 216MHz is less than half the speed of the
 * STM32F407 @ 168MHz. By enabling the ICACHE things get better, but it is
 * still slower, and achieves a speedup of 1.53 only when both ICACHE and
 * DCACHE are enabled. The speedup also includes the gains due to the faster
 * clock frequency. So if you want speed you have to use caches.
 * Moreover, DMA drivers such as the serial port fail also when DCACHE is
 * disabled, and similar issues have been reported on the net as well.
 * So, there's really no reason to disable caches.
 */

#include "interfaces/arch_registers.h"

namespace miosix {

/**
 * To be called in stage_1_boot.cpp to configure caches.
 * Only call this function if the board has caches.
 * \param xramBase base address of external memory, if present, otherwise nullptr
 * \param xramSize size of external memory, if present, otherwise 0
 */
void IRQconfigureCache(const unsigned int *xramBase=nullptr, unsigned int xramSize=0);

/**
 * Call this function to mark a buffer before starting a DMA transfer where
 * the DMA will read the buffer.
 * \param buffer buffer
 * \param size buffer size
 */
inline void markBufferBeforeDmaWrite(const void *buffer, int size)
{
    // You may think that since the cache is configured as write-through,
    // there's nothing to do before the DMA can read a memory buffer just
    // written by the CPU, right? Wrong! Othere than the cache, there's the
    // write buffer to worry about. My hypothesis is that once a memory region
    // is marked as cacheable, the write buffer becomes more lax in
    // automatically flushing as soon as possible. In the stm32 serial port
    // driver writing just a few characters causes garbage to be printed if
    // this __DSB() is removed. Apparently, the characters remian in the write
    // buffer.
    __DSB();
}

/**
 * Call this function after having completed a DMA transfer where the DMA has
 * written to the buffer.
 * \param buffer buffer
 * \param size buffer size
 */
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT==1)
void markBufferAfterDmaRead(void *buffer, int size);
#else
inline void markBufferAfterDmaRead(void *buffer, int size) {}
#endif

} //namespace miosix

#endif //CACHE_CORTEX_MX_H
