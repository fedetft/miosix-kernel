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
#include "serial.h"
#include "kernel/sync.h"
#include "kernel/scheduler/scheduler.h"
#include "interfaces/portability.h"
#include "interfaces/gpio.h"
#include "filesystem/ioctl.h"

using namespace miosix;

static const unsigned int numPorts=3;

//A nice feature of the stm32 is that the USART are connected to the same
//GPIOS in all families, stm32f1, f2, f4 and l1. Additionally, USART1 is
//always connected to the APB2, while USART2 and USART3 are always on APB1
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

namespace miosix {

//
// class STM32Serial
//

STM32Serial::STM32Serial(int id, int baudrate) : Device(Device::TTY)
{
    InterruptDisableLock dLock;
    if(id<1|| id>numPorts || ports[id-1]!=0) errorHandler(UNEXPECTED);
    ports[id-1]=this;
    unsigned int freq=SystemCoreClock;
    switch(id)
    {
        case 1:
            RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
            port=USART1;
            #ifndef _ARCH_CORTEXM3_STM32
            //Only stm32f2, f4 and l1 have the new alternate function mapping
            u1tx::alternateFunction(7);
            u1rx::alternateFunction(7);
            #endif //_ARCH_CORTEXM3_STM32
            u1tx::mode(Mode::ALTERNATE);
            u1rx::mode(Mode::ALTERNATE);
            NVIC_SetPriority(USART1_IRQn,10); //Low priority (Max=0, min=15)
            NVIC_EnableIRQ(USART1_IRQn);
            if(RCC->CFGR & RCC_CFGR_PPRE2_2) freq/=1<<(((RCC->CFGR>>13) & 0x3)+1);
            break;
        case 2:
            RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
            port=USART2;
            #ifndef _ARCH_CORTEXM3_STM32
            //Only stm32f2, f4 and l1 have the new alternate function mapping
            u2tx::alternateFunction(7);
            u2rx::alternateFunction(7);
            #endif //_ARCH_CORTEXM3_STM32
            u2tx::mode(Mode::ALTERNATE);
            u2rx::mode(Mode::ALTERNATE);
            NVIC_SetPriority(USART2_IRQn,10); //Low priority (Max=0, min=15)
            NVIC_EnableIRQ(USART2_IRQn);
            if(RCC->CFGR & RCC_CFGR_PPRE1_2) freq/=1<<(((RCC->CFGR>>10) & 0x3)+1);
            break;
        case 3:
            RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
            port=USART3;
            #ifndef _ARCH_CORTEXM3_STM32
            //Only stm32f2, f4 and l1 have the new alternate function mapping
            u3tx::alternateFunction(7);
            u3rx::alternateFunction(7);
            #endif //_ARCH_CORTEXM3_STM32
            u3tx::mode(Mode::ALTERNATE);
            u3rx::mode(Mode::ALTERNATE);
            NVIC_SetPriority(USART3_IRQn,10); //Low priority (Max=0, min=15)
            NVIC_EnableIRQ(USART3_IRQn);
            if(RCC->CFGR & RCC_CFGR_PPRE1_2) freq/=1<<(((RCC->CFGR>>10) & 0x3)+1);
            break;
    }
    const unsigned int quot=2*freq/baudrate; //2*freq for round to nearest
    port->BRR=quot/2 + (quot & 1);           //Round to nearest
    //Enabled, 8 data bit, no parity, interrupt on character rx
    port->CR1 = USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE;
}

ssize_t STM32Serial::readBlock(void *buffer, size_t size, off_t where)
{
    Lock<Mutex> l(rxMutex);
    char *buf=reinterpret_cast<char*>(buffer);
    for(size_t i=0;i<size;i++) rxQueue.get(buf[i]);
    return size;
}

ssize_t STM32Serial::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<Mutex> l(txMutex);
    const char *buf=reinterpret_cast<const char *>(buffer);
    for(size_t i=0;i<size;i++)
    {
        while((port->SR & USART_SR_TXE)==0) ;
        port->DR=*buf++;
    }
    return size;
}

void STM32Serial::IRQwrite(const char *str)
{
    while(*str)
    {
        while((port->SR & USART_SR_TXE)==0) ;
        port->DR=*str++;
    }
    waitSerialTxFifoEmpty();
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
    bool hppw=false;
    unsigned int status=port->SR;
    if(status & (USART_SR_RXNE | USART_SR_ORE))
    {
        //Always read data, since this clears interrupt flags
        char c=port->DR;
        //If no noise nor framing nor parity error put data in queue
        if((status & USART_SR_RXNE) && ((status & 0x7)==0)) rxQueue.IRQput(c,hppw);
    }
    if(hppw) Scheduler::IRQfindNextThread();
}

STM32Serial::~STM32Serial()
{
    waitSerialTxFifoEmpty();
    
    InterruptDisableLock dLock;
    port->CR1=0;
    
    int id=0;
    for(int i=0;i<numPorts;i++) if(ports[i]==this) id=i+1;
    if(id==0) errorHandler(UNEXPECTED);
    ports[id-1]=0;
    switch(id)
    {
        case 1:
            NVIC_DisableIRQ(USART1_IRQn);
            u1tx::mode(Mode::INPUT_PULL_UP);
            u1rx::mode(Mode::INPUT);
            RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
            break;
        case 2:
            NVIC_DisableIRQ(USART2_IRQn);
            u2tx::mode(Mode::INPUT_PULL_UP);
            u2rx::mode(Mode::INPUT);
            RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN;
            break;
        case 3:
            NVIC_DisableIRQ(USART3_IRQn);
            u3tx::mode(Mode::INPUT_PULL_UP);
            u3rx::mode(Mode::INPUT);
            RCC->APB1ENR &= ~RCC_APB1ENR_USART3EN;
            break;
    }
}

} //namespace miosix
