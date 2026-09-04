/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *   Copyright (C) 2026 by Raul Radu, Terraneo Federico                    *    
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
#include <interfaces/endianness.h>

using namespace miosix;

//Note: enabling debugging might cause deadlock when using sleep() or reboot()
//The bug won't be fixed because debugging is only useful for driver development
///\internal Debug macro, for normal conditions
//#define DBG iprintf
#define DBG(x,...) do {} while(0)
///\internal Debug macro, for errors only
//#define DBGERR iprintf
#define DBGERR(x,...) do {} while(0)

//Gpio mapping
using miso=Gpio<PA,6>;
using mosi=Gpio<PA,7>;
using sck=Gpio<PA,5>;
using cs=Gpio<PA,4>;

    
unsigned int SPIFlash::size() const { return 4*1024*1024; }

void SPIFlash::doWrite(const char *data, int size)
{
    error=false;
    //Wait until the SPI is (not?) busy, required otherwise the last byte is not
    //fully sent
    // iprintf("Waiting for SPI1 to be free... ");
    // while((SPI1->SR & SPI_SR_TXE)==0);
    // while(SPI1->SR & SPI_SR_BSY);
    //iprintf("Ok\nSetupping SPI...");
    // SPI1->CR1=0;
    SPI1->CR2=SPI_CR2_TXDMAEN;
    SPI1->CR1=SPI_CR1_SSM
    | SPI_CR1_SSI
    | SPI_CR1_MSTR
    | SPI_CR1_SPE;
    
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
    if (error)
        iprintf("Error\n");

    if(DMA2_Stream2->NDTR!=0) iprintf("Error, NDTR=%d\n",DMA2_Stream2->NDTR);

    //Wait for last byte to be sent
    while((SPI1->SR & SPI_SR_TXE)==0);
    while(SPI1->SR & SPI_SR_BSY);

    SPI1->CR1=0;
    SPI1->CR2=0;

    volatile short temp;    
    while(SPI1->SR & SPI_SR_RXNE) temp=SPI1->DR;
    (void) temp;
}

void SPIFlash::doRead(char *data, int size)
{
    error=false;

    // //Quirk: reset RXNE by reading DR before starting the DMA, or the first
    // //byte in the DMA buffer is garbage
    // volatile short temp=SPI1->DR;
    // (void)temp;
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
    SPI1->CR2=0;

    volatile short temp;

    //Quirk, disabling the SPI in RXONLY mode is difficult
    while(SPI1->SR & SPI_SR_RXNE) temp=SPI1->DR;
    delayUs(1); //The last transfer may still be in progress
    while(SPI1->SR & SPI_SR_RXNE) temp=SPI1->DR;
    (void) temp;
    // SPI1->CR2=0;
    // SPI1->CR1=SPI_CR1_SSM
    //         | SPI_CR1_SSI
    //         | SPI_CR1_MSTR
    //         | SPI_CR1_SPE;
}

int SPIFlash::ioctl(int cmd, void *arg)
{
    switch (cmd)
    {
        case IOCTL_GET_VOLUME_SIZE: 
        {
            DBG("IOCTL_GET_VOLUME_SIZE\n");
            unsigned long long* sizePtr=static_cast<unsigned long long*>(arg);
            if (sizePtr==nullptr) {
                return -EINVAL;
            }
            Lock<KernelMutex> l(mutex);
            *sizePtr=cardSize;
            return 0;
        }
        case IOCTL_GET_ERASE_SIZE: 
        {
            DBG("IOCTL_GET_ERASE_SIZE\n");
            unsigned long long* eraseSizePtr=static_cast<unsigned long long*>(arg);
            if (eraseSizePtr==nullptr) {
                return -EINVAL;
            }
            Lock<KernelMutex> l(mutex);
            *eraseSizePtr=eraseBlockSize;
            return 0;
        }
        case IOCTL_ERASE: 
        {
            DBG("IOCTL_ERASE\n");
            off_t* eraseAddrPtr=static_cast<off_t*>(arg);
            if (eraseAddrPtr==nullptr) {
                return -EINVAL;
            }
            Lock<KernelMutex> l(mutex);
            eraseBlock(*eraseAddrPtr);
            return 0;
        }
        case IOCTL_SYNC: 
        {
            DBG("IOCTL_SYNC\n");
            return 0; 
        }
        default:
            return -EINVAL;
    }
}

