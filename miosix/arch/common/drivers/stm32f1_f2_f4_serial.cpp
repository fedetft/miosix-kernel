/***************************************************************************
 *   Copyright (C) 2010-2018 by Terraneo Federico                          *
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
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
#include <termios.h>
#include "stm32f1_f2_f4_serial.h"
#include "stm32_serial_common.h"
#include "kernel/sync.h"
#include "filesystem/ioctl.h"
#include "cache/cortexMx_cache.h"
#include "interfaces/interrupts.h"

using namespace std;

namespace miosix {

/*
 * Auxiliary class that encapsulates all parts of code that differ between
 * between each instance of the USART peripheral.
 * 
 * Try not to use the attributes of this class directly even if they are public.
 */

class STM32SerialHW
{
public:
    inline USART_TypeDef *get() const { return port; }
    inline IRQn_Type getIRQn() const { return irq; }
    #if !defined(ALTFUNC_STM32F1)
        inline STM32SerialAltFunc const & getAltFunc() const { return altFunc; }
    #endif
    inline unsigned int IRQgetClock() const { return STM32Bus::getClock(bus); }
    inline void IRQenable() const { STM32Bus::IRQen(bus, clkEnMask); }
    inline void IRQdisable() const { STM32Bus::IRQdis(bus, clkEnMask); }
    inline const STM32SerialDMAHW& getDma() const { return dma; }

    USART_TypeDef *port;        ///< USART port
    IRQn_Type irq;              ///< USART IRQ number
    #if !defined(ALTFUNC_STM32F1)
        STM32SerialAltFunc altFunc; ///< Alternate function to set for GPIOs
    #endif
    STM32Bus::ID bus;           ///< Bus where the port is (APB1 or 2)
    unsigned long clkEnMask;    ///< USART clock enable

    STM32SerialDMAHW dma;
};

/*
 * Table of hardware configurations
 */
