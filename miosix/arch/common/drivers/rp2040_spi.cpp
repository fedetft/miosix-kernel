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

#include "rp2040_spi.h"
#include "config/miosix_settings.h"
#include "rp2040_dma.h"
#include "kernel/lock.h"
#include "kernel/logging.h"

namespace miosix {

RP2040PL022SPI::RP2040PL022SPI(int number, unsigned int bitrate, bool spo, bool sph,
                               GpioPin si, GpioPin so, GpioPin sck, GpioPin ce)
{
    {
        GlobalIrqLock lock;
        IRQn_Type irqn;
        switch(number)
        {
            case 0:
                unreset_block_wait(RESETS_RESET_SPI0_BITS);
                spi=spi0_hw;
                irqn=SPI0_IRQ_IRQn;
                break;
            case 1:
                unreset_block_wait(RESETS_RESET_SPI1_BITS);
                spi=spi1_hw;
                irqn=SPI1_IRQ_IRQn;
                break;
            default:
                errorHandler(Error::UNEXPECTED);
        }
        IRQregisterIrq(lock,irqn,&RP2040PL022SPI::IRQhandleInterrupt,this);
        txDmaCh=RP2040Dma::IRQregisterChannel(lock,&RP2040PL022SPI::IRQhandleDmaInterrupt,this);
        rxDmaCh=RP2040Dma::IRQregisterChannel(lock,&RP2040PL022SPI::IRQhandleDmaInterrupt,this);
        si.function(Function::SPI); si.mode(Mode::INPUT); si.fast();
        so.function(Function::SPI); so.mode(Mode::OUTPUT); so.fast();
        sck.function(Function::SPI); sck.mode(Mode::OUTPUT); sck.fast();
        if (ce.isValid())
        {
            ce.function(Function::SPI); ce.mode(Mode::OUTPUT); ce.fast();
        }
    }
    spi->cr0=(0<<SPI_SSPCR0_SCR_LSB)
            |(sph?SPI_SSPCR0_SPH_BITS:0)
            |(spo?SPI_SSPCR0_SPO_BITS:0)
            |(0<<SPI_SSPCR0_FRF_LSB)
            |(7<<SPI_SSPCR0_DSS_LSB);
    setBitrate(bitrate);
    spi->cr1=SPI_SSPCR1_SSE_BITS;
    spi->dmacr=SPI_SSPDMACR_TXDMAE_BITS|SPI_SSPDMACR_RXDMAE_BITS;
    this->so=so;
}

RP2040PL022SPI::~RP2040PL022SPI()
{
    GlobalIrqLock lock;
    spi->cr1=0;
    if(spi==spi0_hw)
    {
        IRQunregisterIrq(lock,SPI0_IRQ_IRQn,&RP2040PL022SPI::IRQhandleInterrupt,this);
        reset_block(RESETS_RESET_SPI0_BITS);
    } else {
        IRQunregisterIrq(lock,SPI1_IRQ_IRQn,&RP2040PL022SPI::IRQhandleInterrupt,this);
        reset_block(RESETS_RESET_SPI1_BITS);
    }
    RP2040Dma::IRQunregisterChannel(lock,txDmaCh,&RP2040PL022SPI::IRQhandleDmaInterrupt,this);
    RP2040Dma::IRQunregisterChannel(lock,rxDmaCh,&RP2040PL022SPI::IRQhandleDmaInterrupt,this);
}

void RP2040PL022SPI::setBitrate(unsigned int bitrate)
{
    this->bitrate=bitrate;
    unsigned int ratio=peripheralFrequency/bitrate;
    if(ratio<2) ratio=2;
    if(ratio>0xfe00) errorHandler(Error::UNEXPECTED);
    unsigned int presc=2;
    while(ratio>presc*0x100) presc<<=1;
    if(presc>0xfe) presc=0xfe;
    unsigned int scr=(ratio/presc)-1;
    spi->cpsr=presc;
    spi->cr0=(spi->cr0&~SPI_SSPCR0_SCR_BITS) | (scr<<SPI_SSPCR0_SCR_LSB);
}

void RP2040PL022SPI::IRQhandleInterrupt()
{
    FastGlobalLockFromIrq lock;
    if(waiting)
    {
        waiting->IRQwakeup();
        waiting=nullptr;
    }
}

void RP2040PL022SPI::IRQhandleDmaInterrupt()
{
    // We do not take the GIL because the DMA code already did
    spi->imsc=0;
    if(waiting)
    {
        waiting->IRQwakeup();
        waiting=nullptr;
    }
}

void RP2040PL022SPI::setWordSize(unsigned int wordSize)
{
    spi->cr0=(spi->cr0 & ~SPI_SSPCR0_DSS_BITS)
            |((wordSize-1)<<SPI_SSPCR0_DSS_LSB);
}

unsigned short RP2040PL022SPI::sendRecv(unsigned short data, unsigned wordSize)
{
    // Just polling when sending a single byte because using the DMA will
    // probably take more cycles
    setWordSize(wordSize);
    spi->dr=data;
    if(bitrate<cpuFrequency/100)
    {
        long long sleepns=8000000000LL/bitrate;
        while(!(spi->sr&SPI_SSPSR_RNE_BITS)) Thread::nanoSleep(sleepns);
    } else {
        while(!(spi->sr&SPI_SSPSR_RNE_BITS)) ;
    }
    unsigned short res=spi->dr;
    return res;
}

template<typename D>
void RP2040PL022SPI::sendRecvImpl(const D send[], D recv[], size_t len, unsigned wordSize)
{
    setWordSize(wordSize);
    unsigned int datasz=sizeof(D)==1
            ?DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_BYTE
            :DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD;
    // In SPI sending and receiving happens simultaneously, so the DMA also
    // must pump data in as fast as it pumps it out. Unfortunately this is
    // NOT necessarily the case because of bus contention for instance.
    // So it can happen that the write DMA pumps data in merrily while the read
    // DMA is not keeping up with the FIFO getting full. So the PL022 internal
    // read FIFO overflows and we lose bytes.
    //   To work around this issue we force the write DMA to work in lockstep
    // with the read DMA by setting both DMAs' trigger requests to the read
    // trigger. To kickstart the process we write the first byte of the
    // transmission *in software* to the DR.
    //   This has the side-effect of slowing down the data transfer when we
    // get around 25MHz, which if we are running the Pico at 200MHz leaves
    // 8 cycles for the DMA to do the whole read->write bus operation.
    // So it makes sense that we are slowing down just about there, because
    // any faster and we barely have the cycles to do anything at all.
    //   Now, it would be MUCH better if the PL022 did NOT assert the DMA TX
    // signal if the read FIFO was full, given that, well, it's SPI GODDAMNIT.
    // This peripheral almost seems to be designed by people who are ignorant
    // of the fact that read/write must be simultaneous in this protocol!...
    dma_hw->ch[txDmaCh].read_addr=reinterpret_cast<unsigned int>(send+1);
    dma_hw->ch[txDmaCh].write_addr=reinterpret_cast<unsigned int>(&spi->dr);
    dma_hw->ch[txDmaCh].transfer_count=len-1;
    dma_hw->ch[txDmaCh].al1_ctrl=(17<<DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB)
                                |(txDmaCh<<DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) // disable chaining!!!!
                                |DMA_CH0_CTRL_TRIG_INCR_READ_BITS
                                |(datasz<<DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB)
                                |DMA_CH0_CTRL_TRIG_EN_BITS;
    dma_hw->ch[rxDmaCh].read_addr=reinterpret_cast<unsigned int>(&spi->dr);
    dma_hw->ch[rxDmaCh].write_addr=reinterpret_cast<unsigned int>(recv);
    dma_hw->ch[rxDmaCh].transfer_count=len;
    dma_hw->ch[rxDmaCh].al1_ctrl=(17<<DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB)
                                |(rxDmaCh<<DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) // disable chaining!!!!
                                |DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS
                                |(datasz<<DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB)
                                |DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS
                                |DMA_CH0_CTRL_TRIG_EN_BITS;
    {
        FastGlobalIrqLock lock;
        spi->imsc=SPI_SSPIMSC_RORIM_BITS; // enable RX overrun interrupt
        dma_hw->multi_channel_trigger=(1U<<txDmaCh)|(1U<<rxDmaCh);
        spi->dr=send[0]; // kickstart the transfer
        while(true)
        {
            if(!((dma_hw->ch[rxDmaCh].al1_ctrl)&DMA_CH0_CTRL_TRIG_BUSY_BITS)) break;
            if(spi->ris&SPI_SSPRIS_RORRIS_BITS)
            {
                // There was a read fifo overflow error! This means the DMA
                // channel for reading WON'T be able to complete the transfer
                // and some bytes have been lost. So nothing good left to do but
                // report the error...
                #ifdef WITH_ERRLOG
                IRQerrorLog("SPI RX FIFO overrun, you are running too fast!");
                #endif // WITH_ERRLOG
                errorHandler(Error::UNEXPECTED);
            }
            waiting=Thread::IRQgetCurrentThread();
            while(waiting) Thread::IRQglobalIrqUnlockAndWait(lock);
        }
        dma_hw->ch[txDmaCh].al1_ctrl=0;
        dma_hw->ch[rxDmaCh].al1_ctrl=0;
        spi->imsc=0; // disable RX overrun interrupt
        spi->icr=3; // clear all interrupts
    }
}

template<typename D>
void RP2040PL022SPI::sendImpl(const D send[], size_t len, unsigned wordSize)
{
    setWordSize(wordSize);
    unsigned int datasz=sizeof(D)==1
            ?DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_BYTE
            :DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD;
    dma_hw->ch[txDmaCh].read_addr=reinterpret_cast<unsigned int>(send);
    dma_hw->ch[txDmaCh].write_addr=reinterpret_cast<unsigned int>(&spi->dr);
    dma_hw->ch[txDmaCh].transfer_count=len;
    {
        FastGlobalIrqLock lock;
        dma_hw->ch[txDmaCh].ctrl_trig=(16<<DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB)
                               |DMA_CH0_CTRL_TRIG_INCR_READ_BITS
                               |(datasz<<DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB)
                               |DMA_CH0_CTRL_TRIG_EN_BITS;
        while((dma_hw->ch[txDmaCh].al1_ctrl)&DMA_CH0_CTRL_TRIG_BUSY_BITS)
        {
            waiting=Thread::IRQgetCurrentThread();
            while(waiting) Thread::IRQglobalIrqUnlockAndWait(lock);
        }
        dma_hw->ch[txDmaCh].al1_ctrl=0;
    }
    // flush the SPI fifo
    while(spi->sr&SPI_SSPSR_BSY_BITS) ;
    while(spi->sr&SPI_SSPSR_RNE_BITS) (void)spi->dr;
    spi->icr=3; // clear all interrupts (the overrun interrupt might have triggered)
}

template<typename D>
void RP2040PL022SPI::recvImpl(D recv[], size_t len, unsigned wordSize, D sendDummy)
{
    setWordSize(wordSize);
    unsigned int datasz=sizeof(D)==1
            ?DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_BYTE
            :DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD;
    // This API is for receiving data without really sending anything --
    // or better we just send the same byte over and over.
    // But we still need to write the byte to the data register
    // so the same funny cursed business happens as in sendRecvImpl.
    dma_hw->ch[txDmaCh].read_addr=reinterpret_cast<unsigned int>(&sendDummy);
    dma_hw->ch[txDmaCh].write_addr=reinterpret_cast<unsigned int>(&spi->dr);
    dma_hw->ch[txDmaCh].transfer_count=len-1;
    dma_hw->ch[txDmaCh].al1_ctrl=(17<<DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB)
                                |(txDmaCh<<DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) // disable chaining!!!!
                                |(datasz<<DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB)
                                |DMA_CH0_CTRL_TRIG_EN_BITS;
    dma_hw->ch[rxDmaCh].read_addr=reinterpret_cast<unsigned int>(&spi->dr);
    dma_hw->ch[rxDmaCh].write_addr=reinterpret_cast<unsigned int>(recv);
    dma_hw->ch[rxDmaCh].transfer_count=len;
    dma_hw->ch[rxDmaCh].al1_ctrl=(17<<DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB)
                                |(rxDmaCh<<DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) // disable chaining!!!!
                                |DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS
                                |(datasz<<DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB)
                                |DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS
                                |DMA_CH0_CTRL_TRIG_EN_BITS;
    {
        FastGlobalIrqLock lock;
        spi->imsc=SPI_SSPIMSC_RORIM_BITS; // enable RX overrun interrupt
        dma_hw->multi_channel_trigger=(1U<<txDmaCh)|(1U<<rxDmaCh);
        spi->dr=sendDummy; // kickstart the transfer
        while(true)
        {
            if(!((dma_hw->ch[rxDmaCh].al1_ctrl)&DMA_CH0_CTRL_TRIG_BUSY_BITS)) break;
            if(spi->ris&SPI_SSPRIS_RORRIS_BITS)
            {
                // There was a read fifo overflow error! This means the DMA
                // channel for reading WON'T be able to complete the transfer
                // and some bytes have been lost. So nothing good left to do but
                // report the error...
                #ifdef WITH_ERRLOG
                IRQerrorLog("SPI RX FIFO overrun, you are running too fast!");
                #endif // WITH_ERRLOG
                errorHandler(Error::UNEXPECTED);
            }
            waiting=Thread::IRQgetCurrentThread();
            while(waiting) Thread::IRQglobalIrqUnlockAndWait(lock);
        }
        dma_hw->ch[txDmaCh].al1_ctrl=0;
        dma_hw->ch[rxDmaCh].al1_ctrl=0;
        spi->imsc=0; // disable RX overrun interrupt
        spi->icr=3; // clear all interrupts
    }
}

void RP2040PL022SPI::sendRecv(const unsigned short send[], unsigned short recv[], size_t len, unsigned wordSize)
{
    sendRecvImpl<unsigned short>(send, recv, len, wordSize);
}

void RP2040PL022SPI::sendRecv(const unsigned char send[], unsigned char recv[], size_t len, unsigned wordSize)
{
    sendRecvImpl<unsigned char>(send, recv, len, wordSize);
}

void RP2040PL022SPI::send(const unsigned short send[], size_t len, unsigned wordSize)
{
    sendImpl<unsigned short>(send, len, wordSize);
}

void RP2040PL022SPI::send(const unsigned char send[], size_t len, unsigned wordSize)
{
    sendImpl<unsigned char>(send, len, wordSize);
}

void RP2040PL022SPI::recv(unsigned short recv[], size_t len, unsigned wordSize, unsigned short sendDummy)
{
    recvImpl<unsigned short>(recv, len, wordSize, sendDummy);
}

void RP2040PL022SPI::recv(unsigned char recv[], size_t len, unsigned wordSize, unsigned short sendDummy)
{
    recvImpl<unsigned char>(recv, len, wordSize, sendDummy);
}

} // namespace miosix
