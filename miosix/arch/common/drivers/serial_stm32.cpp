/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012, 2013, 2014 by Terraneo Federico       *
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
#include <errno.h>
#include "serial_stm32.h"
#include "kernel/sync.h"
#include "kernel/scheduler/scheduler.h"
#include "interfaces/portability.h"
#include "interfaces/gpio.h"
#include "filesystem/ioctl.h"

using namespace std;
using namespace miosix;

static const int numPorts=3; //Supporting only USART1, USART2, USART3

//A nice feature of the stm32 is that the USART are connected to the same
//GPIOS in all families, stm32f1, f2, f4 and l1. Additionally, USART1 is
//always connected to the APB2, while USART2 and USART3 are always on APB1
//Unfortunately, this does not hold with DMA.
typedef Gpio<GPIOA_BASE,9>  u1tx;
typedef Gpio<GPIOA_BASE,10> u1rx;
typedef Gpio<GPIOA_BASE,2>  u2tx;
typedef Gpio<GPIOA_BASE,3>  u2rx;
typedef Gpio<GPIOB_BASE,10> u3tx;
typedef Gpio<GPIOB_BASE,11> u3rx;

/// Pointer to serial port classes to let interrupts access the classes
static STM32Serial *ports[numPorts]={0};

/**
 * \internal interrupt routine for usart1 actual implementation
 */
void __attribute__((noinline)) usart1irqImpl()
{
   if(ports[0]) ports[0]->IRQhandleInterrupt();
}

/**
 * \internal interrupt routine for usart1
 */
void __attribute__((naked)) USART1_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z13usart1irqImplv");
    restoreContext();
}

/**
 * \internal interrupt routine for usart2 actual implementation
 */
void __attribute__((noinline)) usart2irqImpl()
{
   if(ports[1]) ports[1]->IRQhandleInterrupt();
}

/**
 * \internal interrupt routine for usart2
 */
void __attribute__((naked)) USART2_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z13usart2irqImplv");
    restoreContext();
}

/**
 * \internal interrupt routine for usart3 actual implementation
 */
void __attribute__((noinline)) usart3irqImpl()
{
   if(ports[2]) ports[2]->IRQhandleInterrupt();
}

/**
 * \internal interrupt routine for usart3
 */
void __attribute__((naked)) USART3_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z13usart3irqImplv");
    restoreContext();
}

#ifdef SERIAL_1_DMA

/**
 * \internal USART1 DMA tx actual implementation
 */
void __attribute__((noinline)) usart1txDmaImpl()
{
    if(ports[0]) ports[0]->IRQhandleDMAtx();
}

#ifdef _ARCH_CORTEXM3_STM32
/**
 * \internal DMA1 Channel 4 IRQ (configured as USART1 TX)
 */
void __attribute__((naked)) DMA1_Channel4_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart1txDmaImplv");
    restoreContext();
}

#else //stm32f2 and stm32f4

/**
 * \internal DMA2 stream 7 IRQ (configured as USART1 TX)
 */
void __attribute__((naked)) DMA2_Stream7_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart1txDmaImplv");
    restoreContext();
}
#endif
#endif //SERIAL_1_DMA

#ifdef _ARCH_CORTEXM4_STM32F4
/**
 * The STM3F4 has an ugly quirk of having a 64KB RAM area called CCM that can
 * only be accessed by the processor and not be the DMA.
 * \param x pointer to check
 * \return true if the pointer is inside the CCM, and thus it isn't possible
 * to use it for DMA transfers
 */
static bool isInCCMarea(const void *x)
{
    unsigned int ptr=reinterpret_cast<const unsigned int>(x);
    return (ptr>=0x10000000) && (ptr<(0x10000000+64*1024));
}
#else //_ARCH_CORTEXM4_STM32F4
static inline bool isInCCMarea(const void *x) { return false; }
#endif //_ARCH_CORTEXM4_STM32F4