#if defined(STM32F100xB) || defined(STM32F103xB)
constexpr int maxPorts = 3;
static const STM32SerialHW ports[maxPorts] = {
    { USART1, USART1_IRQn, STM32Bus::APB2, RCC_APB2ENR_USART1EN,
      { DMA1_Channel4, DMA1_Channel4_IRQn, STM32SerialDMAHW::Channel4,
        DMA1_Channel5, DMA1_Channel5_IRQn, STM32SerialDMAHW::Channel5 } },
    { USART2, USART2_IRQn, STM32Bus::APB1, RCC_APB1ENR_USART2EN,
      { DMA1_Channel7, DMA1_Channel7_IRQn, STM32SerialDMAHW::Channel7,
        DMA1_Channel6, DMA1_Channel6_IRQn, STM32SerialDMAHW::Channel6 } },
    { USART3, USART3_IRQn, STM32Bus::APB1, RCC_APB1ENR_USART3EN,
      { DMA1_Channel2, DMA1_Channel2_IRQn, STM32SerialDMAHW::Channel2,
        DMA1_Channel3, DMA1_Channel3_IRQn, STM32SerialDMAHW::Channel3 } },
};
#elif defined(STM32F103xE)
constexpr int maxPorts = 5;
static const STM32SerialHW ports[maxPorts] = {
    { USART1, USART1_IRQn, STM32Bus::APB2, RCC_APB2ENR_USART1EN,
      { DMA1, STM32Bus::AHB, RCC_AHBENR_DMA1EN,
        DMA1_Channel4, DMA1_Channel4_IRQn, STM32SerialDMAHW::Channel4,
        DMA1_Channel5, DMA1_Channel5_IRQn, STM32SerialDMAHW::Channel5 } },
    { USART2, USART2_IRQn, STM32Bus::APB1, RCC_APB1ENR_USART2EN,
      { DMA1, STM32Bus::AHB, RCC_AHBENR_DMA1EN,
        DMA1_Channel7, DMA1_Channel7_IRQn, STM32SerialDMAHW::Channel7,
        DMA1_Channel6, DMA1_Channel6_IRQn, STM32SerialDMAHW::Channel6 } },
    { USART3, USART3_IRQn, STM32Bus::APB1, RCC_APB1ENR_USART3EN,
      { DMA1, STM32Bus::AHB, RCC_AHBENR_DMA1EN,
        DMA1_Channel2, DMA1_Channel2_IRQn, STM32SerialDMAHW::Channel2,
        DMA1_Channel3, DMA1_Channel3_IRQn, STM32SerialDMAHW::Channel3 } },
    { UART4, UART4_IRQn, STM32Bus::APB1, RCC_APB1ENR_UART4EN,
      { DMA2, STM32Bus::AHB, RCC_AHBENR_DMA2EN,
        DMA2_Channel5, DMA2_Channel4_5_IRQn, STM32SerialDMAHW::Channel5,
        DMA2_Channel3, DMA2_Channel3_IRQn, STM32SerialDMAHW::Channel3 } },
    { UART5, UART5_IRQn, STM32Bus::APB1, RCC_APB1ENR_UART5EN, { 0 } },
};
#elif defined(STM32L151xB) || defined(STM32L152xB)
constexpr int maxPorts = 3;
static const STM32SerialHW ports[maxPorts] = {
    { USART1, USART1_IRQn, {7}, STM32Bus::APB2, RCC_APB2ENR_USART1EN,
      { DMA1_Channel4, DMA1_Channel4_IRQn, STM32SerialDMAHW::Channel4,
        DMA1_Channel5, DMA1_Channel5_IRQn, STM32SerialDMAHW::Channel5 } },
    { USART2, USART2_IRQn, {7}, STM32Bus::APB1, RCC_APB1ENR_USART2EN,
      { DMA1_Channel7, DMA1_Channel7_IRQn, STM32SerialDMAHW::Channel7,
        DMA1_Channel6, DMA1_Channel6_IRQn, STM32SerialDMAHW::Channel6 } },
    { USART3, USART3_IRQn, {7}, STM32Bus::APB1, RCC_APB1ENR_USART3EN,
      { DMA1_Channel2, DMA1_Channel2_IRQn, STM32SerialDMAHW::Channel2,
        DMA1_Channel3, DMA1_Channel3_IRQn, STM32SerialDMAHW::Channel3 } },
};
#elif defined(STM32L151xE) || defined(STM32L152xE)
constexpr int maxPorts = 5;
static const STM32SerialHW ports[maxPorts] = {
    { USART1, USART1_IRQn, {7}, STM32Bus::APB2, RCC_APB2ENR_USART1EN,
      { DMA1, STM32Bus::AHB, RCC_AHBENR_DMA1EN,
        DMA1_Channel4, DMA1_Channel4_IRQn, STM32SerialDMAHW::Channel4,
        DMA1_Channel5, DMA1_Channel5_IRQn, STM32SerialDMAHW::Channel5 } },
    { USART2, USART2_IRQn, {7}, STM32Bus::APB1, RCC_APB1ENR_USART2EN,
      { DMA1, STM32Bus::AHB, RCC_AHBENR_DMA1EN,
        DMA1_Channel7, DMA1_Channel7_IRQn, STM32SerialDMAHW::Channel7,
        DMA1_Channel6, DMA1_Channel6_IRQn, STM32SerialDMAHW::Channel6 } },
    { USART3, USART3_IRQn, {7}, STM32Bus::APB1, RCC_APB1ENR_USART3EN,
      { DMA1, STM32Bus::AHB, RCC_AHBENR_DMA1EN,
        DMA1_Channel2, DMA1_Channel2_IRQn, STM32SerialDMAHW::Channel2,
        DMA1_Channel3, DMA1_Channel3_IRQn, STM32SerialDMAHW::Channel3 } },
    { UART4 , UART4_IRQn , {8}, STM32Bus::APB1, RCC_APB1ENR_UART4EN,
      { DMA2, STM32Bus::AHB, RCC_AHBENR_DMA2EN,
        DMA2_Channel5, DMA2_Channel5_IRQn, STM32SerialDMAHW::Channel5,
        DMA2_Channel3, DMA2_Channel3_IRQn, STM32SerialDMAHW::Channel3 } },
    { UART5 , UART5_IRQn , {8}, STM32Bus::APB1, RCC_APB1ENR_UART5EN,
      { DMA2, STM32Bus::AHB, RCC_AHBENR_DMA2EN,
        DMA2_Channel1, DMA2_Channel1_IRQn, STM32SerialDMAHW::Channel1,
        DMA2_Channel2, DMA2_Channel2_IRQn, STM32SerialDMAHW::Channel2 } },
};
#elif defined(STM32F401xE) || defined(STM32F401xC) || defined(STM32F411xE)
constexpr int maxPorts = 6;
static const STM32SerialHW ports[maxPorts] = {
    { USART1, USART1_IRQn, {7}, STM32Bus::APB2, RCC_APB2ENR_USART1EN,
      { DMA2, STM32Bus::AHB1, RCC_AHB1ENR_DMA2EN,
        DMA2_Stream7, DMA2_Stream7_IRQn, STM32SerialDMAHW::Stream7, 4,
        DMA2_Stream5, DMA2_Stream5_IRQn, STM32SerialDMAHW::Stream5, 4 } },
    { USART2, USART2_IRQn, {7}, STM32Bus::APB1, RCC_APB1ENR_USART2EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream6, DMA1_Stream6_IRQn, STM32SerialDMAHW::Stream6, 4,
        DMA1_Stream5, DMA1_Stream5_IRQn, STM32SerialDMAHW::Stream5, 4 } },
    { 0 },
    { 0 },
    { 0 },
    { USART6, USART6_IRQn, {8}, STM32Bus::APB2, RCC_APB2ENR_USART6EN,
      { DMA2, STM32Bus::AHB1, RCC_AHB1ENR_DMA2EN,
        DMA2_Stream6, DMA2_Stream6_IRQn, STM32SerialDMAHW::Stream6, 5,
        DMA2_Stream1, DMA2_Stream1_IRQn, STM32SerialDMAHW::Stream1, 5 } },
};
#elif defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) \
   || defined(STM32F417xx) || defined(STM32F205xx) || defined(STM32F207xx)