ssize_t SPIFlash::writeBlock(const void *buffer, size_t size, off_t where)
{
    // // Write time, with CPU @ 120MHz and SPI2 @ 15MHz, is
    // // 2    us fixed time (mutex locking)
    // // 3.5  us fixed time (sending the address an peripheral register setup)
    // // 0.533us per byte transferred in DMA mode
    // // 4.5  us fixed time (context switch and peripheral register cleanup)
    // // 2    us fixed time (mutex unlock)
    
    if(where % 512 || size % 512) return -EFAULT;
    if (where+size > 0xFFFFFF) return -EFAULT; //Final Address out of range, max 2^24 -1
    Lock<KernelMutex> l(mutex);
    DBG("SPIFlash::writeBlock(): size=%d [bytes]\n", size);

    for(size_t i = 0; i * 256 < size; i++)
    {
        cs::low();
        doWrite("\x06", 1);
        cs::high();
        
        // we use this computation timing to wait for the write enable to complete
        unsigned int writecmd = 0x02 << 24 | (where + (i * 256));
        writecmd = toBigEndian32(writecmd);
        char readS1 = 0x05;
        char s1;
        
        cs::low();
        doWrite(&readS1, 1);
        doRead(&s1, 1);
        while ((s1 & 0x02)==0) // wait for Write Enable Latch to be set
        {
            doRead(&s1, 1);
        }
        cs::high();
        
        delayUs(1);
        
        cs::low();
        doWrite(reinterpret_cast<const char*>(&writecmd), 4);
        doWrite(reinterpret_cast<const char*>(buffer) + (i * 256), 256);
        cs::high();
        
        delayUs(1);
        cs::low();
        doWrite(&readS1, 1);
        doRead(&s1, 1);
        while (s1 & 0x3) // wait for Write Enable Latch and BSY to be reset
        {
            doRead(&s1, 1);
        }
        cs::high();
        delayUs(1);
    }
    return size;
}

ssize_t SPIFlash::readBlock(void *buffer, size_t size, off_t where)
{
    if(where % 512 || size % 512) return -EFAULT;
    if (where+size > 0xFFFFFF) return -EFAULT; //Address out of range, max 2^24 -1
    
    unsigned int readcmd = 0x0b << 24 | where; 
    readcmd = toBigEndian32(readcmd);
    char data[5] = {0x00};
    memcpy(data, &readcmd, 4);
    data[4]=0;

    Lock<KernelMutex> l(mutex);
    DBG("SPIFlash::readBlock(): size=%d [bytes]\n", size);
    cs::low();
    doWrite(data, 5);
    doRead(reinterpret_cast<char*>(buffer), size);
    cs::high();

    return size;
}

void SPIFlash::eraseBlock(off_t where)
{
    if(where % 512 || where > 0xFFFFFF) return; //Address out of range, max 2^24 -1
    cs::low();
    doWrite("\x06", 1);
    cs::high();
    
 
    // we use this computation timing to wait for the write enable to complete
    unsigned int erasecmd = 0x20 << 24 | where;
    erasecmd = toBigEndian32(erasecmd);
    char readS1 = 0x05;
    char s1;
    
    cs::low();
    doWrite(&readS1, 1);
    doRead(&s1, 1);
    while ((s1 & 0x02)==0) // wait for Write Enable Latch to be set
    {
        doRead(&s1, 1);
    }
    cs::high();
    delayUs(1);
    cs::low();
    doWrite(reinterpret_cast<char*>(&erasecmd), 4);
    cs::high();
    delayUs(1);
    cs::low();
    doWrite(&readS1, 1);
    doRead(&s1, 1);
    while (s1 & 0x03) // wait for Write Enable Latch to be reset and BSY to be reset
    {
        doRead(&s1, 1);
    }
    cs::high();
}

SPIFlash::SPIFlash() : miosix::Device(miosix::Device::BLOCK)
{
    GlobalIrqLock dLock;
    mosi::mode(Mode::ALTERNATE);
    mosi::speed(Speed::_100MHz);
    mosi::alternateFunction(5);
    miso::mode(Mode::ALTERNATE);
    miso::speed(Speed::_100MHz);
    miso::alternateFunction(5);
    sck::mode(Mode::ALTERNATE);
    sck::speed(Speed::_100MHz);
    sck::alternateFunction(5);
    cs::mode(Mode::OUTPUT);
    cs::speed(Speed::_100MHz);
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
    SPI1->CR1 &= ~SPI_CR1_RXONLY;

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
    if(DMA2->LISR & (DMA_LISR_TEIF2)) error=true;

    DMA2->LIFCR=DMA_LIFCR_CTCIF2
              | DMA_LIFCR_CTEIF2
              | DMA_LIFCR_CDMEIF2;
    if(waiting) waiting->IRQwakeup();
    waiting=nullptr;
}
