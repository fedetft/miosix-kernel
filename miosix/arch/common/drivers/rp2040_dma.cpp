/***************************************************************************
 *   Copyright (C) 2025 by Daniele Cattaneo                                *
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

#include "rp2040_dma.h"
#include "kernel/error.h"
#include "interfaces/interrupts.h"

namespace miosix {

bool RP2040Dma::initialized=false;
unsigned int RP2040Dma::irqAllocMask = 0;
RP2040Dma::DmaIrqEntry RP2040Dma::irqEntries[RP2040Dma::numChannels];

unsigned int RP2040Dma::IRQregisterChannel(GlobalIrqLock& lock,
        void (*handler)(void*), void *arg) noexcept
{
    if(!initialized) IRQinitialize(lock);
    unsigned int channel=0, chMask=1;
    while(channel<numChannels)
    {
        if((irqAllocMask&chMask)==0) break;
        channel++; chMask<<=1;
    }
    // Out of DMA channels?
    if(channel>=numChannels) errorHandler(Error::UNEXPECTED);

    irqAllocMask|=chMask;
    irqEntries[channel].handler=handler;
    irqEntries[channel].arg=arg;
    if(channel%2==0) dma_hw->inte0|=chMask;
    else dma_hw->inte1|=chMask;
    return channel;
}

void RP2040Dma::IRQunregisterChannel(GlobalIrqLock& lock, unsigned int channel,
        void (*handler)(void*), void *arg) noexcept
{
    unsigned int chMask=1U<<channel;
    if((irqAllocMask&chMask)==0) errorHandler(Error::UNEXPECTED);
    if(irqEntries[channel].handler!=handler) errorHandler(Error::UNEXPECTED);
    if(irqEntries[channel].arg!=arg) errorHandler(Error::UNEXPECTED);

    if((channel%2)==0) dma_hw->inte0&=~chMask;
    else dma_hw->inte1&=~chMask;
    irqAllocMask&=~chMask;
    irqEntries[channel].handler=nullptr;
    irqEntries[channel].arg=0;
}

void RP2040Dma::IRQinitialize(GlobalIrqLock& lock) noexcept
{
    clocks_hw->wake_en0|=CLOCKS_WAKE_EN0_CLK_SYS_DMA_BITS;
    clocks_hw->sleep_en0|=CLOCKS_SLEEP_EN0_CLK_SYS_DMA_BITS;
    unreset_block_wait(RESETS_RESET_DMA_BITS);
    IRQregisterIrq(lock,DMA_IRQ_0_IRQn,&IRQinterruptHandler<0>);
    IRQregisterIrq(lock,DMA_IRQ_1_IRQn,&IRQinterruptHandler<1>);
    initialized=true;
}

template<unsigned int IrqId>
void RP2040Dma::IRQinterruptHandler() noexcept
{
    FastGlobalLockFromIrq lock;
    io_rw_32 *status;
    if(IrqId==0) status=&dma_hw->ints0;
    else status=&dma_hw->ints1;

    unsigned int ch=0, chMask=1;
    unsigned int curStatus=*status;
    while(ch<numChannels && curStatus)
    {
        if(curStatus & chMask)
        {
            *status=chMask;
            if(irqAllocMask & chMask)
            {
                irqEntries[ch].handler(irqEntries[ch].arg);
            }
        }
        curStatus=*status;
        ch++; chMask<<=1;
    }
}

} // namespace miosix
