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

#include "mpu_cortexMx.h"
#include "kernel/error.h"
#include <cstdio>
#include <cstring>
#include <cassert>

using namespace std;

namespace miosix {

unsigned int sizeToMpu(unsigned int size)
{
    assert(size>=32);
    unsigned int result=30-__builtin_clz(size);
    if(size & (size-1)) result++;
    return result;
}

#ifdef WITH_PROCESSES

//
// class MPUConfiguration
//

MPUConfiguration::MPUConfiguration(const unsigned int *elfBase, unsigned int elfSize,
        const unsigned int *imageBase, unsigned int imageSize)
{
    #if __MPU_PRESENT==1
    // NOTE: The ARM documentation is unclear about the effect of the shareable
    // bit on a single core architecture. Experimental evidence on an STM32F476
    // shows that setting it in IRQconfigureCache for the internal RAM region
    // causes the boot to fail.
    // For this reason, all regions are marked as not shareable
    regValues[0]=(reinterpret_cast<unsigned int>(elfBase) & (~0x1f))
               | MPU_RBAR_VALID_Msk | 6; //Region 6
    regValues[2]=(reinterpret_cast<unsigned int>(imageBase) & (~0x1f))
               | MPU_RBAR_VALID_Msk | 7; //Region 7
    regValues[1]=2<<MPU_RASR_AP_Pos //Privileged: RW, unprivileged: RO
               | MPU_RASR_C_Msk
               | 1 //Enable bit
               | sizeToMpu(elfSize)<<1;
    regValues[3]=3<<MPU_RASR_AP_Pos //Privileged: RW, unprivileged: RW
               | MPU_RASR_XN_Msk
               | MPU_RASR_C_Msk
               | 1 //Enable bit
               | sizeToMpu(imageSize)<<1;
    #else //__MPU_PRESENT==1
    #warning architecture lacks MPU, memory protection for processes unsupported
    //Although we have no MPU, store enough information to still enable checking
    //syscall parameters in withinForReading()/withinForWriting()
    regValues[0]=(reinterpret_cast<unsigned int>(elfBase) & (~0x1f));
    regValues[2]=(reinterpret_cast<unsigned int>(imageBase) & (~0x1f));
    regValues[1]=sizeToMpu(elfSize)<<1;
    regValues[3]=sizeToMpu(imageSize)<<1;
    #endif //__MPU_PRESENT==1
}

void MPUConfiguration::dumpConfiguration()
{
    #if __MPU_PRESENT==1
    for(int i=0;i<2;i++)
    {
        unsigned int base=regValues[2*i] & (~0x1f);
        unsigned int end=base+(1<<(((regValues[2*i+1]>>1) & 31)+1));
        char w=regValues[2*i+1] & (1<<MPU_RASR_AP_Pos) ? 'w' : '-';
        char x=regValues[2*i+1] & MPU_RASR_XN_Msk ? '-' : 'x';
        iprintf("* MPU region %d 0x%08x-0x%08x r%c%c\n",i+6,base,end,w,x);
    }
    #else //__MPU_PRESENT==1
    iprintf("* Architecture lacks MPU\n");
    for(int i=0;i<2;i++)
    {
        unsigned int base=regValues[2*i] & (~0x1f);
        unsigned int end=base+(1<<(((regValues[2*i+1]>>1) & 31)+1));
        iprintf("* MPU region %d 0x%08x-0x%08x rwx\n",i+6,base,end);
    }
    #endif //__MPU_PRESENT==1
}

unsigned int MPUConfiguration::roundSizeForMPU(unsigned int size)
{
    return 1<<(sizeToMpu(size)+1);
}

pair<const unsigned int*, unsigned int> MPUConfiguration::roundRegionForMPU(
    const unsigned int *ptr, unsigned int size)
{
    constexpr unsigned int maxSize=0x80000000;
    //NOTE: worst case is p=2147483632 size=32, a memory block in the middle of
    //the 2GB mark. To meet the constraint of returning a pointer aligned to its
    //size we would need to return 0 as pointer, and 4GB as size, the whole
    //address space. This is however not possible as 4GB does not fit in an
    //unsigned int and would overflow size. To prevent an infinite loop, the
    //returned aligned size is limited to less than 2GB, thereby preventing
    //cases when the algorithm would reach 2GB and try to increase it further.
    //Note that the algorithm would fail not only in the impossible worst case
    //but also in cases where a 2GB size would be enough. This is not a concern
    //as in practical applications memory blocks are within the microcontroller
    //flash memory, and the memory is always aligned to its size, so the maximum
    //aligned block this algorithm would return is just the entire flash memory,
    //whose size is far less than 2GB in modern microcontrollers.
    unsigned int p=reinterpret_cast<unsigned int>(ptr);
    unsigned int x=roundSizeForMPU(size);
    for(;;)
    {
        if(x>=maxSize) errorHandler(UNEXPECTED);
        unsigned int ap=p & (~(x-1));
        unsigned int addsz=p-ap;
        unsigned int y=roundSizeForMPU(size+addsz);
        //iprintf("ap=%u addsz=%u x=%u y=%u\n",ap,addsz,x,y);
        if(y==x) return {reinterpret_cast<const unsigned int*>(ap),x};
        x=y;
    }
}

bool MPUConfiguration::withinForReading(const void *ptr, size_t size) const
{
    size_t codeStart=regValues[0] & (~0x1f);
    size_t codeEnd=codeStart+(1<<(((regValues[1]>>1) & 31)+1));
    size_t dataStart=regValues[2] & (~0x1f);
    size_t dataEnd=dataStart+(1<<(((regValues[3]>>1) & 31)+1));
    size_t base=reinterpret_cast<size_t>(ptr);
    //The last check is to prevent a wraparound to be considered valid
    return (   (base>=codeStart && base+size<codeEnd)
            || (base>=dataStart && base+size<dataEnd)) && base+size>=base;
}

bool MPUConfiguration::withinForWriting(const void *ptr, size_t size) const
{
    size_t dataStart=regValues[2] & (~0x1f);
    size_t dataEnd=dataStart+(1<<(((regValues[3]>>1) & 31)+1));
    size_t base=reinterpret_cast<size_t>(ptr);
    //The last check is to prevent a wraparound to be considered valid
    return base>=dataStart && base+size<dataEnd && base+size>=base;
}

bool MPUConfiguration::withinForReading(const char* str) const
{
    size_t codeStart=regValues[0] & (~0x1f);
    size_t codeEnd=codeStart+(1<<(((regValues[1]>>1) & 31)+1));
    size_t dataStart=regValues[2] & (~0x1f);
    size_t dataEnd=dataStart+(1<<(((regValues[3]>>1) & 31)+1));
    size_t base=reinterpret_cast<size_t>(str);
    if((base>=codeStart) && (base<codeEnd))
        return strnlen(str,codeEnd-base)<codeEnd-base;
    if((base>=dataStart) && (base<dataEnd))
        return strnlen(str,dataEnd-base)<dataEnd-base;
    return false;
}

#endif //WITH_PROCESSES

} //namespace miosix
