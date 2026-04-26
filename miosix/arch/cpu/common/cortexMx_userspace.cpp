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

#include "interfaces_private/userspace.h"
#include "interfaces/cpu_const.h"
#include "kernel/error.h"
#include <cstdio>
#include <cstring>
#include <bit>
//Only include if needed to prevent printing multiple times the no MPU warning
#if __MPU_PRESENT==1
#include "cortexMx_mpu.h"
#endif //__MPU_PRESENT==1

using namespace std;

namespace miosix {

#ifdef WITH_PROCESSES

// NOTE: workaround for weird compiler bug. See header file for an explanation.
#ifndef __OPTIMIZE__
void __attribute__((naked,used)) portableSwitchToUserspace()
{
    // It gets worse, the r7 error pops up at random even when not inlining, the
    // function, so switch to a naked function
    asm volatile("push {r7, lr} \n\t"
                 "movs r7, #1   \n\t"
                 "svc  0        \n\t"
                 "pop  {r7, pc} \n\t");
}
#endif //__OPTIMIZE__

void initUserThreadCtxsave(unsigned int *ctxsave, unsigned int pc, int argc,
    void *argvSp, void *envp, unsigned int *gotBase, unsigned int *heapEnd)
{
    unsigned int *stackPtr=reinterpret_cast<unsigned int*>(argvSp);
    //Stack is full descending, so decrement first
    stackPtr--; *stackPtr=0x01000000;                                 //--> xPSR
    stackPtr--; *stackPtr=pc;                                         //--> pc
    stackPtr--; *stackPtr=0xffffffff;                                 //--> lr
    stackPtr--; *stackPtr=0;                                          //--> r12
    stackPtr--; *stackPtr=reinterpret_cast<unsigned int>(heapEnd);    //--> r3
    stackPtr--; *stackPtr=reinterpret_cast<unsigned int>(envp);       //--> r2
    stackPtr--; *stackPtr=reinterpret_cast<unsigned int>(argvSp);     //--> r1
    stackPtr--; *stackPtr=argc;                                       //--> r0

    ctxsave[0]=reinterpret_cast<unsigned int>(stackPtr);              //--> psp
    ctxsave[6]=reinterpret_cast<unsigned int>(gotBase);               //--> r9
    //leaving the content of r4-r8,r10-r11 uninitialized
    #if __FPU_PRESENT==1
    //NOTE: only armv7m with fpu has lr in ctxsave
    ctxsave[9]=0xfffffffd; //EXC_RETURN=thread mode, use psp, no floating ops
    //leaving the content of s16-s31 uninitialized
    #endif //__FPU_PRESENT==1
}

//
// class FaultData
//

void FaultData::print() const
{
    using namespace fault;
    switch(id)
    {
        case NONE: break;
        case STACKOVERFLOW:
            iprintf("* Stack overflow\n");
            break;
        case MP:
            iprintf("* Attempted data access @ 0x%x",arg);
            break;
        case MP_NOADDR:
            iprintf("* Invalid data access");
            break;
        case MP_XN:
            iprintf("* Attempted instruction fetch");
            break;
        case MP_STACK:
            iprintf("* Invalid SP (0x%x) while entering IRQ",arg);
            break;
        case UF_DIVZERO:
            iprintf("* Divide by zero");
            break;
        case UF_UNALIGNED:
            iprintf("* Unaligned memory access");
            break;
        case UF_COPROC:
            iprintf("* Attempted coprocessor access");
            break;
        case UF_EXCRET:
            iprintf("* Invalid exception return sequence");
            break;
        case UF_EPSR:
            iprintf("* Attempted access to the EPSR");
            break;
        case UF_UNDEF:
            iprintf("* Undefined instruction");
            break;
        case UF_UNEXP:
            iprintf("* Unexpected usage fault");
            break;
        case HARDFAULT:
            iprintf("* Hardfault");
            break;
        case BF:
            iprintf("* Busfault @ 0x%x",arg);
            break;
        case BF_NOADDR:
            iprintf("* Busfault");
            break;
    }
    if(id!=NONE && id!=STACKOVERFLOW)
    {
        //NOTE: if the fault really occurs at the address 0xbadadd we'll
        //spuriously say the stack is corrupted...
        if(pc==0xbadadd) iprintf(" (Missing PC, corrupted stack)\n");
        else iprintf(" PC=0x%x\n",pc);
    }
}

void FaultData::IRQtryAddProgramCounter(unsigned int *userCtxsave,
    const MPUConfiguration& mpu)
{
    // ARM Cortex CPUs push the program counter on the stack. Thus we retrieve
    // the stack pointer from userCtxsave and check it is not corrupted.
    // We use mpu.withinForWriting even though the we're only reading from the
    // stack frame since the stack pointer can't point to read-only memory, if
    // it does it's corrupted. If the pointer checks out, we get the desired pc
    const int pcOffsetInStackFrame=6;
    auto *sp=reinterpret_cast<unsigned int*>(userCtxsave[STACK_OFFSET_IN_CTXSAVE]);
    // NOTE: we check that (pcOffsetInStackFrame+1)*4 words are within the
    // process address space instead of CTXSAVE_ON_STACK since in CPUs with
    // FPU until a process uses FPU registers they are not saved, so it is
    // possible that sp+CTXSAVE_ON_STACK coud overflow the process address space
    if(mpu.withinForWriting(sp,(pcOffsetInStackFrame+1)*sizeof(unsigned int*)))
        pc=sp[pcOffsetInStackFrame];
}

//
// class MPUConfiguration
//

/**
 * \internal
 * Decode the boundaries of an MPU region for a given CPU
 * \param reg0 First register encoding the region information (RBAR)
 * \param reg1 Second register encoding the region information (RASR or RLAR)
 * \return a tuple with the start and end address
 */
static tuple<size_t, size_t> decodeMpuRegion(unsigned int reg0, unsigned int reg1)
{
    #if __MPU_PRESENT==1
    #if __CORTEX_M == 33
    size_t regionStart=reg0 & (~0x1f);
    size_t regionEnd=(reg1 | 0x1f)+1;
    #else
    size_t regionStart=reg0 & (~0x1f);
    size_t regionEnd=regionStart+(1<<(((reg1>>1) & 31)+1));
    #endif
    #else //__MPU_PRESENT==1
    size_t regionStart=reg0;
    size_t regionEnd=reg1;
    #endif //__MPU_PRESENT==1
    return {regionStart,regionEnd};
}

#if defined(__CORTEX_M) && __CORTEX_M == 33 && __MPU_PRESENT==1
/*
 * Since ARM made the stupid design decision to disallow MPU regions to overlap
 * starting from ARMv8, implementing processes just got harder and more
 * inefficient.
 * Miosix uses the MPU also in kernel code to achieve kernel-level W^X and to
 * override the default cacheability policy, thus we need to cover each existing
 * memory region with an MPU region for the kernel. These regions diallow
 * nonprivileged access. On top of that, we need to overlap two regions where
 * unprivileged access is allowed, that need to be dynamically changed at every
 * context switch to map to the currently running process.
 * How do we do it now that overlapping regions is no longer possible?
 * Here's how: by juggling MPU regions as best as we can
 *
 *  Running   Running   Running   Running
 *  kernel    process   process   process
 *            from XIP  no XIP    no XIP (code allocated after data)
 * +-------+ +-------+ +-------+ +-------+
 * |   6   | |   6   | |   6   | |   6   | SRAM (or XRAM), when both exist
 * +-------+ +-------+ +-------+ +-------+
 *
 * +-------+ +-------+ +-------+ +-------+
 * |       | |       | |   5   | |   5   |
 * |       | |   5   | +-------+ +-------+
 * |       | |       | |  [4]  | |  [1]  |
 * |       | +-------+ +-------+ +-------+
 * |   1   | |  [4]  | |   3   | |   3   | XRAM (or SRAM when there's no XRAM)
 * |       | +-------+ +-------+ +-------+
 * |       | |       | |  [1]  | |  [4]  |
 * |       | |   3   | +-------+ +-------+
 * |       | |       | |   2   | |   2   |
 * +-------+ +-------+ +-------+ +-------+
 *
 * +-------+ +-------+ +-------+ +-------+
 * |       | |   2   | |       | |       |
 * |       | +-------+ |       | |       |
 * |   0   | |  [1]  | |   0   | |   0   | FLASH
 * |       | +-------+ |       | |       |
 * |       | |   0   | |       | |       |
 * +-------+ +-------+ +-------+ +-------+
 *
 * We assume the kernel has access to either two or three noncontiguous memory
 * regions. The first (bottom) is the FLASH memory, which contains the kernel
 * and the XIP filesystem. The second (middle) is the one that contains the
 * process pool, the RAM allocator for processes. This region can either contain
 * *only* the process pool, or contain *both* kernel data and the process pool.
 * The former is a common configuration when an external RAM (XRAM) is present
 * and whoever ported the board to Miosix decided to add a linker script
 * dedicating the entire XRAM for processes.
 * The latter is a common configuration when there is no XRAM and thus the
 * internal SRAM has to be partitioned among the kernel and processes. However
 * these are just examples, there is a lot of freedom in how board designers
 * can make linker scripts for Miosix.
 * The third memory region, if present, is the RAM memory where the process
 * pool is *not* mapped, which again is commonly the SRAM in boards where an
 * XRAM is present. It ususally contains all kernel data as the entire kernel
 * data can easily fit in a small internal SRAM.
 * Somewhat confusingly, region 6 can either be *above* or *below* te other
 * RAM region (i.e, at higher or lower addresses). What matters is that region
 * 6 is the RAM wihtout the process pool, wherever that happens to be.
 *
 * Numbers in the figure are the used MPU regions for the three possible memory
 * layouts. Square brackets indicate a region where nonprivileged acces is
 * allowed. For all process memory layouts, region 1 is always the process code,
 * while region 4 is always the process data.
 * The left layout is the one when the kernel (i.e: a kernel thread) is running.
 * This is how the kernel boots, the configuration is done in cortexMx_mpu.cpp
 * and a copy of the configuration for regions 0 and 1 is stored in the variable
 * kernelspaceMpuConfiguration as we'll need to restore this layout at runtime
 * every time we switch back to the kernel.
 * The middle left layout is the one when a process is running whose code is in
 * the XIP filesystem, thus running from FLASH. Since regions cannot overlap, we
 * need to split the FLASH and the RAM that contains the process pool in three
 * regions, the one before the process, the one for the process, and the one
 * after it. The top or bottom regions may also be empty.
 * The middle right layout is the one when a process is running whose code is in
 * the process pool. Note that even though both the code and data are in the
 * pool, the kernel uses two noncontiguous regions anyway, both to make better
 * use of the available RAM (two smaller blocks may fit where a single larger
 * block may not), but also to enforce W^X, as well as to share the code region
 * in case more processes are spawned with the same program.
 * This memory layout swaps regions 1 and 2 so that regardless of whether the
 * process is running from XIP or not, the regions where unprivileged access
 * is allowed are always 1 (code) and 4 (data), which simplifies checking
 * syscall parameters.
 * The right layout exists because the process pool allocator makes no guarantee
 * to allocate the process code at lower addresses then the process data.
 * In that case, region 1 and 4 are swapped so that region 1 is always the code
 * and region 4 always the data.
 *
 * The cost of this design, which unfortunately is the only possible one now
 * that overlapping regions are no longer possible, is that there are 6 regions
 * (0 to 5) that need to be switched at runtime at every context switch.
 * Can ARM please undo the stupid design decision of forbidding regions overlap?
 */
static constexpr unsigned int codeIdx=2*1; ///<index into regValues of process code
static constexpr unsigned int dataIdx=2*4; ///<index into regValues of process data
#else //defined(__CORTEX_M) && __CORTEX_M == 33 && __MPU_PRESENT==1
/*
 * Thanks to the possibility to overlap MPU regions in ARMv6M/v7M, we use a
 * divided et impera approach to memory protection.
 * The code in cortexMx_mpu.cpp configures the kernel memory layout using
 * regions 0 to 2 (and on some chips also region 3). These regions are
 * used to achieve kernel-level W^X protection, configure cacheability and
 * to disallow unprivileged code access.
 * On top of those, the code in this file uses two regions, 6 and 7 which by
 * virtue of having a higher number take priority to overlay two regions
 * where unprivileged access is allowed, one for the process code and one for
 * the process data. Of course, W^X is enforced also for processes.
 * These two regions, 6 and 7, are the only regions that need changing at every
 * context switch to map to the memory arease of the currently running process.
 */
static constexpr unsigned int codeIdx=2*0; ///<index into regValues of process code
static constexpr unsigned int dataIdx=2*1; ///<index into regValues of process data
#endif //defined(__CORTEX_M) && __CORTEX_M == 33 && __MPU_PRESENT==1

MPUConfiguration::MPUConfiguration(const unsigned int *elfBase, unsigned int elfSize,
        const unsigned int *imageBase, unsigned int imageSize)
{
    const unsigned int codeStart=reinterpret_cast<unsigned int>(elfBase);
    const unsigned int codeEnd=codeStart+elfSize;
    const unsigned int dataStart=reinterpret_cast<unsigned int>(imageBase);
    const unsigned int dataEnd=dataStart+imageSize;
    #if __MPU_PRESENT==1
    #if __CORTEX_M == 33
    auto mpuRegion=[this](unsigned int region, unsigned int start, unsigned int end,
                      bool executePermitted, bool unprivilegedAccess)
    {
        if(start==end) //Corner case: if address range is empty disable region
        {
            regValues[2*region+0]=0; //RBAR=0
            regValues[2*region+1]=0; //RLAR=0, region disabled
            return;
        }
        unsigned int ap,xn,pxn;
        if(executePermitted==false && unprivilegedAccess==false)
        {
            ap  = 0b00<<1; //RW by privileged only
            xn  =    1<<0; //No execute
            pxn =    1<<4; //Privileged no execute
        } else if(executePermitted==true && unprivilegedAccess==false) {
            ap  = 0b10<<1; //RO by privileged only
            xn  =    0<<0; //Execute
            pxn =    0<<4; //Privileged execute
        } else if(executePermitted==false && unprivilegedAccess==true) {
            ap  = 0b01<<1; //RW by privileged and unprivileged
            xn  =    1<<0; //No execute
            pxn =    1<<4; //Privileged no execute
        } else if(executePermitted==true && unprivilegedAccess==true) {
            ap  = 0b11<<1; //RO by privileged and unprivileged
            xn  =    0<<0; //Execute
            pxn =    1<<4; //Privileged no execute
        }
        regValues[2*region+0]=(start   & (~0x1f)) | ap | xn; //RBAR
        regValues[2*region+1]=((end-1) & (~0x1f)) | pxn | 1; //RLAR
        //NOTE: AttrIndex set to 0, MAIR set in cortexMx_mpu.cpp

        // unsigned int rbase,rend;
        // tie(rbase,rend)=decodeMpuRegion(regValues[2*region],regValues[2*region+1]);
        // char w=regValues[2*region] & (0b10<<MPU_RBAR_AP_Pos) ? '-' : 'w';
        // char x=regValues[2*region] & MPU_RBAR_XN_Msk ? '-' : 'x';
        // char s=regValues[2*region] & (0b01<<MPU_RBAR_AP_Pos) ? '*' : ' ';
        // iprintf("* MPU region %d 0x%08x-0x%08x r%c%c%c\n",region,rbase,rend,w,x,s);
    };

    //The RAM region that contains the process pool can either be the SRAM or
    //XRAM, the decision has already been made in cortexMx_mpu.cpp
    unsigned int ramStart,ramEnd;
    tie(ramStart,ramEnd)=decodeMpuRegion(kernelspaceMpuConfiguration[2],
                                         kernelspaceMpuConfiguration[3]);
    if(codeStart<0x20000000)
    {
        //This process is has code in the XIP filesystem
        mpuRegion(0,0x00000000,codeStart, true, false);
        mpuRegion(1,codeStart, codeEnd,   true, true); //Region 1: code
        mpuRegion(2,codeEnd,   0x20000000,true, false);
        mpuRegion(3,ramStart,  dataStart, false,false);
        mpuRegion(4,dataStart, dataEnd,   false,true); //Region 4: data
        mpuRegion(5,dataEnd,   ramEnd,    false,false);
    } else {
        //This process is has code in the process pool,
        if(dataStart>codeStart)
        {
            //The code region has been allocated before the data region
            mpuRegion(0,0x00000000,0x20000000,true, false);
            mpuRegion(2,ramStart,  codeStart, false,false);
            mpuRegion(1,codeStart, codeEnd,   true, true); //Region 1: code
            mpuRegion(3,codeEnd,   dataStart, false,false);
            mpuRegion(4,dataStart, dataEnd,   false,true); //Region 4: data
            mpuRegion(5,dataEnd,   ramEnd,    false,false);
        } else {
            //The code region has been allocated after the data region
            mpuRegion(0,0x00000000,0x20000000,true, false);
            mpuRegion(2,ramStart,  dataStart, false,false);
            mpuRegion(4,dataStart, dataEnd,   false,true); //Region 4: data
            mpuRegion(3,dataEnd,   codeStart, false,false);
            mpuRegion(1,codeStart, codeEnd,   true, true); //Region 1: code
            mpuRegion(5,codeEnd,   ramEnd,    false,false);
        }
    }
    #else // __CORTEX_M == 33
    // NOTE: using regions 6 and 7 for processes because in the ARM MPU in case
    // of overlapping regions the one with the highest number takes priority.
    // The lower regions are used by the kernel and by default forbid access to
    // unprivileged code, thus we need higher numbered ones for processes to
    // override the default deny policy for the process-specific memory.
    // NOTE: The ARM documentation is unclear about the effect of the shareable
    // bit on a single core architecture. Experimental evidence on an STM32F476
    // shows that setting it in IRQconfigureMPU for the internal RAM region
    // causes the boot to fail.
    // For this reason, all regions are marked as not shareable
    (void)codeEnd;
    (void)dataEnd;
    regValues[0]=(codeStart & (~0x1f))
                | MPU_RBAR_VALID_Msk | 6; //Region 6
    regValues[1]=0b110<<MPU_RASR_AP_Pos   //Privileged: RO, unprivileged: RO
                | MPU_RASR_C_Msk          //Cacheable, write through
                | 1                       //Enable bit
                | sizeToMpu(elfSize)<<1;
    regValues[2]=(dataStart & (~0x1f))
                | MPU_RBAR_VALID_Msk | 7; //Region 7
    regValues[3]=0b011<<MPU_RASR_AP_Pos   //Privileged: RW, unprivileged: RW
                | MPU_RASR_XN_Msk         //Not executable
                | MPU_RASR_C_Msk          //Cacheable, write through
                | 1                       //Enable bit
                | sizeToMpu(imageSize)<<1;
    #endif
    #else //__MPU_PRESENT==1
    #warning Architecture does not provide an MPU, userspace memory protection will not be enforced
    //Although we have no MPU, store enough information to still enable checking
    //syscall parameters in withinForReading()/withinForWriting()
    regValues[0]=codeStart;
    regValues[1]=codeEnd;
    regValues[2]=dataStart;
    regValues[3]=dataEnd;
    #endif //__MPU_PRESENT==1
}

void MPUConfiguration::dumpConfiguration()
{
    for(int i=0;i<2;i++)
    {
        size_t base, end;
        const int region= i==0 ? codeIdx : dataIdx;
        tie(base,end)=decodeMpuRegion(regValues[region],regValues[region+1]);
        #if __MPU_PRESENT==1
        #if __CORTEX_M == 33
        char w=regValues[region] & (0b10<<MPU_RBAR_AP_Pos) ? '-' : 'w';
        char x=regValues[region] & MPU_RBAR_XN_Msk ? '-' : 'x';
        #else //__CORTEX_M == 33
        char w=regValues[region+1] & (1<<MPU_RASR_AP_Pos) ? 'w' : '-';
        char x=regValues[region+1] & MPU_RASR_XN_Msk ? '-' : 'x';
        #endif //__CORTEX_M == 33
        iprintf("* MPU region %d 0x%08x-0x%08x r%c%c\n",region/2,base,end,w,x);
        #else //__MPU_PRESENT==1
        iprintf("* Memory region %d 0x%08x-0x%08x rwx\n",i,base,end);
        #endif //__MPU_PRESENT==1
    }
}

tuple<const unsigned int*, unsigned int> MPUConfiguration::roundRegionForMPU(
    const unsigned int *ptr, unsigned int size)
{
    #if __MPU_PRESENT==1
    unsigned int p=reinterpret_cast<unsigned int>(ptr);
    #if __CORTEX_M == 33
    //ARMv8+ has no power of 2 size constraint, just align pointer to 32 byte
    //and make size a multiple of 32 bytes
    unsigned int ap=p & (~0x1f);
    size+=p-ap;
    return {reinterpret_cast<const unsigned int*>(ap), (size+0x1f) & (~0x1f)};
    #else //__CORTEX_M == 33
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
    unsigned int x=std::bit_ceil(size);
    for(;;)
    {
        if(x>=maxSize) errorHandler(Error::UNEXPECTED);
        unsigned int ap=p & (~(x-1));
        unsigned int addsz=p-ap;
        unsigned int y=std::bit_ceil(size+addsz);
        //iprintf("ap=%u addsz=%u x=%u y=%u\n",ap,addsz,x,y);
        if(y==x) return {reinterpret_cast<const unsigned int*>(ap),x};
        x=y;
    }
    #endif //__CORTEX_M == 33
    #else //__MPU_PRESENT==1
    return {ptr,size}; //No MPU, no rounding needed
    #endif //__MPU_PRESENT==1
}

bool MPUConfiguration::withinForReading(const void *ptr, size_t size) const
{
    size_t base=reinterpret_cast<size_t>(ptr);
    size_t codeStart,codeEnd,dataStart,dataEnd;
    tie(codeStart,codeEnd)=decodeMpuRegion(regValues[codeIdx],regValues[codeIdx+1]);
    tie(dataStart,dataEnd)=decodeMpuRegion(regValues[dataIdx],regValues[dataIdx+1]);
    //The last check is to prevent a wraparound to be considered valid
    return (   (base>=codeStart && base+size<=codeEnd)
            || (base>=dataStart && base+size<=dataEnd)) && base+size>=base;
}

bool MPUConfiguration::withinForWriting(const void *ptr, size_t size) const
{
    //Must be callable also with interrupts disabled,
    //used by FaultData::IRQtryAddProgramCounter()
    size_t base=reinterpret_cast<size_t>(ptr);
    size_t dataStart,dataEnd;
    tie(dataStart,dataEnd)=decodeMpuRegion(regValues[dataIdx],regValues[dataIdx+1]);
    //The last check is to prevent a wraparound to be considered valid
    return base>=dataStart && base+size<=dataEnd && base+size>=base;
}

bool MPUConfiguration::withinForReading(const char* str) const
{
    size_t base=reinterpret_cast<size_t>(str);
    size_t codeStart,codeEnd,dataStart,dataEnd;
    tie(codeStart,codeEnd)=decodeMpuRegion(regValues[codeIdx],regValues[codeIdx+1]);
    tie(dataStart,dataEnd)=decodeMpuRegion(regValues[dataIdx],regValues[dataIdx+1]);
    if((base>=codeStart) && (base<codeEnd))
        return strnlen(str,codeEnd-base)<codeEnd-base;
    if((base>=dataStart) && (base<dataEnd))
        return strnlen(str,dataEnd-base)<dataEnd-base;
    return false;
}

#if __CORTEX_M == 33
unsigned int MPUConfiguration::kernelspaceMpuConfiguration[4];
#endif //__CORTEX_M == 33

#endif //WITH_PROCESSES

} //namespace miosix
