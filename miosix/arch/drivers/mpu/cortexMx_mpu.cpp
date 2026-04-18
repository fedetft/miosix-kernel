/***************************************************************************
 *   Copyright (C) 2018 by Filippi Nicole, Padalino Luca                   *
 *   Copyright (C) 2026 by Alain Carlucci, Terraneo Federico               *
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
#include "drivers/cache/cortexMx_cache.h"
#include "kernel/error.h"
#include "miosix_settings.h"

#if __MPU_PRESENT==1

using namespace std;

namespace miosix {

/**
 * Using the MPU, configure a region of the memory space as
 * - write-through cacheable
 * - non-shareable
 * - readable/writable only by privileged code (for compatibility with the way
 *   processes use the MPU)
 * \param region MPU region. Note that region 6 and 7 are used by processes, and
 * should be avoided here
 * \param base base address, aligned to a 32Byte cache line
 * \param size size, must be at least 32. For ARMv7M and lower must also be a
 * power of 2
 * \param executePermitted if true configure MPU to allow code execution,
 * if false configure MPU to trap code execution for W^X protection
 */

static void IRQconfigureMPURegion(unsigned int region, unsigned int base,
    unsigned int size, bool executePermitted)
{
    #if __CORTEX_M == 33U
    // ARMv8-M
    const unsigned int MPU_RLAR_PXN_Msk=1<<4; //This bit is missing in the ARM .h
    MPU->RNR=region;
    MPU->RBAR=(base & (~0x1f))
             | (executePermitted ? 2<<MPU_RBAR_AP_Pos : MPU_RBAR_XN_Msk); //W^X
    MPU->RLAR=((base+size-1) & (~0x1f))
             | (executePermitted ? 0 : MPU_RLAR_PXN_Msk)
             | (0<<MPU_RLAR_AttrIndx_Pos) //NOTE: only region 0 enabled in MAIR0
             | 1;                 //Enable bit
    asm volatile("dsb":::"memory");
    #else
    // ARMv7-M
    // NOTE: The ARM documentation is unclear about the effect of the shareable
    // bit on a single core architecture. Experimental evidence on an STM32F476
    // shows that setting it in IRQconfigureMPU for the internal RAM region
    // causes the boot to fail.
    // For this reason, all regions are marked as not shareable
    MPU->RBAR=(base & (~0x1f)) | MPU_RBAR_VALID_Msk | region;
    MPU->RASR=(executePermitted ? 0 : MPU_RASR_XN_Msk)
             | 1<<MPU_RASR_AP_Pos //Privileged: RW, unprivileged: no access
             | MPU_RASR_C_Msk     //Normal, outer/inner write through, no write alloc
             | 1                  //Enable bit
             | sizeToMpu(size)<<1;
    #endif
}

void IRQenableMPU()
{
    #if __CORTEX_M == 33U
    // ARMv8-M MPU attributes are stored in separate registers, indexed in RLAR
    MPU->MAIR0 = (0xaa << 0); // Normal, outer/inner write through, no write alloc
    MPU->MAIR1 = 0;
    #endif

    // NOTE: using regions starting from 0 for the kernel because in the ARM MPU
    // in case of overlapping regions the one with the highest number takes
    // priority. The lower regions used by the kernel by default forbid access
    // to unprivileged code, while the higher numbered ones are used by processes
    // to override the default deny policy for the process-specific memory.
    IRQconfigureMPURegion(0,0x00000000,0x20000000,true);
    IRQconfigureMPURegion(1,0x20000000,0x20000000,false);

    //These are defined in the linker script
    extern unsigned char _xram_start asm("_xram_start");
    extern unsigned char _xram_size asm("_xram_size");
    unsigned int xramBase=reinterpret_cast<unsigned int>(&_xram_start);
    unsigned int xramSize=reinterpret_cast<unsigned int>(&_xram_size);

    #ifdef __CODE_IN_XRAM
    bool allowCodeInXram=true;
    #else //__CODE_IN_XRAM
    bool allowCodeInXram=false;
    #endif //__CODE_IN_XRAM
    if(xramSize) IRQconfigureMPURegion(2,xramBase,xramSize,allowCodeInXram);

    //After configuring the MPU, enable it
    MPU->CTRL = MPU_CTRL_HFNMIENA_Msk
              | MPU_CTRL_PRIVDEFENA_Msk
              | MPU_CTRL_ENABLE_Msk;
    
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT==1)
    SCB_EnableICache();
    SCB_EnableDCache();
    #endif
}

#if __CORTEX_M != 33U
unsigned int sizeToMpu(unsigned int size)
{
    if(extraChecks!=ExtraChecks::None && size<32) errorHandler(Error::UNEXPECTED);
    unsigned int result=30-__builtin_clz(size);
    if(size & (size-1)) result++;
    return result;
}
#endif

//
// class KernelspaceMpuConfiguration
//

KernelspaceMpuConfiguration::KernelspaceMpuConfiguration()
{
    for(int i=0;i<3;i++)
    {
        MPU->RNR=i;
        regValues[2*i]=MPU->RBAR;
        #if __CORTEX_M == 33U
        regValues[2*i+1]=MPU->RLAR;
        #else
        regValues[2*i+1]=MPU->RASR;
        #endif
    }
}

void KernelspaceMpuConfiguration::apply()
{
    for(int i=0;i<3;i++)
    {
        //NOTE: when reading RBAR in ARMv7-M/v6-M, VALID reads as 0 so either we
        //set this bit back or we need to also update RNR, which we do
        MPU->RNR=i;
        MPU->RBAR=regValues[2*i];
        #if __CORTEX_M == 33U
        MPU->RLAR=regValues[2*i+1];
        #else
        MPU->RASR=regValues[2*i+1];
        #endif
    }
}

} //namespace miosix

#endif //__MPU_PRESENT==1
