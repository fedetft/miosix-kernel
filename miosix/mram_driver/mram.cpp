/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

#include "mram.h"
#include <miosix.h>
#include <kernel/scheduler/scheduler.h>
#include <interfaces/delays.h>

using namespace miosix;

//Gpio mapping
typedef Gpio<GPIOI_BASE,2> miso;
typedef Gpio<GPIOI_BASE,3> mosi;
typedef Gpio<GPIOI_BASE,1> sck;
typedef Gpio<GPIOI_BASE,0> cs;

/**
 * DMA RX end of transfer
 */
void __attribute__((naked)) DMA1_Stream3_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z20SPI2rxDmaHandlerImplv");
    restoreContext();
}

/**
 * DMA TX end of transfer
 */
void __attribute__((naked)) DMA1_Stream4_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z20SPI2txDmaHandlerImplv");
    restoreContext();
}

static Thread *waiting;
static bool error;

/**
 * DMA RX end of transfer actual implementation
 */
void __attribute__((used)) SPI2rxDmaHandlerImpl()
{
    if(DMA1->LISR & (DMA_LISR_TEIF3 | DMA_LISR_DMEIF3)) error=true;
    DMA1->LIFCR=DMA_LIFCR_CTCIF3
              | DMA_LIFCR_CTEIF3
              | DMA_LIFCR_CDMEIF3;
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    waiting=0;
}

/**
 * DMA TX end of transfer actual implementation
 */
void __attribute__((used)) SPI2txDmaHandlerImpl()
{
    if(DMA1->HISR & (DMA_HISR_TEIF4 | DMA_HISR_DMEIF4)) error=true;
    DMA1->HIFCR=DMA_HIFCR_CTCIF4
              | DMA_HIFCR_CTEIF4
              | DMA_HIFCR_CDMEIF4;
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    waiting=0;
}

/**
 * Transfer a byte through SPI2 where the mram is connected
 * \param data byte to send
 * \return byte received
 */
static unsigned char spi2sendRecv(unsigned char data=0)
{
    SPI2->DR=data;
    while((SPI2->SR & SPI_SR_RXNE)==0) ;
    return SPI2->DR;
}

//
// class Mram
//

Mram& Mram::instance()
{
    static Mram singleton;
    return singleton;
}
    
unsigned int Mram::size() const { return 128*1024; }

