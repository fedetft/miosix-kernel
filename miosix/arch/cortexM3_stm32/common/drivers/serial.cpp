/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico                   *
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

#include <cstring>
#include <algorithm>
#include "serial.h"
#include "kernel/sync.h"
#include "kernel/scheduler/scheduler.h"
#include "interfaces/arch_registers.h"
#include "interfaces/portability.h"
#include "interfaces/delays.h"
#include "interfaces/gpio.h"

/*
 * A word about this serial driver.
 * Unfortunately the stm32 has a bug, documented in the errata sheet that
 * if both the dma and the cpu access the external memory, shit happens.
 * Since I use to put code in xram for debugging, the cpu fetches data from
 * xram and there's no way to avoid that, so I can't use dma while debugging.
 * As a result, the driver for transmission contains two different
 * implementations selected with lots of #ifdef __ENABLE_XRAM. If the external
 * ram is disabled, the fast dma implementation is used, otherwise the slow
 * polled driver.
 */

/**
 * \internal
 * serial port interrupt routine
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in serial_irq_impl()
 */
void USART1_IRQHandler() __attribute__ ((naked));
void USART1_IRQHandler()
{
    saveContext();
	//Call serial_irq_impl(). Name is a C++ mangled name.
	asm volatile("bl _ZN6miosix15serial_irq_implEv");
    restoreContext();
}

#ifndef __ENABLE_XRAM
/**
 * \internal
 * serial port tx dma interrupt routine
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in serial_dma_impl()
 */
void DMA1_Channel4_IRQHandler() __attribute__ ((naked));
void DMA1_Channel4_IRQHandler()
{
    saveContext();
    //Call serial_dma_impl(). Name is a C++ mangled name.
    asm volatile("bl _ZN6miosix15serial_dma_implEv");
    restoreContext();
}
#endif //__ENABLE_XRAM

