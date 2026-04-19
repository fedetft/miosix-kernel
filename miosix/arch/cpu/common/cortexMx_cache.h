/***************************************************************************
 *   Copyright (C) 2018-2026 by Terraneo Federico                          *
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

/*
 * README: Essentials about how cache support is implemented.
 * 
 * Caches in the Cortex M7 are transparent to software, except when using
 * the DMA. As the DMA reads and writes directly to memory, explicit management
 * is required. The default cache policy is write-back, but following the
 * preferred cache policy described in interfaces/cache.h, this code assumes
 * that the cache has been configured as write-through by the MPU code.
 * Unfortunately in ARM Cortex, cache configuration is done through the MPU,
 * thus this code is tightly coupled to the MPU code.
 * 
 * A note about performance. Using the testsuite benchmark, when caches are
 * disabled the STM32F746 @ 216MHz is less than half the speed of the
 * STM32F407 @ 168MHz. By enabling the ICACHE things get better, but it is
 * still slower, and achieves a speedup of 1.53 only when both ICACHE and
 * DCACHE are enabled. The speedup also includes the gains due to the faster
 * clock frequency. So if you want speed you have to use caches.
 */

#include "interfaces/arch_registers.h"

namespace miosix {

static const unsigned int cacheLine=32; //Cortex-M7 cache line size

inline void markBufferBeforeDmaWrite(const void *buffer, int size)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT==1)
    // You may think that since the cache is configured as write-through,
    // there's nothing to do before the DMA can read a memory buffer just
    // written by the CPU, right? Wrong! Other than the cache, there's the
    // write buffer to worry about. My hypothesis is that once a memory region
    // is marked as cacheable, the write buffer becomes more lax in
    // automatically flushing as soon as possible. In the stm32 serial port
    // driver writing just a few characters causes garbage to be printed if
    // this __DSB() is removed. Apparently, the characters remian in the write
    // buffer.
    __DSB();
#endif
}

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT==1)
void markBufferAfterDmaRead(void *buffer, int size);
#else
inline void markBufferAfterDmaRead(void *buffer, int size) {}
#endif

inline void IRQenableCache()
{
    #if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT==1)
    SCB_EnableICache();
    #endif
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT==1)
    SCB_EnableDCache();
    #endif
}

} //namespace miosix