constexpr int maxPorts = 6;
static const STM32SerialHW ports[maxPorts] = {
    { USART1, USART1_IRQn, {7}, STM32Bus::APB2, RCC_APB2ENR_USART1EN,
      { DMA2, STM32Bus::AHB1, RCC_AHB1ENR_DMA2EN,
        DMA2_Stream7, DMA2_Stream7_IRQn, STM32SerialDMAHW::Stream7, 4,
        DMA2_Stream5, DMA2_Stream5_IRQn, STM32SerialDMAHW::Stream5, 4 } },
    { USART2, USART2_IRQn, {7}, STM32Bus::APB1, RCC_APB1ENR_USART2EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream6, DMA1_Stream6_IRQn, STM32SerialDMAHW::Stream6, 4,
        DMA1_Stream5, DMA1_Stream5_IRQn, STM32SerialDMAHW::Stream5, 4 } },
    { USART3, USART3_IRQn, {7}, STM32Bus::APB1, RCC_APB1ENR_USART3EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream3, DMA1_Stream3_IRQn, STM32SerialDMAHW::Stream3, 4,
        DMA1_Stream1, DMA1_Stream1_IRQn, STM32SerialDMAHW::Stream1, 4 } },
    { UART4 , UART4_IRQn , {8}, STM32Bus::APB1, RCC_APB1ENR_UART4EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream4, DMA1_Stream4_IRQn, STM32SerialDMAHW::Stream4, 4,
        DMA1_Stream2, DMA1_Stream2_IRQn, STM32SerialDMAHW::Stream2, 4 } },
    { UART5 , UART5_IRQn , {8}, STM32Bus::APB1, RCC_APB1ENR_UART5EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream7, DMA1_Stream7_IRQn, STM32SerialDMAHW::Stream7, 4,
        DMA1_Stream0, DMA1_Stream0_IRQn, STM32SerialDMAHW::Stream0, 4 } },
    { USART6, USART6_IRQn, {8}, STM32Bus::APB2, RCC_APB2ENR_USART6EN,
      { DMA2, STM32Bus::AHB1, RCC_AHB1ENR_DMA2EN,
        DMA2_Stream6, DMA2_Stream6_IRQn, STM32SerialDMAHW::Stream6, 5,
        DMA2_Stream1, DMA2_Stream1_IRQn, STM32SerialDMAHW::Stream1, 5 } },
};
#elif defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F469xx) \
   || defined(STM32F479xx)
