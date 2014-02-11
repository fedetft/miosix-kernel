/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014                *
 *   by Terraneo Federico                                                  *
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
#include "filesystem/ioctl.h"

namespace miosix {

/// Pointer to serial port classes to let interrupts access the classes
static LPC2000Serial *ports[2]={0};

/**
 * \internal interrupt routine for usart0 actual implementation
 */
void __attribute__((noinline)) usart0irqImpl()
{
    if(ports[0]) ports[0]->IRQhandleInterrupt();
    VICVectAddr=0xff;//Restart VIC
}

/**
 * \internal interrupt routine for usart0
 */
void __attribute__((interrupt("IRQ"),naked)) usart0irq()
{
    saveContextFromIrq();
	asm volatile("bl _ZN6miosix13usart0irqImplEv");
    restoreContext();
}

///**
// * \internal interrupt routine for usart1 actual implementation
// */
//void __attribute__((noinline)) usart1irqImpl()
//{
//    if(ports[1]) ports[1]->IRQhandleInterrupt();
//    VICVectAddr=0xff;//Restart VIC
//}
//
///**
// * \internal interrupt routine for usart1
// */
//void __attribute__ ((interrupt("IRQ"),naked)) usart1irq()
//{
//    saveContextFromIrq();
//	asm volatile("bl _ZN6miosix13usart1irqImplEv");
//    restoreContext();
//}

//
// class LPC2000Serial
//

LPC2000Serial::LPC2000Serial(/*int id,*/ int baudrate) : Device(false,false,true)
{
    InterruptDisableLock dLock;
    int id=0;//FIXME
    if(id<0 || id>1 || ports[id]!=0) errorHandler(UNEXPECTED);
    ports[id]=this;
    PCONP|=(1<<3);//Enable UART0 peripheral
    U0LCR=0x3;//DLAB disabled
    //0x07= fifo enabled, reset tx and rx hardware fifos
    //0x80= uart rx fifo trigger level 8 characters
    U0FCR=0x07 | 0x80;
    U0LCR=0x83;//8data bit, 1stop, no parity, DLAB enabled
    int div=TIMER_CLOCK/16/baudrate;
    U0DLL=div & 0xff;
    U0DLM=(div>>8) & 0xff;
    U0LCR=0x3;//DLAB disabled
    U0IER=0x7;//Enable RLS, RDA, CTI and THRE interrupts
    PINSEL0&=~(0xf);//Clear bits 0 to 3 of PINSEL0
    PINSEL0|=0x5;//Set p0.0 as TXD and p0.1 as RXD
    //Init VIC
    VICSoftIntClr=(1<<6);//Clear uart0 interrupt flag (if previously set)
    VICIntSelect&=~(1<<6);//uart0=IRQ
    VICIntEnable=(1<<6);//uart0 interrupt ON
    //Changed to slot 2 since slot 1 is used by auxiliary timer.
    VICVectCntl2=0x20 | 0x6;//Slot 2 of VIC used by uart0
//    VICVectAddr2=id==0 ? &usart0irq : &usart1irq;
    VICVectAddr2=reinterpret_cast<unsigned int>(&usart0irq);
}

ssize_t LPC2000Serial::readBlock(void *buffer, size_t size, off_t where)
{
    Lock<Mutex> l(rxMutex);
    char *buf=reinterpret_cast<char*>(buffer);
    for(size_t i=0;i<size;i++) rxQueue.get(buf[i]);
    return size;
}

ssize_t LPC2000Serial::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<Mutex> l(txMutex);
    FastInterruptDisableLock dLock;
    size_t len=size;
    const char *buf=reinterpret_cast<const char*>(buffer);
    while(len>0)
    {
        //If no data in software and hardware queue
        if((U0LSR & (1<<5)) && (txQueue.isEmpty()))
        {
            //Fill hardware queue first
            for(int i=0;i<HARDWARE_TX_QUEUE_LENGTH;i++)
            {
                U0THR=*buf++;
                len--;
                if(len==0) break;
            }
        } else {
            if(txQueue.IRQput(*buf)==true)
            {
                buf++;
                len--;
            } else {
                FastInterruptEnableLock eLock(dLock);
                txQueue.waitUntilNotFull();
            }
        }
    }
    return size;
}

void LPC2000Serial::IRQwrite(const char *str)
{
    while((*str)!='\0')
    {
        //Wait until the hardware fifo is ready to accept one char
        while(!(U0LSR & (1<<5))) ; //wait
        U0THR=*str;
        str++;
    }
    while(!serialTxFifoEmpty()) ; //wait
}

int LPC2000Serial::ioctl(int cmd, void* arg)
{
    if(cmd==IOCTL_SYNC) while(!serialTxFifoEmpty()) ; //wait
    else return -ENOTTY; //Means the operation does not apply to this descriptor
}

void LPC2000Serial::IRQhandleInterrupt()
{
    char c;
    bool hppw=false;
    switch(U0IIR & 0xf)
    {
        case 0x6: //RLS
            c=U0LSR;//Read U0LSR to clear interrupt
            c=U0RBR;//Read U0RBR to discard char that caused error
            break;
        case 0x4: //RDA
            for(int i=0;i<HARDWARE_RX_QUEUE_LENGTH;i++)
                if(rxQueue.IRQput(U0RBR,hppw)==false) /*fifo overflow*/;
        case 0xc: //CTI
            while(U0LSR & (1<<0))
                if(rxQueue.IRQput(U0RBR,hppw)==false) /*fifo overflow*/;
            break;
        case 0x2: //THRE
            for(int i=0;i<HARDWARE_TX_QUEUE_LENGTH;i++)
            {
                //If software queue empty, stop
                if(txQueue.IRQget(c,hppw)==false) break;
                U0THR=c;
            }
            break;
    }
    if(hppw) Scheduler::IRQfindNextThread();
}

LPC2000Serial::~LPC2000Serial()
{
    while(!(txQueue.isEmpty() && serialTxFifoEmpty())) ; //wait
    
    InterruptDisableLock dLock;
    int id;
    if(ports[0]==this) id=0;
    else if(ports[1]==this) id=1;
    else errorHandler(UNEXPECTED);
    //Disable VIC
    VICIntEnClr=(1<<6);
    //Disable UART0
    U0LCR=0;//DLAB disabled
    U0FCR=0;
    //Disable PIN
    PINSEL0&=~(0xf);//Clear bits 0 to 3 of PINSEL0
}

} //namespace miosix
