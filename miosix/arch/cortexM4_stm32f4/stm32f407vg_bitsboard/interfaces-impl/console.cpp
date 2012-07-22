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

/**
 * The stm32f4discovery unfortunately has one of the two GPIOs of USART1
 * used as VBUS for the USB interface. This pin is connected with a 4.7uF
 * capacitor to ground, so USART1 is impossible to use on this board.
 * This (simple) driver falls back to using USART3.
 * PB10 is USART3_TX, while PB11 USART3_RX
 */

#include "interfaces/console.h"
#include "console-impl.h"
#include "kernel/sync.h"
#include "kernel/scheduler/scheduler.h"
#include "kernel/logging.h"
#include "interfaces/arch_registers.h"
#include "interfaces/portability.h"
#include "interfaces/gpio.h"
#include "board_settings.h"
#include "drivers/dcc.h"
#include <cstring>

#ifndef STDOUT_REDIRECTED_TO_DCC

/**
 * \internal
 * serial port interrupt routine
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in serial_irq_impl()
 */
void USART3_IRQHandler() __attribute__ ((naked));
void USART3_IRQHandler()
{
    saveContext();
	asm volatile("bl _ZN6miosix13serialIrqImplEv");
    restoreContext();
}

namespace miosix {

static Mutex rxMutex;///<\internal Mutex used to guard the rx queue
static Mutex txMutex;///<\internal Mutex used to guard the tx queue
const unsigned int SOFTWARE_RX_QUEUE=32;///<\internal Size of rx software queue
static Queue<char,SOFTWARE_RX_QUEUE> rxQueue;///<\internal Rx software queue

/**
 * \internal
 * Serial interrupt code. Declared noinline to avoid the compiler trying to
 * inline it into the caller, which would violate the requirement on naked
 * functions.
 * Function is not static because otherwise the compiler optimizes it out...
 */
void serialIrqImpl() __attribute__((noinline));
void serialIrqImpl()
{
    bool hppw=false;
    unsigned int status=USART3->SR;
    if(status & USART_SR_ORE) //Receiver overrun
    {
        char c=USART3->DR; //Always read data, since this clears interrupt flags
        if((status & USART_SR_RXNE) && ((status & 0x7)==0))
            rxQueue.IRQput(c,hppw);
    } else if(status & USART_SR_RXNE) //Receiver data available
    {
        char c=USART3->DR; //Always read data, since this clears interrupt flags
        if((status & 0x7)==0) //If no noise nor framing nor parity error
        {
            rxQueue.IRQput(c,hppw);
        }
    }
    if(hppw) Scheduler::IRQfindNextThread();
}

void IRQBitsBoardSerialPortInit()
{
    rxQueue.IRQreset();
    //Enable clock to USART3
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    //Enabled, 8 data bit, no parity, interrupt on character rx
    USART3->CR1 = USART_CR1_UE | USART_CR1_RXNEIE;
    USART3->CR2 = 0;//Disable lin mode and synchronous mode
    USART3->CR3 = 0;//Disable irda and smartcard mode
    
    #ifdef SYSCLK_FREQ_168MHz
    //USART3 is connected to APB1 @ 42MHz
    const unsigned int brr = (136<<4) | (11<<0); //BRR=136.6875 0.023% Error
    #else
    #warning "No serial baudrate for this clock frequency"
    #endif
    //baudrate=19200
    USART3->BRR=brr;
    
    //Now that serial port is active, configure I/Os
    Gpio<GPIOB_BASE,10>::alternateFunction(7);
    Gpio<GPIOB_BASE,10>::mode(Mode::ALTERNATE); //PB.10 = USART3 tx
    Gpio<GPIOB_BASE,11>::alternateFunction(7);
    Gpio<GPIOB_BASE,11>::mode(Mode::ALTERNATE); //PB.11 = USART3 rx
    USART3->CR1 |= USART_CR1_TE | USART_CR1_RE;//Finally enable tx and rx
    NVIC_SetPriority(USART3_IRQn,10);//Low priority for serial. (Max=0, min=15)
    NVIC_EnableIRQ(USART3_IRQn);
}

static void serialWriteImpl(const char *str, unsigned int len)
{
    {
        Lock<Mutex> l(txMutex);
        for(unsigned int i=0;i<len;i++)
        {
            while((USART3->SR & USART_SR_TXE)==0) ;
            USART3->DR=*str++;
        }
    }
}

void Console::write(const char *str)
{
    serialWriteImpl(str,std::strlen(str));
}

void Console::write(const char *data, int length)
{
    serialWriteImpl(data,length);
}

bool Console::txComplete()
{
    return (USART3->SR & USART_SR_TC)!=0;
}

void Console::IRQwrite(const char *str)
{
    while((*str)!='\0')
    {
        //Wait until the hardware fifo is ready to accept one char
        while((USART3->SR & USART_SR_TXE)==0) ; //Wait until ready
        USART3->DR=*str++;
    }
}

bool Console::IRQtxComplete()
{
    return (USART3->SR & USART_SR_TC)!=0;
}

char Console::readChar()
{
    Lock<Mutex> l(rxMutex);
    char result;
    rxQueue.get(result);
    return result;
}

bool Console::readCharNonBlocking(char& c)
{
    Lock<Mutex> l(rxMutex);
    if(rxQueue.isEmpty()==false)
    {
        rxQueue.get(c);
        return true;
    }
    return false;
}

} // namespace miosix

#else //STDOUT_REDIRECTED_TO_DCC

namespace miosix {

void Console::write(const char *str)
{
    debugWrite(str,std::strlen(str));
}

void Console::write(const char *data, int length)
{
    debugWrite(data,length);
}

bool Console::txComplete()
{
    return true;
}

void Console::IRQwrite(const char *str)
{
    IRQdebugWrite(str);
}

bool Console::IRQtxComplete()
{
    return true;
}

char Console::readChar()
{
    errorLog("***stdin is not available in DCC");
    for(;;) ;
    return 0; //Only to avoid a compiler warning
}

bool Console::readCharNonBlocking(char& c)
{
    return false;
}

} //namespace miosix

#endif //STDOUT_REDIRECTED_TO_DCC
