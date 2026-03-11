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

#include "cortexMx_cache.h"
#include <utility>

using namespace std;

namespace miosix {

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT==1)
/**
 * Align a generic buffer to another one that contains the first one, but the
 * start size is aligned to a cache line
 */
static pair<uint32_t*, int32_t> alignBuffer(void *buffer, int size)
{
    auto bufferAddr=reinterpret_cast<unsigned int>(buffer);
    
    auto base=bufferAddr & (~(cacheLine-1));
    size+=bufferAddr-base;
    
    return make_pair(reinterpret_cast<uint32_t*>(base),size);
}

void markBufferAfterDmaRead(void *buffer, int size)
{
    //Since the current cache policy is write-through, we just invalidate the
    //cache lines corresponding to the buffer. No need to flush (clean) the cache.
    auto result=alignBuffer(buffer,size);
    SCB_InvalidateDCache_by_Addr(result.first,result.second);
}
#endif

} //namespace miosix