namespace miosix {

//
// class STM32Serial
//

// A note on the baudrate/500: the buffer is selected so as to withstand
// 20ms of full data rate. In the 8N1 format one char is made of 10 bits.
// So (baudrate/10)*0.02=baudrate/500
STM32Serial::STM32Serial(int id, int baudrate)
        : Device(Device::TTY), rxQueue(rxQueueMin+baudrate/500), rxWaiting(0),
        idle(true)
{
    #ifdef SERIAL_1_DMA
    txWaiting=0;
    dmaTxInProgress=false;
    #endif //SERIAL_1_DMA
    InterruptDisableLock dLock;
    if(id<1|| id>numPorts || ports[id-1]!=0) errorHandler(UNEXPECTED);
    ports[id-1]=this;
    unsigned int freq=SystemCoreClock;
    //Quirk: stm32f1 rx pin has to be in input mode, while stm32f2 and up want
    //it in ALTERNATE mode. Go figure...
    #ifdef _ARCH_CORTEXM3_STM32
    Mode::Mode_ rxPinMode=Mode::INPUT;
    #else //_ARCH_CORTEXM3_STM32
    Mode::Mode_ rxPinMode=Mode::ALTERNATE;
    #endif //_ARCH_CORTEXM3_STM32
    switch(id)
    {
        case 1:
            port=USART1;
            RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
            #ifdef SERIAL_1_DMA
            #ifdef _ARCH_CORTEXM3_STM32
            RCC->AHBENR |= RCC_AHBENR_DMA1EN;
            NVIC_SetPriority(DMA1_Channel4_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(DMA1_Channel4_IRQn);
            #else //stm32f2, stm32f4
            RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
            NVIC_SetPriority(DMA2_Stream7_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(DMA2_Stream7_IRQn);
            #endif
            port->CR3=USART_CR3_DMAT;
            #endif //SERIAL_1_DMA
            #ifndef _ARCH_CORTEXM3_STM32
            //Only stm32f2, f4 and l1 have the new alternate function mapping
            u1tx::alternateFunction(7);
            u1rx::alternateFunction(7);
            #endif //_ARCH_CORTEXM3_STM32
            u1tx::mode(Mode::ALTERNATE);
            u1rx::mode(rxPinMode);
            NVIC_SetPriority(USART1_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(USART1_IRQn);
            if(RCC->CFGR & RCC_CFGR_PPRE2_2) freq/=1<<(((RCC->CFGR>>13) & 0x3)+1);
            break;
        case 2:
            port=USART2;
            RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
            #ifndef _ARCH_CORTEXM3_STM32
            //Only stm32f2, f4 and l1 have the new alternate function mapping
            u2tx::alternateFunction(7);
            u2rx::alternateFunction(7);
            #endif //_ARCH_CORTEXM3_STM32
            u2tx::mode(Mode::ALTERNATE);
            u2rx::mode(rxPinMode);
            NVIC_SetPriority(USART2_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(USART2_IRQn);
            if(RCC->CFGR & RCC_CFGR_PPRE1_2) freq/=1<<(((RCC->CFGR>>10) & 0x3)+1);
            break;
        case 3:
            port=USART3;
            RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
            #ifndef _ARCH_CORTEXM3_STM32
            //Only stm32f2, f4 and l1 have the new alternate function mapping
            u3tx::alternateFunction(7);
            u3rx::alternateFunction(7);
            #endif //_ARCH_CORTEXM3_STM32
            u3tx::mode(Mode::ALTERNATE);
            u3rx::mode(rxPinMode);
            NVIC_SetPriority(USART3_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(USART3_IRQn);
            if(RCC->CFGR & RCC_CFGR_PPRE1_2) freq/=1<<(((RCC->CFGR>>10) & 0x3)+1);
            break;
    }
    const unsigned int quot=2*freq/baudrate; //2*freq for round to nearest
    port->BRR=quot/2 + (quot & 1);           //Round to nearest
    //Enabled, 8 data bit, no parity, interrupt on character rx
    port->CR1 = USART_CR1_UE     //Enable port
              | USART_CR1_RXNEIE //Interrupt on data received
              | USART_CR1_IDLEIE //Interrupt on idle line
              | USART_CR1_TE     //Transmission enbled
              | USART_CR1_RE;    //Reception enabled
}

ssize_t STM32Serial::readBlock(void *buffer, size_t size, off_t where)
{
    Lock<FastMutex> l(rxMutex);
    char *buf=reinterpret_cast<char*>(buffer);
    size_t result=0;
    FastInterruptDisableLock dLock;
    for(;;)
    {
        //Try to get data from the queue
        for(;result<size;result++)
        {
            if(rxQueue.tryGet(buf[result])==false) break;
            //This is here just not to keep IRQ disabled for the whole loop
            FastInterruptEnableLock eLock(dLock);
        }
        if(idle && result>0) break;
        if(result==size) break;
        //Wait for data in the queue
        do {
            rxWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        } while(rxWaiting);
    }
    return result;
}

ssize_t STM32Serial::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<FastMutex> l(txMutex);
    const char *buf=reinterpret_cast<const char*>(buffer);
    #ifdef SERIAL_1_DMA
    if(this==ports[0])
    {
        size_t remaining=size;
        if(isInCCMarea(buf)==false)
        {
            //Use zero copy for all but the last txBufferSize bytes, if possible
            while(remaining>txBufferSize)
            {
                //DMA is limited to 64K
                size_t transferSize=min<size_t>(remaining-txBufferSize,65535);
                writeDma(buf,transferSize);
                buf+=transferSize;
                remaining-=transferSize;
            }
        }
        while(remaining>0)
        {
            size_t transferSize=min<size_t>(remaining,txBufferSize);
            memcpy(txBuffer,buf,transferSize);
            writeDma(txBuffer,transferSize);
            buf+=transferSize;
            remaining-=transferSize;
        }
        return size;
    }
    #endif //SERIAL_1_DMA
    for(size_t i=0;i<size;i++)
    {
        while((port->SR & USART_SR_TXE)==0) ;
        port->DR=*buf++;
    }
    return size;
}

void STM32Serial::IRQwrite(const char *str)
{
    // We can reach here also with only kernel paused, so make sure
    // interrupts are disabled. This is important for the DMA case
    bool interrupts=areInterruptsEnabled();
    if(interrupts) fastDisableInterrupts();
    #ifdef SERIAL_1_DMA
    if(this==ports[0])
    {
        #ifdef _ARCH_CORTEXM3_STM32
        //If no DMA transfer is in progress bit EN is zero. Otherwise wait until
        //DMA xfer ends, by waiting for the TC (or TE) interrupt flag
        while((DMA1_Channel4->CCR & DMA_CCR4_EN) &&
              !(DMA1->ISR & (DMA_ISR_TCIF4 | DMA_ISR_TEIF4))) ;
        #else //_ARCH_CORTEXM3_STM32
        //Wait until DMA xfer ends. EN bit is cleared by hardware on transfer end
        while(DMA2_Stream7->CR & DMA_SxCR_EN) ;
        #endif //_ARCH_CORTEXM3_STM32
    }
    #endif //SERIAL_1_DMA
    while(*str)
    {
        while((port->SR & USART_SR_TXE)==0) ;
        port->DR=*str++;
    }
    waitSerialTxFifoEmpty();
    if(interrupts) fastEnableInterrupts();
}

int STM32Serial::ioctl(int cmd, void* arg)
{
    if(cmd==IOCTL_SYNC)
    {
        waitSerialTxFifoEmpty();
        return 0;
    } else return -ENOTTY; //Means the operation does not apply to this descriptor
}

void STM32Serial::IRQhandleInterrupt()
{
    unsigned int status=port->SR;
    char c;
    if(status & USART_SR_RXNE)
    {
        //Always read data, since this clears interrupt flags
        c=port->DR;
        //If no noise nor framing nor parity error put data in buffer
        if((status & 0x7)==0) if(rxQueue.tryPut(c)==false) /*fifo overflow*/;
        idle=false;
    }
    if(status & USART_SR_IDLE)
    {
        c=port->DR; //clears interrupt flags
        idle=true;
    }
    if((status & USART_SR_IDLE) || rxQueue.size()>=rxQueueMin)
    {
        //Enough data in buffer or idle line, awake thread
        if(rxWaiting)
        {
            rxWaiting->IRQwakeup();
            if(rxWaiting->IRQgetPriority()>
                Thread::IRQgetCurrentThread()->IRQgetPriority())
                    Scheduler::IRQfindNextThread();
            rxWaiting=0;
        }
    }
}

#ifdef SERIAL_1_DMA
void STM32Serial::IRQhandleDMAtx()
{
    #ifdef _ARCH_CORTEXM3_STM32
    DMA1->IFCR=DMA_IFCR_CGIF4;
    DMA1_Channel4->CCR=0; //Disable DMA
    #else //stm32f2 and f4
    DMA2->HIFCR=DMA_HIFCR_CTCIF7
              | DMA_HIFCR_CTEIF7
              | DMA_HIFCR_CDMEIF7
              | DMA_HIFCR_CFEIF7;
    #endif
    dmaTxInProgress=false;
    if(txWaiting==0) return;
    txWaiting->IRQwakeup();
    if(txWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    txWaiting=0;
}
#endif //SERIAL_1_DMA

STM32Serial::~STM32Serial()
{
    waitSerialTxFifoEmpty();
    {
        InterruptDisableLock dLock;
        port->CR1=0;
        
        int id=0;
        for(int i=0;i<numPorts;i++) if(ports[i]==this) id=i+1;
        if(id==0) errorHandler(UNEXPECTED);
        ports[id-1]=0;
        switch(id)
        {
            case 1:
                #ifdef SERIAL_1_DMA
                #ifdef _ARCH_CORTEXM3_STM32
                NVIC_DisableIRQ(DMA1_Channel4_IRQn);
                #else //stm32f2, stm32f4
                NVIC_DisableIRQ(DMA2_Stream7_IRQn);
                #endif
                #endif //SERIAL_1_DMA
                NVIC_DisableIRQ(USART1_IRQn);
                u1tx::mode(Mode::INPUT);
                u1rx::mode(Mode::INPUT);
                RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
                break;
            case 2:
                NVIC_DisableIRQ(USART2_IRQn);
                u2tx::mode(Mode::INPUT);
                u2rx::mode(Mode::INPUT);
                RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN;
                break;
            case 3:
                NVIC_DisableIRQ(USART3_IRQn);
                u3tx::mode(Mode::INPUT);
                u3rx::mode(Mode::INPUT);
                RCC->APB1ENR &= ~RCC_APB1ENR_USART3EN;
                break;
        }
    }
}

#ifdef SERIAL_1_DMA
void STM32Serial::writeDma(const char *buffer, size_t size)
{
    FastInterruptDisableLock dLock;
    // If a previous DMA xfer is in progress, wait
    if(dmaTxInProgress)
    {
        txWaiting=Thread::IRQgetCurrentThread();
        do {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        } while(txWaiting);
    }
    // Setup DMA xfer
    dmaTxInProgress=true;
    #ifdef _ARCH_CORTEXM3_STM32
    DMA1_Channel4->CPAR=reinterpret_cast<unsigned int>(&port->DR);
    DMA1_Channel4->CMAR=reinterpret_cast<unsigned int>(buffer);
    DMA1_Channel4->CNDTR=size;
    DMA1_Channel4->CCR=DMA_CCR4_MINC  //Increment RAM pointer
                     | DMA_CCR4_DIR   //Memory to peripheral
                     | DMA_CCR4_TEIE  //Interrupt on transfer error
                     | DMA_CCR4_TCIE  //Interrupt on transfer complete
                     | DMA_CCR4_EN;   //Start DMA
    #else //_ARCH_CORTEXM3_STM32
    DMA2_Stream7->PAR=reinterpret_cast<unsigned int>(&port->DR);
    DMA2_Stream7->M0AR=reinterpret_cast<unsigned int>(buffer);
    DMA2_Stream7->NDTR=size;
    //Quirk: not enabling DMA_SxFCR_FEIE because the USART seems to
    //generate a spurious fifo error. The code was tested and the
    //transfer completes successfully even in the presence of this fifo
    //error
    DMA2_Stream7->FCR=DMA_SxFCR_DMDIS;//Enable fifo
    DMA2_Stream7->CR=DMA_SxCR_CHSEL_2 //Select channel 4 (USART_TX)
                   | DMA_SxCR_MINC    //Increment RAM pointer
                   | DMA_SxCR_DIR_0   //Memory to peripheral
                   | DMA_SxCR_TCIE    //Interrupt on completion
                   | DMA_SxCR_TEIE    //Interrupt on transfer error
                   | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                   | DMA_SxCR_EN;     //Start the DMA
    #endif //_ARCH_CORTEXM3_STM32
}
#endif //SERIAL_1_DMA

} //namespace miosix
