/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *   Copyright (C) 2026 by Radu Raul                                       *    
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

#include "spi-flash.h"
#include <interfaces/delays.h>
#include <interfaces/bsp.h>

using namespace miosix;

//Gpio mapping
using miso=Gpio<PA,6>;
using mosi=Gpio<PA,7>;
using sck=Gpio<PA,5>;
using cs=Gpio<PA,4>;

/**
 * Transfer a byte through SPI2 where the mram is connected
 * \param data byte to send
 * \return byte received
 */
// static unsigned char spi2sendRecv(unsigned char data=0)
// {
//     SPI2->DR=data;
//     while((SPI2->SR & SPI_SR_RXNE)==0) ;
//     return SPI2->DR;
// }
    
unsigned int SPIFlash::size() const { return 4*1024*1024; }

void SPIFlash::doWrite(const char *data, int size)
{
    //Wait until the SPI is (not?) busy, required otherwise the last byte is not
    //fully sent
    while((SPI1->SR & SPI_SR_TXE)==0);
    while(SPI1->SR & SPI_SR_BSY);

    SPI1->CR1=0;
    SPI1->CR2=SPI_CR2_TXDMAEN;
    SPI1->CR1=SPI_CR1_SSM
            | SPI_CR1_SSI
            | SPI_CR1_MSTR
            | SPI_CR1_SPE
            | SPI_CR1_BR_2;

    waiting=Thread::getCurrentThread();

    DMA2_Stream2->CR=0;
    DMA2_Stream2->PAR=reinterpret_cast<unsigned int>(&SPI1->DR);
    DMA2_Stream2->M0AR=reinterpret_cast<unsigned int>(data);
    DMA2_Stream2->NDTR=size;
    DMA2_Stream2->CR=DMA_SxCR_PL_1 //High priority because fifo disabled
                | DMA_SxCR_MINC    //Increment memory pointer
                | DMA_SxCR_DIR_0   //Memory to peripheral
                | DMA_SxCR_TCIE    //Interrupt on transfer complete
                | DMA_SxCR_TEIE    //Interrupt on transfer error
                | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                | DMA_SxCR_CHSEL_1 //Selecting channel 2 (010)
                | DMA_SxCR_EN;     //Start DMA
    
    {
        FastGlobalIrqLock dLock;
        while(waiting) Thread::IRQglobalIrqUnlockAndWait(dLock);
    }

    if (error) {
        for (size_t i = 0; i < 10; i++)
        {
            ledOn();
            Thread::sleep(50);
            ledOff();
            Thread::sleep(50);
        }
    }

    //Wait for last byte to be sent
    while((SPI1->SR & SPI_SR_TXE)==0);
    while(SPI1->SR & SPI_SR_BSY);
    
    SPI1->CR1=0;
    SPI1->CR2=0;
    SPI1->CR1=SPI_CR1_SSM
            | SPI_CR1_SSI
            | SPI_CR1_MSTR
            | SPI_CR1_SPE;
    
    //Quirk: reset RXNE by reading DR, or a byte remains in the input buffer
    volatile short temp=SPI1->DR;
    (void)temp;
}

void SPIFlash::doRead(char *data, int size)
{
    //Wait until the SPI is busy, required otherwise the last byte is not
    //fully sent
    while((SPI1->SR & SPI_SR_TXE)==0) ;
    while(SPI1->SR & SPI_SR_BSY) ;
    //Quirk: reset RXNE by reading DR before starting the DMA, or the first
    //byte in the DMA buffer is garbage
    volatile short temp=SPI1->DR;
    (void)temp;
    SPI1->CR1=0;
    SPI1->CR2=SPI_CR2_RXDMAEN;

    waiting=Thread::getCurrentThread();

    DMA2_Stream0->CR=0;
    DMA2_Stream0->PAR=reinterpret_cast<unsigned int>(&SPI1->DR);
    DMA2_Stream0->M0AR=reinterpret_cast<unsigned int>(data);
    DMA2_Stream0->NDTR=size;
    DMA2_Stream0->CR=DMA_SxCR_PL_1 //High priority because fifo disabled
                | DMA_SxCR_MINC    //Increment memory pointer
                | DMA_SxCR_TCIE    //Interrupt on transfer complete
                | DMA_SxCR_TEIE    //Interrupt on transfer error
                | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                | DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_0  // channel 3 (011)
                | DMA_SxCR_EN;     //Start DMA

    //Quirk: start the SPI in RXONLY mode only *after* the DMA has been
    //setup or the SPI doesn't wait for the DMA and the first bytes are lost
    SPI1->CR1=SPI_CR1_RXONLY
            | SPI_CR1_SSM
            | SPI_CR1_SSI
            | SPI_CR1_MSTR
            | SPI_CR1_SPE;
        
    {
        FastGlobalIrqLock dLock;
        while(waiting) Thread::IRQglobalIrqUnlockAndWait(dLock);
    }


    SPI1->CR1=0;
    
    //Quirk, disabling the SPI in RXONLY mode is difficult
    while(SPI1->SR & SPI_SR_RXNE) temp=SPI1->DR;
    delayUs(1); //The last transfer may still be in progress
    while(SPI1->SR & SPI_SR_RXNE) temp=SPI1->DR;
    
    SPI1->CR2=0;
    SPI1->CR1=SPI_CR1_SSM
            | SPI_CR1_SSI
            | SPI_CR1_MSTR
            | SPI_CR1_SPE;
}

