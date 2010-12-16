/***************************************************************************
 *   Copyright (C) 2008 by Terraneo Federico                               *
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
#include "serial.h"
#include "LPC213x.h"
#include "kernel/sync.h"
#include "interfaces/portability.h"

namespace miosix {

//Configure the software queue here
const int SOFTWARE_TX_QUEUE=32;///<\internal Size of tx software queue
const int SOFTWARE_RX_QUEUE=32;///<\internal Size of rx software queue

//The hardware queues cannot be modified, since their length is hardware-specific
const int HARDWARE_TX_QUEUE_LENGTH=16;
const int HARDWARE_RX_QUEUE_LENGTH=8;

//Static (local) variables
static Mutex tx_mutex;///<\internal Mutex used to guard the tx queue
static Mutex rx_mutex;///<\internal Mutex used to guard the rx queue

static Queue<char,SOFTWARE_TX_QUEUE> tx_queue;///<\internal Tx software queue
static Queue<char,SOFTWARE_RX_QUEUE> rx_queue;///<\internal Rx software queue

///\internal True if a rx character found the queue full
static volatile bool rx_lost_flag=false;
static bool serial_enabled=false;///<\internal True if serial port is enabled 

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
    char c;
    int i;
    bool hppw=false;
    switch(U0IIR & 0xf)
    {
        case 0x6: //RLS
            c=U0LSR;//Read U0LSR to clear interrupt
            c=U0RBR;//Read U0RBR to discard char that caused error
            break;
        case 0x4: //RDA
            for(i=0;i<HARDWARE_RX_QUEUE_LENGTH;i++)
            {
                if(rx_queue.IRQput(U0RBR,hppw)==false) rx_lost_flag=true;
            }
        case 0xc: //CTI
            while(U0LSR & (1<<0))
            {
                if(rx_queue.IRQput(U0RBR,hppw)==false) rx_lost_flag=true;
            }
            break;
        case 0x2: //THRE
            for(i=0;i<HARDWARE_TX_QUEUE_LENGTH;i++)
            {
                //If software queue empty, stop
                if(tx_queue.IRQget(c,hppw)==false) break;
                U0THR=c;
            }
            break;
    }
    if(hppw) Scheduler::IRQfindNextThread();
    VICVectAddr=0xff;//Restart VIC
}


/**
 * \internal
 * serial port interrupt routine
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in serial_irq_impl()
 */
void serial_IRQ_Routine()   __attribute__ ((interrupt("IRQ"),naked));
void serial_IRQ_Routine()
{
    saveContextFromIrq();
	//Call serial_irq_impl(). Name is a C++ mangled name.
	asm volatile("bl _ZN6miosix15serial_irq_implEv");
    restoreContext();
}

void IRQserialInit(unsigned int div)
{
    tx_queue.IRQreset();
    rx_queue.IRQreset();
    PCONP|=(1<<3);//Enable UART0 peripheral
    U0LCR=0x3;//DLAB disabled
    //0x07= fifo enabled, reset tx and rx hardware fifos
    //0x80= uart rx fifo trigger level 8 characters
    U0FCR=0x07 | 0x80;
    U0LCR=0x83;//8data bit, 1stop, no parity, DLAB enabled
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
    VICVectAddr2=(unsigned long)&serial_IRQ_Routine;
    serial_enabled=true;
}

void IRQserialDisable()
{
    serial_enabled=false;
    while(!(tx_queue.isEmpty() && IRQserialTxFifoEmpty())) ; //wait
    //Disable VIC
    VICIntEnClr=(1<<6);
    //Disable UART0
    U0LCR=0;//DLAB disabled
    U0FCR=0;
    //Disable PIN
    PINSEL0&=~(0xf);//Clear bits 0 to 3 of PINSEL0
}

bool IRQisSerialEnabled()
{
    return serial_enabled;
}

void serialWrite(const char *str, unsigned int len)
{
    if(!serial_enabled) return;
    int i;
    Lock l(tx_mutex);
    {
        InterruptDisableLock dLock;
        while(len>0)
        {
            //If somebody disables serial port while we are transmitting
            if(!serial_enabled) break;
            //If no data in software and hardware queue
            if((U0LSR & (1<<5))&&(tx_queue.isEmpty()))
            {
                //Fill hardware queue first
                for(i=0;i<HARDWARE_TX_QUEUE_LENGTH;i++)
                {
                    U0THR=*str;
                    str++;
                    len--;
                    if(len==0) break;
                }
            } else {
                if(tx_queue.IRQput(*str)==true)
                {
                    str++;
                    len--;
                } else {
                    InterruptEnableLock eLock(dLock);
                    tx_queue.waitUntilNotFull();
                }
            }
        }
    }
}

bool serialTxComplete()
{
    //If both hardware and software queue are empty, tx is complete.
    return tx_queue.isEmpty() && IRQserialTxFifoEmpty();
}

char serialReadChar()
{
    char result;
    rx_mutex.lock();
    rx_queue.get(result);
    rx_mutex.unlock();
    return result;
}

bool serialReadCharNonblocking(char& c)
{
    bool result=false;
    rx_mutex.lock();
    if(rx_queue.isEmpty()==false)
    {
        rx_queue.get(c);
        result=true;
    }
    rx_mutex.unlock();
    return result;
}

bool serialRxLost()
{
    bool temp=(bool)rx_lost_flag;
    rx_lost_flag=false;
    return temp;
}

void serialRxFlush()
{
    rx_queue.reset();
}

void IRQserialWriteString(const char *str)
{
    if(!serial_enabled) return;
    while((*str)!='\0')
    {
        //Wait until the hardware fifo is ready to accept one char
        while(!(U0LSR & (1<<5))) ; //wait
        U0THR=*str;
        str++;
    }
}

bool IRQserialTxFifoEmpty()
{
    return U0LSR & (1<<6);
}

};//Namespace miosix
