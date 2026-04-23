/***************************************************************************
 *   Copyright (C) 2008-2026 by Terraneo Federico                          *
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

#ifndef COMPILING_MIOSIX
#error "This is header is private, it can't be used outside Miosix itself."
#error "If your code depends on a private header, it IS broken."
#endif //COMPILING_MIOSIX

#include "interfaces/arch_registers.h" //For __CORTEX_M
#include "thread.h"

namespace miosix {

//These are defined in thread.cpp
extern volatile Thread *runningThreads[CPU_NUM_CORES];

/**
 * \internal
 * To be used in interrupts where a context switch can occur to check if the
 * stack of the thread being preempted has overflowed.
 * Note that since Miosix 3 all peripheral interrupts no longer perform a
 * full context save/restore thus you cannot call this functions from such
 * interrupts.
 *
 * If the overflow check failed for a kernel thread or a thread running in
 * kernelspace this function causes a reboot. On a platform with processes
 * this function calls IRQreportFault() if the stack overflow happened in
 * userspace, causing the process to segfault.
 */
inline void IRQstackOverflowCheck()
{
    //CortexM33 has hardware stackoverflow checking, no need to check in software
    #if !defined(__CORTEX_M) || __CORTEX_M != 33U
    Thread *cur=const_cast<Thread*>(runningThreads[getCurrentCoreId()]);
    const unsigned int watermarkSize=WATERMARK_LEN/sizeof(unsigned int);
    #ifdef WITH_PROCESSES
    if(cur->flags.isInUserspace())
    {
        bool overflow=false;
        if(cur->userCtxsave[STACK_OFFSET_IN_CTXSAVE] <
            reinterpret_cast<unsigned int>(cur->userWatermark+watermarkSize))
            overflow=true;
        if(overflow==false)
            for(unsigned int i=0;i<watermarkSize;i++)
                if(cur->userWatermark[i]!=WATERMARK_FILL) overflow=true;
        if(overflow) IRQreportFault(FaultData(fault::STACKOVERFLOW));
    }
    #endif //WITH_PROCESSES
    if(cur->ctxsave[STACK_OFFSET_IN_CTXSAVE] <
        reinterpret_cast<unsigned int>(cur->watermark+watermarkSize))
        errorHandler(Error::STACK_OVERFLOW);
    for(unsigned int i=0;i<watermarkSize;i++)
        if(cur->watermark[i]!=WATERMARK_FILL) errorHandler(Error::STACK_OVERFLOW);
    #endif //!defined(__CORTEX_M) || __CORTEX_M != 33U
}

} //namespace miosix
