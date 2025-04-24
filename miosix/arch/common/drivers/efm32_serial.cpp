/***************************************************************************
 *   Copyright (C) 2015-2024 by Terraneo Federico                          *
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
#include "efm32_serial.h"
#include "kernel/sync.h"
#include "interfaces/interrupts.h"
#include "filesystem/ioctl.h"

namespace miosix {

//
// class EFM32Serial
//

// A note on the baudrate/500: the buffer is selected so as to withstand
// 20ms of full data rate. In the 8N1 format one char is made of 10 bits.
// So (baudrate/10)*0.02=baudrate/500
EFM32Serial::EFM32Serial(int id, int baudrate, GpioPin tx, GpioPin rx)
        : Device(Device::TTY), rxQueue(rxQueueMin+baudrate/500), rxWaiting(0),
          baudrate(baudrate)
{
    {
        GlobalIrqLock dLock;
        tx.mode(Mode::OUTPUT_HIGH);
        rx.mode(Mode::INPUT_PULL_UP_FILTER);
        switch(id)
        {
            case 0:
                port=USART0;
                irqn=USART0_RX_IRQn;
                CMU->HFPERCLKEN0|=CMU_HFPERCLKEN0_USART0;
                port->IRCTRL=0; //USART0 also has IrDA mode
                break;
            case 1:
                port=USART1;
                irqn=USART1_RX_IRQn;
                CMU->HFPERCLKEN0|=CMU_HFPERCLKEN0_USART1;
                break;
            default:
                errorHandler(Error::UNEXPECTED);
        }
        IRQregisterIrq(irqn,&EFM32Serial::IRQinterruptHandler,this);
    }
    
    port->IEN=USART_IEN_RXDATAV;
    port->CTRL=USART_CTRL_TXBIL_HALFFULL; //Use the buffer more efficiently
    port->FRAME=USART_FRAME_STOPBITS_ONE
              | USART_FRAME_PARITY_NONE
              | USART_FRAME_DATABITS_EIGHT;
    port->TRIGCTRL=0;
    #ifdef _ARCH_CORTEXM3_EFM32GG
    port->INPUT=0;
    port->I2SCTRL=0;
    #endif //_ARCH_CORTEXM3_EFM32GG
    port->ROUTE=USART_ROUTE_LOCATION_LOC0 //Default location, hardcoded for now!
              | USART_ROUTE_TXPEN         //Enable TX pin
              | USART_ROUTE_RXPEN;        //Enable RX pin
    //The number we need is periphClock/baudrate/16-1, but with two bits of
    //fractional part. We divide by 2 instead of 16 to have 3 bit of fractional
    //part. We use the additional fractional bit to add one to round towards
    //the nearest. This gets us a little more precision. Then we subtract 8
    //which is one with three fractional bits. Then we shift to fit the integer
    //part in bits 20:8 and the fractional part in bits 7:6, masking away the
    //third fractional bit. Easy, isn't it? Not quite.
    port->CLKDIV=((((peripheralFrequency/baudrate/2)+1)-8)<<5) & 0x1fffc0;
    port->CMD=USART_CMD_CLEARRX
            | USART_CMD_CLEARTX
            | USART_CMD_TXTRIDIS
            | USART_CMD_RXBLOCKDIS
            | USART_CMD_MASTEREN
            | USART_CMD_TXEN
            | USART_CMD_RXEN;
}

ssize_t EFM32Serial::readBlock(void *buffer, size_t size, off_t where)
{
    Lock<KernelMutex> l(rxMutex);
    char *buf=reinterpret_cast<char*>(buffer);
    size_t result=0;
    FastGlobalIrqLock dLock;
    for(;;)
    {
        //Try to get data from the queue
        for(;result<size;result++)
        {
            if(rxQueue.tryGet(buf[result])==false) break;
            //This is here just not to keep IRQ disabled for the whole loop
            FastGlobalIrqUnlock eLock(dLock);
        }
        //Don't block if we have at least one char
        //This is required for \n detection
        if(result>0) break; 
        //Wait for data in the queue
        rxWaiting=Thread::IRQgetCurrentThread();
        do Thread::IRQglobalIrqUnlockAndWait(dLock); while(rxWaiting);
    }
    return result;
}

ssize_t EFM32Serial::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<KernelMutex> l(txMutex);
    const char *buf=reinterpret_cast<const char*>(buffer);
    for(size_t i=0;i<size;i++)
    {
        while((port->STATUS & USART_STATUS_TXBL)==0) ;
        port->TXDATA=*buf++;
    }
    return size;
}

void EFM32Serial::IRQwrite(const char *str)
{
    while(*str)
    {
        while((port->STATUS & USART_STATUS_TXBL)==0) ;
        port->TXDATA=*str++;
    }
    waitSerialTxFifoEmpty();
}

int EFM32Serial::ioctl(int cmd, void* arg)
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
            t->c_cflag=CS8;
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

EFM32Serial::~EFM32Serial()
{
    waitSerialTxFifoEmpty();
    
    GlobalIrqLock dLock;
    port->CMD=USART_CMD_TXDIS
            | USART_CMD_RXDIS;
    port->ROUTE=0;
    IRQunregisterIrq(irqn,&EFM32Serial::IRQinterruptHandler,this);
    if(port==USART0) CMU->HFPERCLKEN0 &= ~CMU_HFPERCLKEN0_USART0;
    else             CMU->HFPERCLKEN0 &= ~CMU_HFPERCLKEN0_USART1;
}

void EFM32Serial::waitSerialTxFifoEmpty()
{
    //The documentation states that the TXC bit goes to one as soon as a
    //transmission is complete. However, this bit is initially zero, so if we
    //call this function before transmitting, the loop will wait forever. As a
    //solution, add a timeout having as value the time needed to send three
    //bytes (the current one in the shift register plus the two in the buffer).
    //The +1 is to produce rounding on the safe side, the 30 is the time to send
    //three char through the port, including start and stop bits.
    int timeout=(cpuFrequency/baudrate+1)*30;
    while(timeout-->0 && (port->STATUS & USART_STATUS_TXC)==0) ;
}

void EFM32Serial::IRQinterruptHandler()
{
    bool atLeastOne=false;
    while(port->STATUS & USART_STATUS_RXDATAV)
    {
        unsigned int c=port->RXDATAX;
        if((c & (USART_RXDATAX_FERR | USART_RXDATAX_PERR))==0)
        {
            atLeastOne=true;
            if(rxQueue.tryPut(c & 0xff)==false) /*fifo overflow*/;
        }
    }
    if(atLeastOne && rxWaiting)
    {
        rxWaiting->IRQwakeup();
        rxWaiting=nullptr;
    }
}

} //namespace miosix
