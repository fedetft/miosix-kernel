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

#include "cortexMx_mpu.h"
#include "cache/cortexMx_cache.h"
#include "interfaces_private/userspace.h" //For IRQenableMPUatBoot
#include <utility>

using namespace std;

namespace miosix {

/**
 * Using the MPU, configure a region of the memory space as
 * - write-through cacheable
 * - non-shareable
 * - readable/writable/executable only by privileged code (for compatibility
 *   with the way processes use the MPU)
 * \param region MPU region. Note that region 6 and 7 are used by processes, and
 * should be avoided here
 * \param base base address, aligned to a 32Byte cache line
 * \param size size, must be at least 32 and a power of 2, or it is rounded to
 * the next power of 2
 */
static void IRQconfigureMPURegion(unsigned int region, unsigned int base,
    unsigned int size, bool executePermitted)
{
    // NOTE: The ARM documentation is unclear about the effect of the shareable
    // bit on a single core architecture. Experimental evidence on an STM32F476
    // shows that setting it in IRQconfigureMPU for the internal RAM region
    // causes the boot to fail.
    // For this reason, all regions are marked as not shareable
    MPU->RBAR=(base & (~(cacheLine-1))) | MPU_RBAR_VALID_Msk | region;
    MPU->RASR=(executePermitted ? 0 : 1<<MPU_RASR_XN_Pos)
               | 1<<MPU_RASR_AP_Pos //Privileged: RW, unprivileged: no access
               | MPU_RASR_C_Msk     //Cacheable, write through
               | 1                  //Enable bit
               | sizeToMpu(size)<<1;
}

void IRQconfigureMPU(const unsigned int *xramBase, unsigned int xramSize)
{
    // NOTE: using regions 0 to 3 for the kernel because in the ARM MPU in case
    // of overlapping regions the one with the highest number takes priority.
    // The lower regions are used by the kernel and by default forbid access to
    // unprivileged code, while the higher numbered ones are used by processes
    // to override the default deny policy for the process-specific memory.
    IRQconfigureMPURegion(0,0x00000000,0x20000000,true);
    IRQconfigureMPURegion(1,0x20000000,0x20000000,false);
    if(xramSize)
        IRQconfigureMPURegion(2,reinterpret_cast<unsigned int>(xramBase),xramSize,false);
    IRQenableMPUatBoot();
    
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT==1)
    SCB_EnableICache();
    SCB_EnableDCache();
    #endif
}
}
