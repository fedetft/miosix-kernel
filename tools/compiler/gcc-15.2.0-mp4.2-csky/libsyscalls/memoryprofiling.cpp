/***************************************************************************
 *   Copyright (C) 2008 - 2024 by Terraneo Federico                        *
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

#include "memoryprofiling.h"
#include <cstdio>
#include <malloc.h>
#include <reent.h>
#include <unistd.h>

const int miosixCustomSysconfBase = 100000;
const int watermarkLen   = miosixCustomSysconfBase+0;
const int stackFill      = miosixCustomSysconfBase+1;
const int ctxsaveOnStack = miosixCustomSysconfBase+2;

// declared in crt1.cpp
extern const char *__processHeapEnd;
extern const char *__processStackEnd;
extern const char *__maxUsedHeap;

namespace miosix {

//TODO: when processes can spawn threads we need the per-thread stack bottom
static const unsigned int *getStackBottom()
{
    return reinterpret_cast<const unsigned int *>(__processHeapEnd)+sysconf(watermarkLen);
}

//
// MemoryProfiling class
//

void MemoryProfiling::print()
{
    unsigned int curFreeStack=getCurrentFreeStack();
    unsigned int absFreeStack=getAbsoluteFreeStack();
    unsigned int stackSize=getStackSize();
    unsigned int curFreeHeap=getCurrentFreeHeap();
    unsigned int absFreeHeap=getAbsoluteFreeHeap();
    unsigned int heapSize=getHeapSize();

    iprintf("Stack memory statistics.\n"
            "Size: %u\n"
            "Used (current/max): %u/%u\n"
            "Free (current/min): %u/%u\n"
            "Heap memory statistics.\n"
            "Size: %u\n"
            "Used (current/max): %u/%u\n"
            "Free (current/min): %u/%u\n",
            stackSize,stackSize-curFreeStack,stackSize-absFreeStack,
            curFreeStack,absFreeStack,
            heapSize,heapSize-curFreeHeap,heapSize-absFreeHeap,
            curFreeHeap,absFreeHeap);
}

unsigned int MemoryProfiling::getStackSize()
{
    //TODO: when processes can spawn threads we need the per-thread stack size
    return __processStackEnd-__processHeapEnd-sysconf(watermarkLen);
}

unsigned int MemoryProfiling::getAbsoluteFreeStack()
{
    const unsigned int stackOccupiedByCtxsave=sysconf(ctxsaveOnStack);
    const unsigned int fillByte=sysconf(stackFill);
    const unsigned int fillWord=fillByte | fillByte<<8 | fillByte<<16 | fillByte<<24;
    const unsigned int *walk=getStackBottom();
    const unsigned int stackSize=getStackSize();
    unsigned int count=0;
    while(count<stackSize && *walk==fillWord)
    {
        //Count unused stack
        walk++;
        count+=4;
    }
    //This takes in account CTXSAVE_ON_STACK. It might underestimate
    //the absolute free stack (by a maximum of CTXSAVE_ON_STACK) but
    //it will never overestimate it, which is important since this
    //member function can be used to select stack sizes.
    if(count<=stackOccupiedByCtxsave) return 0;
    return count-stackOccupiedByCtxsave;
}

unsigned int MemoryProfiling::getCurrentFreeStack()
{
    unsigned int stackOccupiedByCtxsave=sysconf(ctxsaveOnStack);
    #ifdef __ARM_EABI__
    void *stack_ptr;
    asm volatile("mov %0, sp" : "=r"(stack_ptr));
    #else //__ARM_EABI__
    //NOTE: __builtin_frame_address doesn't add the size of the current function
    //frame, likely __builtin_stack_address would be better but GCC 9.2.0 does
    //not have it
    void *stack_ptr=__builtin_frame_address(0);
    #endif //__ARM_EABI__
    const unsigned int *walk=getStackBottom();
    unsigned int freeStack=(reinterpret_cast<unsigned int>(stack_ptr)
                          - reinterpret_cast<unsigned int>(walk));
    //This takes into account CTXSAVE_ON_STACK.
    if(freeStack<=stackOccupiedByCtxsave) return 0;
    return freeStack-stackOccupiedByCtxsave;
}

unsigned int MemoryProfiling::getHeapSize()
{
    extern char _end asm("_end"); //defined in the linker script
    return reinterpret_cast<unsigned int>(__processHeapEnd)
         - reinterpret_cast<unsigned int>(&_end);
}

unsigned int MemoryProfiling::getAbsoluteFreeHeap()
{
    //Make sure __maxUsedHeap is initialized
    _sbrk_r(__getreent(),0);

    return reinterpret_cast<unsigned int>(__processHeapEnd)
         - reinterpret_cast<unsigned int>(__maxUsedHeap);
}

unsigned int MemoryProfiling::getCurrentFreeHeap()
{
    struct mallinfo mallocData=_mallinfo_r(__getreent());
    return getHeapSize()-mallocData.uordblks;
}

} //namespace miosix
