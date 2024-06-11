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
#include <config/miosix_settings.h>

// declared in crt1.cpp
extern const char *__processHeapEnd;
extern const char *__processStackEnd;
extern const char *__maxUsedHeap;

namespace miosix {

//TODO: when processes can spawn threads we need the per-thread stack bottom
static const unsigned int *getStackBottom()
{
    return reinterpret_cast<const unsigned int *>(__processHeapEnd)+WATERMARK_LEN;
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
    return __processStackEnd-__processHeapEnd-WATERMARK_LEN;
}

unsigned int MemoryProfiling::getAbsoluteFreeStack()
{
    const unsigned int *walk=getStackBottom();
    const unsigned int stackSize=getStackSize();
    unsigned int count=0;
    while(count<stackSize && *walk==STACK_FILL)
    {
        //Count unused stack
        walk++;
        count+=4;
    }
    //This takes in account CTXSAVE_ON_STACK. It might underestimate
    //the absolute free stack (by a maximum of CTXSAVE_ON_STACK) but
    //it will never overestimate it, which is important since this
    //member function can be used to select stack sizes.
    if(count<=CTXSAVE_ON_STACK) return 0;
    return count-CTXSAVE_ON_STACK;
}

unsigned int MemoryProfiling::getCurrentFreeStack()
{
    register int *stack_ptr asm("sp");
    const unsigned int *walk=getStackBottom();
    unsigned int freeStack=(reinterpret_cast<unsigned int>(stack_ptr)
                          - reinterpret_cast<unsigned int>(walk));
    //This takes into account CTXSAVE_ON_STACK.
    if(freeStack<=CTXSAVE_ON_STACK) return 0;
    return freeStack-CTXSAVE_ON_STACK;
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