bool SPIFlash::write(unsigned int addr, const void *data, int size)
{
    // // Write time, with CPU @ 120MHz and SPI2 @ 15MHz, is
    // // 2    us fixed time (mutex locking)
    // // 3.5  us fixed time (sending the address an peripheral register setup)
    // // 0.533us per byte transferred in DMA mode
    // // 4.5  us fixed time (context switch and peripheral register cleanup)
    // // 2    us fixed time (mutex unlock)
    
    // if(addr>=this->size() || addr+size>this->size()) return false;
    // cs::low();
    // spi2sendRecv(0x02); //Write command
    // spi2sendRecv((addr>>16) & 0xff);
    // spi2sendRecv((addr>>8) & 0xff);
    // spi2sendRecv(addr & 0xff);
    
    // //DMA1 stream 4 channel 0 = SPI2_TX
    
    // error=false;

    
    
    // cs::high();
    // bool result=!error;
    // return result;
    return false;
}

bool SPIFlash::read(unsigned int addr, void *data, int size)
{
    // if(addr>=this->size() || addr+size>this->size()) return false;
    // cs::low();
    // spi2sendRecv(0x03); //Read command
    // spi2sendRecv((addr>>16) & 0xff);
    // spi2sendRecv((addr>>8) & 0xff);
    // spi2sendRecv(addr & 0xff);
    


    // cs::high();
    // bool result=!error;
    // return result;
    return false;
}

SPIFlash::SPIFlash()
{
    GlobalIrqLock dLock;
    mosi::mode(Mode::ALTERNATE);
    mosi::alternateFunction(5);
    miso::mode(Mode::ALTERNATE);
    miso::alternateFunction(5);
    sck::mode(Mode::ALTERNATE);
    sck::alternateFunction(5);
    cs::mode(Mode::OUTPUT);
    cs::high();
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR2=0;
    SPI1->CR1=SPI_CR1_SSM  //Software cs
            | SPI_CR1_SSI  //Hardware cs internally tied high
            | SPI_CR1_MSTR //Master mode
            | SPI_CR1_SPE; //SPI enabled
    IRQregisterIrq(dLock,DMA2_Stream0_IRQn,&SPIFlash::dmaRxHandler,this);
    IRQregisterIrq(dLock,DMA2_Stream2_IRQn,&SPIFlash::dmaTxHandler,this);
}

SPIFlash::~SPIFlash()
{
    GlobalIrqLock dLock;
    IRQunregisterIrq(dLock,DMA2_Stream0_IRQn,&SPIFlash::dmaRxHandler,this);
    IRQunregisterIrq(dLock,DMA2_Stream2_IRQn,&SPIFlash::dmaTxHandler,this);
    SPI1->CR1=0;
    RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
}

/**
 * DMA RX end of transfer actual implementation
 */
void SPIFlash::dmaRxHandler()
{
    if(DMA2->LISR & (DMA_LISR_TEIF0 | DMA_LISR_DMEIF0)) error=true;
    DMA2->LIFCR=DMA_LIFCR_CTCIF0
              | DMA_LIFCR_CTEIF0
              | DMA_LIFCR_CDMEIF0;
    if(waiting) waiting->IRQwakeup();
    waiting=nullptr;
}

/**
 * DMA TX end of transfer actual implementation
 */
void SPIFlash::dmaTxHandler()
{
    if(DMA2->LISR & (DMA_LISR_TEIF2 | DMA_LISR_DMEIF2)) error=true;

    DMA2->LIFCR=DMA_LIFCR_CTCIF2
              | DMA_LIFCR_CTEIF2
              | DMA_LIFCR_CDMEIF2;
    if(waiting) waiting->IRQwakeup();
    waiting=nullptr;
}
