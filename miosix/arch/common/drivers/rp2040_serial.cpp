/***************************************************************************
 *   Copyright (C) 2024,2025 by Daniele Cattaneo                           *
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

#include <termios.h>
#include <errno.h>
#include "config/miosix_settings.h"
#include "kernel/lock.h"
#include "filesystem/ioctl.h"
#include "rp2040_serial.h"
#include "rp2040_dma.h"

namespace miosix {

RP2040PL011DmaSerial::RP2040PL011DmaSerial(int number, int baudrate,
    GpioPin tx, GpioPin rx, GpioPin rts, GpioPin cts) noexcept
    : Device(Device::TTY), rxQueue(32+baudrate/500)
{
    GlobalIrqLock lock;
    tx.function(Function::UART); tx.mode(Mode::OUTPUT);
    rx.function(Function::UART); rx.mode(Mode::INPUT);
    if(rts.isValid()) { rts.function(Function::UART); rts.mode(Mode::OUTPUT); }
    if(cts.isValid()) { cts.function(Function::UART); cts.mode(Mode::INPUT); }
    unsigned int irqn;
    switch(number)
    {
        case 0:
            unreset_block_wait(RESETS_RESET_UART0_BITS);
            uart=uart0_hw;
            irqn=UART0_IRQ_IRQn;
            break;
        case 1:
            unreset_block_wait(RESETS_RESET_UART1_BITS);
            uart=uart1_hw;
            irqn=UART1_IRQ_IRQn;
            break;
        default:
            errorHandler(Error::UNEXPECTED);
    }
    // Configure interrupts
    IRQregisterIrq(lock,irqn,&RP2040PL011DmaSerial::IRQhandleInterrupt,this);
    uart->ifls = (2<<UART_UARTIFLS_RXIFLSEL_LSB) | (2<<UART_UARTIFLS_TXIFLSEL_LSB);
    enableRXInterrupts();
    // Reserve DMA channels
    txDmaCh = RP2040Dma::IRQregisterChannel(lock,&RP2040PL011DmaSerial::IRQhandleDmaInterrupt,this);
    // Setup baud rate
    int rate = 16 * baudrate;
    int div = peripheralFrequency / rate;
    int frac = ((rate * 128) / (peripheralFrequency % rate) + 1) / 2;
    uart->ibrd = div;
    uart->fbrd = frac;
    // Line configuration and UART enable
    uart->lcr_h = (3 << UART_UARTLCR_H_WLEN_LSB) | UART_UARTLCR_H_FEN_BITS;
    uart->cr=
          UART_UARTCR_UARTEN_BITS
        | UART_UARTCR_TXE_BITS
        | UART_UARTCR_RXE_BITS
        | (rts.isValid()?UART_UARTCR_RTSEN_BITS:0)
        | (cts.isValid()?UART_UARTCR_CTSEN_BITS:0);
    // DMA enable
    uart->dmacr = UART_UARTDMACR_TXDMAE_BITS;
}

ssize_t RP2040PL011DmaSerial::readBlock(void *buffer, size_t size, off_t where)
{
    if(size==0) return 0;
    Lock<KernelMutex> lock(rxMutex);
    auto bytes = reinterpret_cast<unsigned char *>(buffer);
    size_t i = 0;
    // Block until we can read the first byte
    rxQueue.get(bytes[i++]);
    // Get bytes as long as there are bytes in the software queue or the
    // hardware FIFO.
    // As the interrupt handler never empties the FIFO unless the line is idle,
    // this also tells us if the line is idle and we should stop.
    while(i<size && (!(uart->fr & UART_UARTFR_RXFE_BITS) || !rxQueue.isEmpty()))
    {
        rxQueue.get(bytes[i++]);
        // Ensure the read interrupts can be serviced to read the next byte.
        // The interrupt routine disables them on sw queue full.
        if(rxQueue.free()>=32) enableRXInterrupts();
    }
    return i;
}

ssize_t RP2040PL011DmaSerial::writeBlock(const void *buffer, size_t size, off_t where)
{
    if(size==0) return 0;
    Lock<KernelMutex> lock(txMutex);
    dma_hw->ch[txDmaCh].read_addr=reinterpret_cast<unsigned int>(buffer);
    dma_hw->ch[txDmaCh].write_addr=reinterpret_cast<unsigned int>(&uart->dr);
    dma_hw->ch[txDmaCh].transfer_count=size;
    unsigned int dreq=(uart==uart0_hw)?20:22;
    {
        FastGlobalIrqLock lock;
        dma_hw->ch[txDmaCh].ctrl_trig=
              (dreq<<DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB)
            | (txDmaCh<<DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) // disables chaining
            | DMA_CH0_CTRL_TRIG_INCR_READ_BITS
            | (0<<DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB)
            | DMA_CH0_CTRL_TRIG_EN_BITS;
        // Wait for the DMA channel to finish
        while(true)
        {
            if(!((dma_hw->ch[txDmaCh].al1_ctrl)&DMA_CH0_CTRL_TRIG_BUSY_BITS)) break;
            txWaiting=Thread::IRQgetCurrentThread();
            while(txWaiting) Thread::IRQglobalIrqUnlockAndWait(lock);
        }
        dma_hw->ch[txDmaCh].al1_ctrl=0;
    }
    return size;
}

void RP2040PL011DmaSerial::IRQwrite(const char *str)
{
    // Wait for any pending DMA transfer to complete
    while(!(uart->fr & UART_UARTFR_TXFE_BITS)) ;
    // Write to the data register directly
    for(int i=0; str[i] != '\0'; i++)
    {
        while(uart->fr & UART_UARTFR_TXFF_BITS) ;
        uart->dr = str[i];
    }
    // Flush
    while(!(uart->fr & UART_UARTFR_TXFE_BITS)) ;
}

int RP2040PL011DmaSerial::ioctl(int cmd, void *arg)
{
    if(reinterpret_cast<unsigned>(arg) & 0b11) return -EFAULT; //Unaligned
    termios *t=reinterpret_cast<termios*>(arg);
    switch(cmd)
    {
        case IOCTL_SYNC:
            while(!(uart->fr & UART_UARTFR_TXFE_BITS)) ;
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

RP2040PL011DmaSerial::~RP2040PL011DmaSerial()
{
    GlobalIrqLock lock;
    //Disable UART operation
    uart->cr = 0;
    if(uart==uart0_hw)
    {
        IRQunregisterIrq(lock,UART0_IRQ_IRQn,&RP2040PL011DmaSerial::IRQhandleInterrupt,this);
        reset_block(RESETS_RESET_UART0_BITS);
    } else {
        IRQunregisterIrq(lock,UART1_IRQ_IRQn,&RP2040PL011DmaSerial::IRQhandleInterrupt,this);
        reset_block(RESETS_RESET_UART1_BITS);
    }
    RP2040Dma::IRQunregisterChannel(lock,txDmaCh,&RP2040PL011DmaSerial::IRQhandleDmaInterrupt,this);
}

void RP2040PL011DmaSerial::IRQhandleInterrupt() noexcept
{
    FastGlobalLockFromIrq dLock;
    uint32_t flags = uart->mis;
    if(flags & (UART_UARTMIS_RXMIS_BITS|UART_UARTMIS_RTMIS_BITS))
    {
        // Read enough data to clear the interrupt status,
        // or until the software-side queue is full
        while((uart->mis & (UART_UARTMIS_RXMIS_BITS | UART_UARTMIS_RTMIS_BITS))
                && !rxQueue.isFull())
            rxQueue.IRQput(static_cast<unsigned char>(uart->dr));
        // If the sw queue is full, mask RX interrupts temporarily. The
        // device read handler will un-mask them when the queue has some
        // space again. If there was more data to read and hence the interrupt
        // flag was not cleared, un-masking the interrupts causes the immediate
        // reentry in this interrupt handler, which allows to finish the work
        // without losing the line idle status information (which only exists
        // in the interrupt flags).
        if(rxQueue.isFull()) disableRXInterrupts();
    }
}

void RP2040PL011DmaSerial::IRQhandleDmaInterrupt() noexcept
{
    // We do not take the GIL because the DMA code already did
    if(txWaiting)
    {
        txWaiting->IRQwakeup();
        txWaiting=nullptr;
    }
}

} // namespace miosix