constexpr int maxPorts = 8;
static const STM32SerialHW ports[maxPorts] = {
    { USART1, USART1_IRQn, {7}, STM32Bus::APB2, RCC_APB2ENR_USART1EN,
      { DMA2, STM32Bus::AHB1, RCC_AHB1ENR_DMA2EN,
        DMA2_Stream7, DMA2_Stream7_IRQn, STM32SerialDMAHW::Stream7, 4,
        DMA2_Stream5, DMA2_Stream5_IRQn, STM32SerialDMAHW::Stream5, 4 } },
    { USART2, USART2_IRQn, {7}, STM32Bus::APB1, RCC_APB1ENR_USART2EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream6, DMA1_Stream6_IRQn, STM32SerialDMAHW::Stream6, 4,
        DMA1_Stream5, DMA1_Stream5_IRQn, STM32SerialDMAHW::Stream5, 4 } },
    { USART3, USART3_IRQn, {7}, STM32Bus::APB1, RCC_APB1ENR_USART3EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream3, DMA1_Stream3_IRQn, STM32SerialDMAHW::Stream3, 4,
        DMA1_Stream1, DMA1_Stream1_IRQn, STM32SerialDMAHW::Stream1, 4 } },
    { UART4 , UART4_IRQn , {8}, STM32Bus::APB1, RCC_APB1ENR_UART4EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream4, DMA1_Stream4_IRQn, STM32SerialDMAHW::Stream4, 4,
        DMA1_Stream2, DMA1_Stream2_IRQn, STM32SerialDMAHW::Stream2, 4 } },
    { UART5 , UART5_IRQn , {8}, STM32Bus::APB1, RCC_APB1ENR_UART5EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream7, DMA1_Stream7_IRQn, STM32SerialDMAHW::Stream7, 4,
        DMA1_Stream0, DMA1_Stream0_IRQn, STM32SerialDMAHW::Stream0, 4 } },
    { USART6, USART6_IRQn, {8}, STM32Bus::APB2, RCC_APB2ENR_USART6EN,
      { DMA2, STM32Bus::AHB1, RCC_AHB1ENR_DMA2EN,
        DMA2_Stream6, DMA2_Stream6_IRQn, STM32SerialDMAHW::Stream6, 5,
        DMA2_Stream1, DMA2_Stream1_IRQn, STM32SerialDMAHW::Stream1, 5 } },
    { UART7 , UART7_IRQn , {8}, STM32Bus::APB1, RCC_APB1ENR_UART7EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream1, DMA1_Stream1_IRQn, STM32SerialDMAHW::Stream1, 5,
        DMA1_Stream3, DMA1_Stream3_IRQn, STM32SerialDMAHW::Stream3, 5 } },
    { UART8 , UART8_IRQn , {8}, STM32Bus::APB1, RCC_APB1ENR_UART8EN,
      { DMA1, STM32Bus::AHB1, RCC_AHB1ENR_DMA1EN,
        DMA1_Stream0, DMA1_Stream0_IRQn, STM32SerialDMAHW::Stream0, 5,
        DMA1_Stream6, DMA1_Stream6_IRQn, STM32SerialDMAHW::Stream6, 5 } },
};
#else
#error Unsupported STM32 chip for this serial driver
#endif

//
// class STM32SerialBase
//