bool Mram::write(unsigned int addr, const void *data, int size)
{
    // Write time, with CPU @ 120MHz and SPI2 @ 15MHz, is
    // 2    us fixed time (mutex locking)
    // 3.5  us fixed time (sending the address an peripheral register setup)
    // 0.533us per byte transferred in DMA mode
    // 4.5  us fixed time (context switch and peripheral register cleanup)
    // 2    us fixed time (mutex unlock)
    
    if(addr>=this->size() || addr+size>=this->size()) return false;
    pthread_mutex_lock(&mutex);
    if(sleepMode)
    {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    cs::low();
    spi2sendRecv(0x02); //Write command
    spi2sendRecv((addr>>16) & 0xff);
    spi2sendRecv((addr>>8) & 0xff);
    spi2sendRecv(addr & 0xff);
    
    //DMA1 stream 4 channel 0 = SPI2_TX
    {
        FastInterruptDisableLock dLock;
        error=false;
        
        //Wait until the SPI is busy, required otherwise the last byte is not
        //fully sent
        while((SPI2->SR & SPI_SR_TXE)==0) ;
        while(SPI2->SR & SPI_SR_BSY) ;
        SPI2->CR1=0;
        SPI2->CR2=SPI_CR2_TXDMAEN;
        SPI2->CR1=SPI_CR1_SSM
                | SPI_CR1_SSI
                | SPI_CR1_MSTR
                | SPI_CR1_SPE;
        
        waiting=Thread::getCurrentThread();
        NVIC_ClearPendingIRQ(DMA1_Stream4_IRQn);
        NVIC_SetPriority(DMA1_Stream4_IRQn,10);//Low priority for DMA
        NVIC_EnableIRQ(DMA1_Stream4_IRQn);
        
        DMA1_Stream4->CR=0;
        DMA1_Stream4->PAR=reinterpret_cast<unsigned int>(&SPI2->DR);
        DMA1_Stream4->M0AR=reinterpret_cast<unsigned int>(data);
        DMA1_Stream4->NDTR=size;
        DMA1_Stream4->CR=DMA_SxCR_PL_1 //High priority because fifo disabled
                    | DMA_SxCR_MINC    //Increment memory pointer
                    | DMA_SxCR_DIR_0   //Memory to peripheral
                    | DMA_SxCR_TCIE    //Interrupt on transfer complete
                    | DMA_SxCR_TEIE    //Interrupt on transfer error
                    | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                    | DMA_SxCR_EN;     //Start DMA
        
        while(waiting!=0)
        {
            waiting->IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
        NVIC_DisableIRQ(DMA1_Stream4_IRQn);
        
        //Wait for last byte to be sent
        while((SPI2->SR & SPI_SR_TXE)==0) ;
        while(SPI2->SR & SPI_SR_BSY) ;
        SPI2->CR1=0;
        SPI2->CR2=0;
        SPI2->CR1=SPI_CR1_SSM
                | SPI_CR1_SSI
                | SPI_CR1_MSTR
                | SPI_CR1_SPE;
    }
    cs::high();
    bool result=!error;
    pthread_mutex_unlock(&mutex);
    return result;
}

bool Mram::read(unsigned int addr, void *data, int size)
{
    // Read time, with CPU @ 120MHz and SPI2 @ 15MHz, is
    // 2    us fixed time (mutex locking)
    // 3.5  us fixed time (sending the address an peripheral register setup)
    // 0.533us per byte transferred in DMA mode
    // 4.5  us fixed time (context switch and peripheral register cleanup)
    // 2    us fixed time (mutex unlock)
    if(addr>=this->size() || addr+size>=this->size()) return false;
    pthread_mutex_lock(&mutex);
    if(sleepMode) { pthread_mutex_unlock(&mutex); return false; }
    cs::low();
    spi2sendRecv(0x03); //Read command
    spi2sendRecv((addr>>16) & 0xff);
    spi2sendRecv((addr>>8) & 0xff);
    spi2sendRecv(addr & 0xff);
    
    //DMA1 stream 3 channel 0 = SPI2_RX
    {
        FastInterruptDisableLock dLock;
        error=false;
        
        //Wait until the SPI is busy, required otherwise the last byte is not
        //fully sent
        while((SPI2->SR & SPI_SR_TXE)==0) ;
        while(SPI2->SR & SPI_SR_BSY) ;
        //Quirk: reset RXNE by reading DR before starting the DMA, or the first
        //byte in the DMA buffer is garbage
        volatile short temp=SPI2->DR;
        (void)temp;
        SPI2->CR1=0;
        SPI2->CR2=SPI_CR2_RXDMAEN;
        
        waiting=Thread::getCurrentThread();
        NVIC_ClearPendingIRQ(DMA1_Stream3_IRQn);
        NVIC_SetPriority(DMA1_Stream3_IRQn,10);//Low priority for DMA
        NVIC_EnableIRQ(DMA1_Stream3_IRQn);
        
        DMA1_Stream3->CR=0;
        DMA1_Stream3->PAR=reinterpret_cast<unsigned int>(&SPI2->DR);
        DMA1_Stream3->M0AR=reinterpret_cast<unsigned int>(data);
        DMA1_Stream3->NDTR=size;
        DMA1_Stream3->CR=DMA_SxCR_PL_1 //High priority because fifo disabled
                    | DMA_SxCR_MINC    //Increment memory pointer
                    | DMA_SxCR_TCIE    //Interrupt on transfer complete
                    | DMA_SxCR_TEIE    //Interrupt on transfer error
                    | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                    | DMA_SxCR_EN;     //Start DMA
        
        //Quirk: start the SPI in RXONLY mode only *after* the DMA has been
        //setup or the SPI doesn't wait for the DMA and the first bytes are lost
        SPI2->CR1=SPI_CR1_RXONLY
                | SPI_CR1_SSM
                | SPI_CR1_SSI
                | SPI_CR1_MSTR
                | SPI_CR1_SPE;
        
        while(waiting!=0)
        {
            waiting->IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
        NVIC_DisableIRQ(DMA1_Stream3_IRQn);
        SPI2->CR1=0;
        SPI2->CR2=0;
        SPI2->CR1=SPI_CR1_SSM
                | SPI_CR1_SSI
                | SPI_CR1_MSTR
                | SPI_CR1_SPE;
    }
    cs::high();
    bool result=!error;
    pthread_mutex_unlock(&mutex);
    return result;
}

void Mram::exitSleepMode()
{
    pthread_mutex_lock(&mutex);
    cs::low();
    spi2sendRecv(0xab); //Exit sleep mode command
    cs::high();
    Thread::sleep(1);   //It needs at least 400us to go out of sleep mode
    cs::low();
    spi2sendRecv(0x06); //Write enable command
    cs::high();
    sleepMode=false;
    pthread_mutex_unlock(&mutex);
}

void Mram::enterSleepMode()
{
    pthread_mutex_lock(&mutex);
    cs::low();
    spi2sendRecv(0x04); //Write disable command
    cs::high();
    for(int i=0;i<5;i++) __NOP(); //To meet 40ns CS high time between commands
    cs::low();
    spi2sendRecv(0xb9); //Entr sleep mode command
    cs::high();
    delayUs(3);         //It needs at least 3us to enter sleep mode
    sleepMode=true;
    pthread_mutex_unlock(&mutex);
}

Mram::Mram() : sleepMode(true)
{
    pthread_mutex_init(&mutex,0);
    {
        FastInterruptDisableLock dLock;
        mosi::mode(Mode::ALTERNATE);
        mosi::alternateFunction(5);
        miso::mode(Mode::ALTERNATE);
        miso::alternateFunction(5);
        sck::mode(Mode::ALTERNATE);
        sck::alternateFunction(5);
        cs::mode(Mode::OUTPUT);
        cs::high();
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
        RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
        SPI2->CR2=0;
        SPI2->CR1=SPI_CR1_SSM  //Software cs
                | SPI_CR1_SSI  //Hardware cs internally tied high
                | SPI_CR1_MSTR //Master mode
                | SPI_CR1_SPE; //SPI enabled
    }
}