namespace miosix {

static Mutex rxMutex;///<\internal Mutex used to guard the rx queue
const unsigned int SOFTWARE_RX_QUEUE=32;///<\internal Size of rx software queue
static Queue<char,SOFTWARE_RX_QUEUE> rxQueue;///<\internal Rx software queue

///\internal True if a rx character found the queue full
static volatile bool rxLostFlag=false;
static bool serialEnabled=false;///<\internal True if serial port is enabled

/**
 * \internal
 * Serial interrupt code. Declared noinline to avoid the compiler trying to
 * inline it into the caller, which would violate the requirement on naked
 * functions.
 * Function is not static because otherwise the compiler optimizes it out...
 */
void serial_irq_impl() __attribute__((noinline));
void serial_irq_impl()
{
    bool hppw=false;
    unsigned int status=USART1->SR;
    if(status & USART_SR_ORE) //Receiver overrun
    {
        rxLostFlag=true;
        char c=USART1->DR; //Always read data, since this clears interrupt flags
        if((status & USART_SR_RXNE) && ((status & 0x7)==0))
            rxQueue.IRQput(c,hppw);
    } else if(status & USART_SR_RXNE) //Receiver data available
    {
        char c=USART1->DR; //Always read data, since this clears interrupt flags
        if((status & 0x7)==0) //If no noise nor framing nor parity error
        {
            if(rxQueue.IRQput(c,hppw)==false) rxLostFlag=true;
        }
    }
    if(hppw) Scheduler::IRQfindNextThread();
}

static Mutex txMutex;///<\internal Mutex used to guard the tx queue

#ifndef __ENABLE_XRAM
const unsigned int SOFTWARE_TX_QUEUE=32;///<\internal Size of tx software queue
static char tx_dma_buffer[SOFTWARE_TX_QUEUE];///<\internal Tx dma buffer
static Queue<char,1> tx_queue;///<\internal Tx queue. Size is 1 because of dma

/**
 * \internal
 * Serial dma code. Declared noinline to avoid the compiler trying to
 * inline it into the caller, which would violate the requirement on naked
 * functions.
 * Function is not static because otherwise the compiler optimizes it out...
 */
void serial_dma_impl() __attribute__((noinline));
void serial_dma_impl()
{
    DMA1->IFCR=DMA_IFCR_CGIF4;
    DMA1_Channel4->CCR=0;//Disable DMA
    //Wake eventual waiting thread, and do context switch if priority is higher
    char useless;
    bool hppw=false;
    tx_queue.IRQget(useless,hppw);
    if(hppw) Scheduler::IRQfindNextThread();
}

void IRQserialInit()
{
    tx_queue.IRQreset();
    rxQueue.IRQreset();
    //Enable clock to GPIOA, AFIO and USART1
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN |
            RCC_APB2ENR_USART1EN;
    //Enable DMA1
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    //Enabled, 8 data bit, no parity, interrupt on character rx
    USART1->CR1 = USART_CR1_UE | USART_CR1_RXNEIE;
    USART1->CR2 = 0;//Disable lin mode and synchronous mode
    USART1->CR3 = USART_CR3_DMAT;//Disable irda and smartcard mode, dma mode

    //baudrate=19200
    #ifdef SYSCLK_FREQ_72MHz
    USART1->BRR = (234<<4) | (6<<0); //BRR=234.375      0% Error
    #elif SYSCLK_FREQ_56MHz
    USART1->BRR = (182<<4) | (5<<0); //BRR=182.3125 0.011% Error
    #elif SYSCLK_FREQ_48MHz
    USART1->BRR = (156<<4) | (4<<0); //BRR=156.25       0% Error
    #elif SYSCLK_FREQ_36MHz
    USART1->BRR = (117<<4) | (3<<0); //BRR=117.1875     0% Error
    #elif SYSCLK_FREQ_24MHz
    USART1->BRR = ( 78<<4) | (2<<0); //BRR= 78.125      0% Error
    #else //Running on internal 8MHz frequency
    USART1->BRR = ( 26<<4) | (1<<0); //BRR= 26.0625  0.08% Error
    #endif

    //Now that serial port is active, configure I/Os
    Gpio<GPIOA_BASE, 9>::mode(Mode::ALTERNATE);
    Gpio<GPIOA_BASE,10>::mode(Mode::INPUT);

    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;//Finally enable tx and rx
    NVIC_SetPriority(USART1_IRQn,10);//Low priority for serial. (Max=0, min=15)
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(DMA1_Channel4_IRQn,10);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    serialEnabled=true;
}

void IRQserialDisable()
{
    serialEnabled=false;
    while(!IRQserialTxFifoEmpty()) ; //wait
    USART1->CR1=0;
    tx_queue.IRQreset(); //Wake eventual thread waiting
}

void serialWrite(const char *str, unsigned int len)
{
    if(!serialEnabled) return;
    Lock<Mutex> l(txMutex);
    for(;len>0;)
    {
        // This will block if the buffer is currently being handled by the dma.
        // The advantage of using a queue is that if the dma tranfer ends while
        // a thread with lower priority than the one that is waiting, a context
        // switch will happen.
        tx_queue.put(0);

        {
            FastInterruptDisableLock lock;
            //What if somebody disales serial port while we are transmitting
            if(!serialEnabled) return;

            // If we get here, the dma buffer is ready
            unsigned int transferSize=std::min(len,SOFTWARE_TX_QUEUE);
            memcpy(tx_dma_buffer,str,transferSize);
            // Setup DMA xfer
            DMA1_Channel4->CPAR=reinterpret_cast<unsigned int>(&USART1->DR);
            DMA1_Channel4->CMAR=reinterpret_cast<unsigned int>(tx_dma_buffer);
            DMA1_Channel4->CNDTR=transferSize;
            DMA1_Channel4->CCR=DMA_CCR4_MINC | DMA_CCR4_DIR | DMA_CCR4_TEIE |
                    DMA_CCR4_TCIE | DMA_CCR4_EN;
            len-=transferSize;
            str+=transferSize;
        }
    }
}

void IRQserialWriteString(const char *str)
{
    // Unfortunately, we can't use dma xfer here, since we could be within an
    // interrupt, and therefore the interrupt that signals the end of dma xfer
    // could be missed. Instead, we temporarily disable the dma, send our data
    // and re-enable it after
    if(!serialEnabled) return;

    // We can reach here also with only kernel paused, so make sure interrupts
    // are disabled.
    bool interrupts=areInterruptsEnabled();
    if(interrupts) fastDisableInterrupts();

    while(!IRQserialTxFifoEmpty()) ; //Wait before disabling serial port
    USART1->CR1 &= ~USART_CR1_TE;
    USART1->CR3=0; //Temporarily disable DMA
    USART1->CR1 |= USART_CR1_TE;
    while((*str)!='\0')
    {
        //Wait until the hardware fifo is ready to accept one char
        while((USART1->SR & USART_SR_TXE)==0) ; //Wait until ready
        USART1->DR=*str++;
    }
    while(!IRQserialTxFifoEmpty()) ; //Wait before dsabling serial port
    USART1->CR1 &= ~USART_CR1_TE;
    USART1->CR3 = USART_CR3_DMAT; //Re enable DMA
    USART1->CR1 |= USART_CR1_TE;

    if(interrupts) fastEnableInterrupts();
}

#else //__ENABLE_XRAM

void IRQserialInit()
{
    rxQueue.IRQreset();
    //Enable clock to GPIOA, AFIO and USART1
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN |
            RCC_APB2ENR_USART1EN;
    //Enabled, 8 data bit, no parity, interrupt on character rx
    USART1->CR1 = USART_CR1_UE | USART_CR1_RXNEIE;
    USART1->CR2 = 0;//Disable lin mode and synchronous mode
    USART1->CR3 = 0;//Disable irda and smartcard mode

    //baudrate=19200
    #ifdef SYSCLK_FREQ_72MHz
    USART1->BRR = (234<<4) | (6<<0); //BRR=234.375      0% Error
    #elif SYSCLK_FREQ_56MHz
    USART1->BRR = (182<<4) | (5<<0); //BRR=182.3125 0.011% Error
    #elif SYSCLK_FREQ_48MHz
    USART1->BRR = (156<<4) | (4<<0); //BRR=156.25       0% Error
    #elif SYSCLK_FREQ_36MHz
    USART1->BRR = (117<<4) | (3<<0); //BRR=117.1875     0% Error
    #elif SYSCLK_FREQ_24MHz
    USART1->BRR = ( 78<<4) | (2<<0); //BRR= 78.125      0% Error
    #else //Running on internal 8MHz frequency
    USART1->BRR = ( 26<<4) | (1<<0); //BRR= 26.0625  0.08% Error
    #endif

    //Now that serial port is active, configure I/Os
    Gpio<GPIOA_BASE, 9>::mode(Mode::ALTERNATE);
    Gpio<GPIOA_BASE,10>::mode(Mode::INPUT);
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;//Finally enable tx and rx
    NVIC_SetPriority(USART1_IRQn,10);//Low priority for serial. (Max=0, min=15)
    NVIC_EnableIRQ(USART1_IRQn);
    serialEnabled=true;
}

void IRQserialDisable()
{
    serialEnabled=false;
    while(!IRQserialTxFifoEmpty()) ; //wait
    USART1->CR1=0;
}

void serialWrite(const char *str, unsigned int len)
{
    if(!serialEnabled) return;
    {
        Lock<Mutex> l(txMutex);
        for(unsigned int i=0;i<len;i++)
        {
            while((USART1->SR & USART_SR_TXE)==0)
            {
                //Wait till we can transmit, but while waiting check if
                //someboby disables the serial port while we are transmitting
                if(!serialEnabled) return;
            }
            USART1->DR=*str++;
        }
    }
}

void IRQserialWriteString(const char *str)
{
    if(!serialEnabled) return;
    while((*str)!='\0')
    {
        //Wait until the hardware fifo is ready to accept one char
        while((USART1->SR & USART_SR_TXE)==0) ; //Wait until ready
        USART1->DR=*str++;
    }
}

#endif //__ENABLE_XRAM

bool IRQisSerialEnabled()
{
    return serialEnabled;
}

bool serialTxComplete()
{
    return IRQserialTxFifoEmpty();
}

char serialReadChar()
{
    Lock<Mutex> l(rxMutex);
    char result;
    rxQueue.get(result);
    return result;
}

bool serialReadCharNonblocking(char& c)
{
    Lock<Mutex> l(rxMutex);
    if(rxQueue.isEmpty()==false)
    {
        rxQueue.get(c);
        return true;
    }
    return false;
}

bool serialRxLost()
{
    bool temp=(bool)rxLostFlag;
    rxLostFlag=false;
    return temp;
}

void serialRxFlush()
{
    rxQueue.reset();
}

bool IRQserialTxFifoEmpty()
{
    return (USART1->SR & USART_SR_TC)!=0;
}

}//Namespace miosix
