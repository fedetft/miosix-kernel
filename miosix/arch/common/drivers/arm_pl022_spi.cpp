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

#include "config/miosix_settings.h"
#include "arm_pl022_spi.h"

namespace miosix {

void PL022Spi::initialize(unsigned int bitrate, bool spo, bool sph) noexcept
{
    if(irqn>=0)
    {
        GlobalIrqLock lock;
        IRQregisterIrq(lock,irqn,&PL022Spi::IRQhandleInterrupt,this);
    }
    spi->CR0=Regs::CR0_SPH().put(sph)
            |Regs::CR0_SPO().put(spo)
            |Regs::CR0_FRF().put(0) // Motorola frame format
            |Regs::CR0_DSS().put(7); // 8-bit word size
    setBitrate(bitrate);
    spi->CR1=Regs::CR1_SSE().put(1);
}

PL022Spi::~PL022Spi() noexcept
{
    GlobalIrqLock lock;
    spi->CR1=0;
    if(irqn>=0) IRQunregisterIrq(lock,irqn,&PL022Spi::IRQhandleInterrupt,this);
}

void PL022Spi::setBitrate(unsigned int bitrate) noexcept
{
    this->bitrate=bitrate;
    unsigned int ratio=peripheralClock/bitrate;
    if(ratio<2) ratio=2;
    if(ratio>0xfe00) errorHandler(Error::UNEXPECTED);
    unsigned int presc=2;
    while(ratio>presc*0x100) presc<<=1;
    if(presc>0xfe) presc=0xfe;
    unsigned int scr=(ratio/presc)-1;
    spi->CPSR=presc;
    spi->CR0=Regs::CR0_SCR().put(scr,spi->CR0);
}

void PL022Spi::IRQhandleInterrupt() noexcept
{
    FastGlobalLockFromIrq lock;
    spi->IMSC=0;
    if(waiting)
    {
        waiting->IRQwakeup();
        waiting=nullptr;
    }
}

void PL022Spi::waitForInterrupt(unsigned int flag) noexcept
{
    if(irqn>=0 && bitrate<10*1000*1000)
    {
        while(!(spi->RIS&flag))
        {
            FastGlobalIrqLock lock;
            waiting=Thread::IRQgetCurrentThread();
            spi->IMSC=flag;
            while(waiting) Thread::IRQglobalIrqUnlockAndWait(lock);
        }
    } else if(bitrate<10*1000*1000) {
        long long sleepns=1000000000LL/bitrate;
        while(!(spi->RIS&flag)) Thread::nanoSleep(sleepns);
    } else {
        while(!(spi->RIS&flag)) ;
    }
}

void PL022Spi::waitForEndTransfer() noexcept
{
    if(bitrate<10*1000*1000)
    {
        long long sleepns=1000000000LL/bitrate;
        while(Regs::SR_BSY().get(spi->SR)) Thread::nanoSleep(sleepns);
    } else {
        while(Regs::SR_BSY().get(spi->SR)) ;
    }
}

void PL022Spi::setWordSize(unsigned int wordSize) noexcept
{
    spi->CR0=Regs::CR0_DSS().put(wordSize-1,spi->CR0);
}

unsigned short PL022Spi::sendRecv(unsigned short data, unsigned wordSize) noexcept
{
    setWordSize(wordSize);
    spi->DR=data;
    waitForEndTransfer();
    return spi->DR;
}

template<typename D>
void PL022Spi::sendRecvImpl(const D send[], D recv[], size_t len, unsigned wordSize) noexcept
{
    setWordSize(wordSize);
    size_t w=0, r=0;
    // Transmit the first 4 bytes
    for(size_t i=0; i<4 && w<len; i++) spi->DR=send[w++];
    while(w<len)
    {
        // Transmit up to 4 bytes
        waitForInterrupt(Regs::INT_TX().mask());
        for(size_t i=0; i<4 && w<len; i++) spi->DR=send[w++];
        // Receive up to 4 bytes
        waitForInterrupt(Regs::INT_RX().mask());
        for(size_t i=0; i<4 && r<len; i++) recv[r++]=spi->DR;
    }
    // Last burst of reads
    waitForEndTransfer();
    while(r<len) recv[r++]=spi->DR;
}

template<typename D>
void PL022Spi::sendImpl(const D send[], size_t len, unsigned wordSize) noexcept
{
    setWordSize(wordSize);
    size_t w=0;
    // Transmit the first 4 bytes
    for(size_t i=0; i<4 && w<len; i++) spi->DR=send[w++];
    while(w<len)
    {
        // Transmit up to 4 bytes
        waitForInterrupt(Regs::INT_TX().mask());
        for(size_t i=0; i<4 && w<len; i++) spi->DR=send[w++];
    }
    // Flush read buffer
    waitForEndTransfer();
    while(Regs::SR_RNE().get(spi->SR)) (void)spi->DR;
    // Clear overflow conditions
    spi->ICR=Regs::INT_RO().mask() | Regs::INT_RT().mask();
}

template<typename D>
void PL022Spi::recvImpl(D recv[], size_t len, unsigned wordSize, D sendDummy) noexcept
{
    setWordSize(wordSize);
    size_t w=0, r=0;
    // Transmit the first 4 bytes
    for(size_t i=0; i<4 && w<len; i++) { spi->DR=sendDummy; w++; }
    while(w<len)
    {
        // Transmit up to 4 bytes
        waitForInterrupt(Regs::INT_TX().mask());
        for(size_t i=0; i<4 && w<len; i++) { spi->DR=sendDummy; w++; }
        // Receive up to 4 bytes
        waitForInterrupt(Regs::INT_RX().mask());
        for(size_t i=0; i<4 && r<len; i++) recv[r++]=spi->DR;
    }
    // Last burst of reads
    waitForEndTransfer();
    while(r<len) recv[r++]=spi->DR;
}

void PL022Spi::sendRecv(const unsigned short send[], unsigned short recv[], size_t len, unsigned wordSize) noexcept
{
    sendRecvImpl<unsigned short>(send, recv, len, wordSize);
}

void PL022Spi::sendRecv(const unsigned char send[], unsigned char recv[], size_t len, unsigned wordSize) noexcept
{
    sendRecvImpl<unsigned char>(send, recv, len, wordSize);
}

void PL022Spi::send(const unsigned short data[], size_t len, unsigned wordSize) noexcept
{
    sendImpl<unsigned short>(data, len, wordSize);
}

void PL022Spi::send(const unsigned char data[], size_t len, unsigned wordSize) noexcept
{
    sendImpl<unsigned char>(data, len, wordSize);
}

void PL022Spi::recv(unsigned short recv[], size_t len, unsigned wordSize, unsigned short sendDummy) noexcept
{
    recvImpl<unsigned short>(recv, len, wordSize, sendDummy);
}

void PL022Spi::recv(unsigned char recv[], size_t len, unsigned wordSize, unsigned short sendDummy) noexcept
{
    recvImpl<unsigned char>(recv, len, wordSize, sendDummy);
}

} // namespace miosix