// A note on the baudrate/500: the buffer is selected so as to withstand
// 20ms of full data rate. In the 8N1 format one char is made of 10 bits.
// So (baudrate/10)*0.02=baudrate/500
STM32SerialBase::STM32SerialBase(int id, int baudrate, bool flowControl) : 
    flowControl(flowControl), portId(id), rxQueue(rxQueueMin+baudrate/500)
{
    if(id<1 || id>maxPorts) errorHandler(Error::UNEXPECTED);
    port=&ports[id-1];
    if(port->get()==nullptr) errorHandler(Error::UNEXPECTED);
}

void STM32SerialBase::commonInit(int id, int baudrate, GpioPin tx, GpioPin rx,
                    GpioPin rts, GpioPin cts)
{
    #if defined(ALTFUNC_STM32F1)
        //Quirk: stm32f1 rx pin has to be in input mode, while stm32f2 and up
        //want it in ALTERNATE mode. Go figure...
        tx.mode(Mode::ALTERNATE);
        rx.mode(Mode::INPUT_PULL_UP_DOWN);
        rx.pullup(); //Pullup: prevent spurious rx if unconnected
        if(flowControl)
        {
            rts.mode(Mode::ALTERNATE);
            cts.mode(Mode::INPUT);
        }
    #else
        port->getAltFunc().set(tx);
        port->getAltFunc().set(rx,true); //Pullup: prevent spurious rx if unconnected
        if(flowControl)
        {
            port->getAltFunc().set(rts);
            port->getAltFunc().set(cts);
        }
    #endif
    unsigned int freq=port->IRQgetClock();
    unsigned int quot=2*freq/baudrate;      //2*freq for round to nearest
    port->get()->BRR=quot/2 + (quot & 1);   //Round to nearest
    // Some STM32 (i.e. F103xB) have broken one bit sampling mode,
    // so ST "helpfully" removed the register field from the headers.
    #ifdef USART_CR3_ONEBIT
        unsigned long onebit=USART_CR3_ONEBIT;
    #else
        unsigned long onebit=0;
    #endif
    if(flowControl==false) port->get()->CR3 |= onebit;
    else port->get()->CR3 |= onebit | USART_CR3_RTSE | USART_CR3_CTSE;
}

void STM32SerialBase::IRQwrite(const char *str)
{
    while(*str)
    {
        while((port->get()->SR & USART_SR_TXE)==0) ;
        port->get()->DR=*str++;
    }
    waitSerialTxFifoEmpty();
}

inline void STM32SerialBase::waitSerialTxFifoEmpty()
{
    while((port->get()->SR & USART_SR_TC)==0) ;
}

ssize_t STM32SerialBase::readFromRxQueue(void *buffer, size_t size)
{
    Lock<KernelMutex> l(rxMutex);
    char *buf=reinterpret_cast<char*>(buffer);
    size_t result=0;
    FastGlobalIrqLock dLock;
    DeepSleepLock dpLock;
    for(;;)
    {
        //Try to get data from the queue
        for(;result<size;result++)
        {
            if(rxQueue.tryGet(buf[result])==false) break;
            //This is here just not to keep IRQ disabled for the whole loop
            FastGlobalIrqUnlock eLock(dLock);
        }
        if(idle && result>0) break;
        if(result==size) break;
        //Wait for data in the queue
        rxWaiting=Thread::IRQgetCurrentThread();
        do Thread::IRQglobalIrqUnlockAndWait(dLock); while(rxWaiting);
    }
    return result;
}

void STM32SerialBase::rxWakeup()
{
    if(rxWaiting)
    {
        rxWaiting->IRQwakeup();
        rxWaiting=nullptr;
    }
}

int STM32SerialBase::ioctl(int cmd, void* arg)
{
    if(reinterpret_cast<unsigned>(arg) & 0b11) return -EFAULT; //Unaligned
    termios *t=reinterpret_cast<termios*>(arg);
    switch(cmd)
    {
        case IOCTL_SYNC:
            waitSerialTxFifoEmpty();
            return 0;
        case IOCTL_TCGETATTR:
            t->c_iflag=IGNBRK | IGNPAR;
            t->c_oflag=0;
            t->c_cflag=CS8 | (flowControl ? CRTSCTS : 0);
            t->c_lflag=0;
            return 0;
        case IOCTL_TCSETATTR_NOW:
        case IOCTL_TCSETATTR_DRAIN:
        case IOCTL_TCSETATTR_FLUSH:
            //Changing things at runtime unsupported, so do nothing, but don't
            //return error as console_device.h implements some attribute changes
            return 0;
        default:
            return -ENOTTY; //Means the operation does not apply to this descriptor
    }
}

