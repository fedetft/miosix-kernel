/***************************************************************************
 *   Copyright (C) 2008-2024 by Terraneo Federico                          *
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

#include "interfaces_private/cpu.h"

namespace miosix {

void initKernelThreadCtxsave(unsigned int *ctxsave, void (*pc)(void *(*)(void*),void*),
                             unsigned int *sp, unsigned int *spLimit,
                             void *(*arg0)(void*), void *arg1) noexcept
{
    (void)spLimit;
    ctxsave[0]=reinterpret_cast<unsigned int>(arg0);
    ctxsave[1]=reinterpret_cast<unsigned int>(arg1);
    ctxsave[2]=0;
    ctxsave[3]=0;
    ctxsave[4]=0;
    ctxsave[5]=0;
    ctxsave[6]=0;
    ctxsave[7]=0;
    ctxsave[8]=0;
    ctxsave[9]=0;
    ctxsave[10]=0;
    ctxsave[11]=0;
    ctxsave[12]=0;
    ctxsave[13]=reinterpret_cast<unsigned int>(sp);//Thread stack pointer
    ctxsave[14]=0xffffffff;//threadLauncher never returns, so lr is not important
    ctxsave[15]=reinterpret_cast<unsigned int>(pc);//Thread pc=entry point
    ctxsave[16]=0x1f;//thread starts in system mode with IRQ and FIQ enabled.
}

void IRQportableStartKernel() noexcept
{
    //create a temporary space to save current registers. This data is useless
    //since there's no way to stop the sheduler, but we need to save it anyway.
    unsigned int s_ctxsave[miosix::CTXSAVE_SIZE];
    ctxsave[getCurrentCoreId()]=s_ctxsave;//make global ctxsave point to it
    IRQinvokeScheduler();
    //Can't use fastEnableIrq() since we want to enable both IRQ and FIQ.
    //After this point boot is complete and Miosix never disables FIQ again
    asm volatile("mrs r0, cpsr       \n\t"
                 "and r0, r0, #~0xc0 \n\t"
                 "msr cpsr_c, r0     \n\t"
                 :::"r0", "memory");
    //Never reaches here
}

} //namespace miosix
