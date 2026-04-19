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

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file cache.h
 * This file contains code to flush cached in drivers that use DMA.
 *
 * Miosix, whenever permitted by the underlying architecture, configures the
 * data cache as write through. Why not supporting write-back?
 *
 * When writing data from memory to a peripheral using DMA, things are easy
 * also with write-back. You just flush (clean) the relevant cache lines, and
 * the DMA has access to the correct values. So it looks like it's ok.
 * When instead the DMA has to write to a memory location things become
 * complicated. Assume that a buffer not aligned to a cache line is passed to
 * a DMA read routine. After that a context switch happens and another thread
 * writes to a memory location that is on the same cache line as the (beginning
 * or end of) the buffer passed to the DMA. At the same time the DMA is writing
 * to the buffer.
 * At the end the situation looks like this, where the thread has written to
 * location X in the cache, while the DMA has written Y to the buffer.
 * <-- outside buffer --x-- buffer -->
 * +----------------------------------+
 * |   X                |             | cache
 * +----------------------------------+
 * |                    |YYYYYYYYYYYYY| memory
 * +----------------------------------+
 * What are you suppose to do? If you flush (clean) the cache line, X will be
 * committed to memory, but the Y data written by the DMA will be lost. If you
 * invalidate the cache, Y is preserved, but X is lost.
 * If you're just thinking that the problem can be solved by making sure buffers
 * are aligned to the cache line (and their size is a multiple of the cache
 * line), well, there's a problem.
 * Miosix is a real-time OS, and for performance and safety, most drivers are
 * zero copy. Applications routinely pass to DMA drivers such as the SDIO large
 * buffers (think 100+KB). Of course allocating an aligned buffer inside the
 * DMA driver as large as the user-passed buffer and copying the data there
 * isn't just slow, it wouldn't be safe, as you risk to exceed the free heap
 * memory or fragment the heap. Allocating a small buffer and splitting the
 * large DMA transfer in many small ones where the user passed buffer is copyied
 * one chunk at a time would be feasible, but even slower, and even more so
 * considering that some peripherals such as SD cards are optimized for large
 * sequential writes, not for small chunks.
 * But what if we make sure all buffers passed to DMA drivers are aligned?
 * That is a joke, as it the burden of doing so is unmaintainable. Buffers
 * passed to DMA memory are everywhere, in the C/C++ standard library
 * (think the buffer used for IO formatting inside printf/fprintf), and
 * everywhere in application code. Something like
 * char s[128]="Hello world";
 * puts(s);
 * may cause s to be passed to a DMA driver. We would spend our lives chasing
 * unaligned buffers, and the risk of this causing difficult to reproduce memory
 * corruptions is too high. For this reason, Miosix prefers write-through
 * caches.
 *
 * Should an architecture be added where caches have to be write back, then all
 * DMA-enabled drivers for that architecture will hae to be written to not be
 * zero copy, and instead to always copy the passed data in fixed size buffers
 * that are cache-aligned, to the detriment of driver performance and causing
 * increased memory use.
 *
 * At the time of writing, all architectures Miosix supports that have caches
 * provide a way to configure the caches as write through.
 */

namespace miosix {

/**
 * When writing DMA-enabled drivers, before passing a buffer to the DMA for it
 * to be written to a peripheral, call this function to keep the DMA operations
 * in sync with the cache.
 *
 * This function becomes a no-op for architectures without DCACHE, so you can
 * freely put it in any DMA-enabled driver.
 * \param buffer buffer
 * \param size buffer size
 */
void markBufferBeforeDmaWrite(const void *buffer, int size);

/**
 * After a DMA read from a peripheral to a memory buffer has completed,
 * call this function to keep the DMA operations in sync with the cache.
 *
 * This function becomes a no-op for architectures without DCACHE, so you can
 * freely put it in any DMA-enabled driver.
 * \param buffer buffer
 * \param size buffer size
 */
void markBufferAfterDmaRead(void *buffer, int size);

/**
 * \internal
 * Enable caches. This function is called by the kernel in the architecture
 * independent boot.cpp to enable caches early at boot.
 * It must not be called by application code.
 * It must not be called by device drivers.
 * It must not be called by board support packages.
 * TODO: make an interfaces-private header only for this function?
 */
void IRQenableCache();

} //namespace miosix

/**
 * \}
 */

#include "interfaces-impl/cache_impl.h"