//
// class STM32Serial
//

STM32Serial::STM32Serial(int id, int baudrate, GpioPin tx, GpioPin rx)
    : STM32SerialBase(id,baudrate,false), Device(Device::TTY)
{
    commonInit(id,baudrate,tx,rx,tx,rx); //The last two args will be ignored
}

STM32Serial::STM32Serial(int id, int baudrate, GpioPin tx, GpioPin rx,
                         GpioPin rts, GpioPin cts)
    : STM32SerialBase(id,baudrate,true), Device(Device::TTY)
{
    commonInit(id,baudrate,tx,rx,rts,cts);
}

void STM32Serial::commonInit(int id, int baudrate, GpioPin tx, GpioPin rx,
                             GpioPin rts, GpioPin cts)
{
    GlobalIrqLock dLock;
    port->IRQenable();
    IRQregisterIrq(port->getIRQn(),&STM32Serial::IRQhandleInterrupt,this);
    STM32SerialBase::commonInit(id,baudrate,tx,rx,rts,cts);
    //Enabled, 8 data bit, no parity, interrupt on character rx
    port->get()->CR1 = USART_CR1_UE     //Enable port
                     | USART_CR1_RXNEIE //Interrupt on data received
                     | USART_CR1_IDLEIE //Interrupt on idle line
                     | USART_CR1_TE     //Transmission enbled
                     | USART_CR1_RE;    //Reception enabled
}

ssize_t STM32Serial::readBlock(void *buffer, size_t size, off_t where)
{
    return STM32SerialBase::readFromRxQueue(buffer, size);
}

ssize_t STM32Serial::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<KernelMutex> l(txMutex);
    DeepSleepLock dpLock;
    const char *buf=reinterpret_cast<const char*>(buffer);
    for(size_t i=0;i<size;i++)
    {
        while((port->get()->SR & USART_SR_TXE)==0) ;
        port->get()->DR=*buf++;
    }
    return size;
}

void STM32Serial::IRQhandleInterrupt()
{
    unsigned int status=port->get()->SR;
    char c;
    rxUpdateIdle(status);
    if(status & USART_SR_RXNE)
    {
        //Always read data, since this clears interrupt flags
        c=port->get()->DR;
        //If no error put data in buffer
        if((status & USART_SR_FE)==0)
            if(rxQueuePut(c)==false) /*fifo overflow*/;
    }
    if(status & USART_SR_IDLE)
    {
        c=port->get()->DR; //clears interrupt flags
    }
    if((status & USART_SR_IDLE) || rxQueue.size()>=rxQueueMin)
    {
        //Enough data in buffer or idle line, awake thread
        rxWakeup();
    }
}

void STM32Serial::IRQwrite(const char *str)
{
    STM32SerialBase::IRQwrite(str);
}

STM32Serial::~STM32Serial()
{
    waitSerialTxFifoEmpty();
    {
        GlobalIrqLock dLock;
        port->get()->CR1=0;
        IRQunregisterIrq(port->getIRQn(),&STM32Serial::IRQhandleInterrupt,this);
        port->IRQdisable();
    }
}

//
// class STM32DMASerial
//

STM32DMASerial::STM32DMASerial(int id, int baudrate, GpioPin tx, GpioPin rx)
    : STM32SerialBase(id,baudrate,false), Device(Device::TTY)
{
    commonInit(id,baudrate,tx,rx,tx,rx); //The last two args will be ignored
}

STM32DMASerial::STM32DMASerial(int id, int baudrate, GpioPin tx, GpioPin rx,
            GpioPin rts, GpioPin cts)
    : STM32SerialBase(id,baudrate,true), Device(Device::TTY)
{
    commonInit(id,baudrate,tx,rx,rts,cts);
}

void STM32DMASerial::commonInit(int id, int baudrate, GpioPin tx, GpioPin rx,
                    GpioPin rts, GpioPin cts)
{
    //Check if DMA is supported for this port
    auto dma=port->getDma();
    if(!dma.get()) errorHandler(Error::UNEXPECTED);
    GlobalIrqLock dLock;

    dma.IRQenable();
    IRQregisterIrq(dma.getTxIRQn(),&STM32DMASerial::IRQhandleDmaTxInterrupt,this);
    IRQregisterIrq(dma.getRxIRQn(),&STM32DMASerial::IRQhandleDmaRxInterrupt,this);
    dma.IRQinit();

    port->IRQenable();
    //Lower priority to ensure IRQhandleDmaRxInterrupt() is called before
    //IRQhandleInterrupt(), so that idle is set correctly
    NVIC_SetPriority(port->getIRQn(),defaultIrqPriority+1);
    IRQregisterIrq(port->getIRQn(),&STM32DMASerial::IRQhandleInterrupt,this);

    STM32SerialBase::commonInit(id,baudrate,tx,rx,rts,cts);

    port->get()->CR3 = USART_CR3_DMAT | USART_CR3_DMAR; //Enable USART DMA
    port->get()->CR1 = USART_CR1_UE     //Enable port
                     | USART_CR1_IDLEIE //Interrupt on idle line
                     | USART_CR1_TE     //Transmission enbled
                     | USART_CR1_RE;    //Reception enabled
    IRQstartDmaRead();
}

ssize_t STM32DMASerial::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<KernelMutex> l(txMutex);
    DeepSleepLock dpLock;
    const char *buf=reinterpret_cast<const char*>(buffer);
    size_t remaining=size;
    //If the client-provided buffer is not in CCM, we can use it directly
    //as DMA source memory (zero copy).
    if(isInCCMarea(buf)==false) 
    {
        //We don't use zero copy for the last txBufferSize bytes because in this
        //way we can return from this function a bit earlier.
        //If we returned while the DMA was still reading from the client
        //buffer, and the buffer is immediately rewritten, shit happens
        while(remaining>txBufferSize)
        {
            //DMA is limited to 64K
            size_t transferSize=min<size_t>(remaining-txBufferSize,65535);
            waitDmaWriteEnd();
            startDmaWrite(buf,transferSize);
            buf+=transferSize;
            remaining-=transferSize;
        }
    }
    //DMA out all remaining data through txBuffer
    while(remaining>0)
    {
        size_t transferSize=min(remaining,static_cast<size_t>(txBufferSize));
        waitDmaWriteEnd();
        //Copy to txBuffer only after DMA xfer completed, as the previous
        //xfer may be using the same buffer
        memcpy(txBuffer,buf,transferSize);
        startDmaWrite(txBuffer,transferSize);
        buf+=transferSize;
        remaining-=transferSize;
    }
    #ifdef WITH_DEEP_SLEEP
    //The serial driver by default can return even though the last part of
    //the data is still being transmitted by the DMA. When using deep sleep
    //however the DMA operation needs to be fully enclosed by a deep sleep
    //lock to prevent the scheduler from stopping peripheral clocks.
    waitDmaWriteEnd();
    waitSerialTxFifoEmpty(); //TODO: optimize by doing it only when entering deep sleep
    #endif //WITH_DEEP_SLEEP
    return size;
}

void STM32DMASerial::startDmaWrite(const char *buffer, size_t size)
{
    markBufferBeforeDmaWrite(buffer,size);
    DeepSleepLock dpLock;
    //The TC (Transfer Complete) bit in the Status Register (SR) of the serial
    //port can be reset by writing to it directly, or by first reading it
    //out and then writing to the Data Register (DR).
    //  The waitSerialTxFifoEmpty() function relies on the status of TC to
    //accurately reflect whether the serial port is pushing out bytes or not,
    //so it is extremely important for TC to *only* be reset at the beginning of
    //a transmission.
    //  Since the DMA peripheral only writes to DR and never reads from SR,
    //we must read from SR manually first to ensure TC is cleared as soon as
    //the DMA writes to it -- and not earlier!
    //  The alternative is to zero out TC by hand, but if we do that TC becomes
    //unreliable as a flag. Consider the case in which we are clearing out TC
    //in this routine before configuring the DMA. If the DMA fails to start due
    //to an error, or the CPU is reset before the DMA transfer is started, the
    //USART won't be pushing out bytes but TC will still be zero. As a result,
    //waitSerialTxFifoEmpty() will loop forever waiting for the end of a
    //transfer that is not happening.
    while((port->get()->SR & USART_SR_TXE)==0) ;
    
    dmaTxInProgress=true;
    port->getDma().startDmaWrite(&port->get()->DR,buffer,size);
}

void STM32DMASerial::IRQhandleDmaTxInterrupt()
{
    port->getDma().IRQhandleDmaTxInterrupt();
    dmaTxInProgress=false;
    if(txWaiting==nullptr) return;
    txWaiting->IRQwakeup();
    txWaiting=nullptr;
}

void STM32DMASerial::waitDmaWriteEnd()
{
    FastGlobalIrqLock dLock;
    // If a previous DMA xfer is in progress, wait
    if(dmaTxInProgress)
    {
        txWaiting=Thread::IRQgetCurrentThread();
        do Thread::IRQglobalIrqUnlockAndWait(dLock); while(txWaiting);
    }
}

ssize_t STM32DMASerial::readBlock(void *buffer, size_t size, off_t where)
{
    return STM32SerialBase::readFromRxQueue(buffer, size);
}

void STM32DMASerial::IRQstartDmaRead()
{
    port->getDma().IRQstartDmaRead(&port->get()->DR,rxBuffer,rxQueueMin);
}

int STM32DMASerial::IRQstopDmaRead()
{
    return rxQueueMin - port->getDma().IRQstopDmaRead();
}

void STM32DMASerial::IRQflushDmaReadBuffer()
{
    int elem=IRQstopDmaRead();
    markBufferAfterDmaRead(rxBuffer,rxQueueMin);
    for(int i=0;i<elem;i++)
        if(rxQueue.tryPut(rxBuffer[i])==false) /*fifo overflow*/;
    IRQstartDmaRead();
}

void STM32DMASerial::IRQhandleDmaRxInterrupt()
{
    IRQflushDmaReadBuffer();
    rxUpdateIdle(0);
    rxWakeup();
}

void STM32DMASerial::IRQhandleInterrupt()
{
    unsigned int status=port->get()->SR;
    rxUpdateIdle(status);
    if(status & USART_SR_IDLE)
    {
        (unsigned long)port->get()->DR; //clears interrupt flags
        IRQflushDmaReadBuffer();
    }
    if((status & USART_SR_IDLE) || rxQueue.size()>=rxQueueMin)
    {
        //Enough data in buffer or idle line, awake thread
        rxWakeup();
    }
}

void STM32DMASerial::IRQwrite(const char *str)
{
    //Wait until DMA xfer ends. EN bit is cleared by hardware on transfer end
    port->getDma().IRQwaitDmaWriteStop();
    STM32SerialBase::IRQwrite(str);
}

STM32DMASerial::~STM32DMASerial()
{
    waitSerialTxFifoEmpty();
    {
        GlobalIrqLock dLock;
        port->get()->CR1=0;
        IRQstopDmaRead();
        auto dma=port->getDma();
        IRQunregisterIrq(dma.getTxIRQn(),&STM32DMASerial::IRQhandleDmaTxInterrupt,this);
        IRQunregisterIrq(dma.getRxIRQn(),&STM32DMASerial::IRQhandleDmaRxInterrupt,this);
        IRQunregisterIrq(port->getIRQn(),&STM32DMASerial::IRQhandleInterrupt,this);
        port->IRQdisable();
    }
}

} //namespace miosix
