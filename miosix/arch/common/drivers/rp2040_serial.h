/***************************************************************************
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

#pragma once

#include "filesystem/console/console_device.h"
#include "kernel/sync.h"
#include "kernel/queue.h"
#include "interfaces/arch_registers.h"

namespace miosix {

class RP2040PL011Serial0;
class RP2040PL011Serial1;

namespace internal {

extern RP2040PL011Serial0 *uart0Handler;
extern RP2040PL011Serial1 *uart1Handler;

} // namespace internal

class RP2040PL011SerialBase : public Device
{
public:
    /**
     * Constructor, initializes the serial port.
     * Calls errorHandler(UNEXPECTED) if id is not in the correct range, or when
     * attempting to construct multiple objects with the same id. That is,
     * it is possible to instantiate only one instance of this class for each
     * hardware USART.
     * \param baudrate serial port baudrate
     */
    RP2040PL011SerialBase(uart_hw_t *uart) : Device(Device::TTY), 
        uart(uart), txLowWaterFlag(1) {}
    
    /**
     * Read a block of data
     * \param buffer buffer where read data will be stored
     * \param size buffer size
     * \param where where to read from
     * \return number of bytes read or a negative number on failure. Note that
     * it is normal for this function to return less character than the amount
     * asked
     */
    ssize_t readBlock(void *buffer, size_t size, off_t where);
    
    /**
     * Write a block of data
     * \param buffer buffer where take data to write
     * \param size buffer size
     * \param where where to write to
     * \return number of bytes written or a negative number on failure
     */
    ssize_t writeBlock(const void *buffer, size_t size, off_t where);
    
    /**
     * Write a string.
     * An extension to the Device interface that adds a new member function,
     * which is used by the kernel on console devices to write debug information
     * before the kernel is started or in case of serious errors, right before
     * rebooting.
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt. This default implementation ignores writes.
     * \param str the string to write. The string must be NUL terminated.
     */
    void IRQwrite(const char *str);
    
    /**
     * Performs device-specific operations
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    int ioctl(int cmd, void *arg);

    /**
     * \internal the serial port interrupts call this member function.
     * Never call this from user code.
     */
    void IRQhandleInterrupt();
    
    /**
     * Destructor
     */
    ~RP2040PL011SerialBase()
    {
        //Disable UART operation
        uart->cr = 0;
    }
    
protected:
    // Initialize the serial port for a given baud rate. This function is in the
    // header to allow compile-time computation of the baud rate through
    // inlining
    void init(int baudrate)
    {
        //Configure interrupts
        uart->ifls = (2 << UART_UARTIFLS_RXIFLSEL_MSB) | (2 << UART_UARTIFLS_TXIFLSEL_LSB);
        uart->imsc = UART_UARTIMSC_RTIM_BITS | UART_UARTIMSC_RXIM_BITS | UART_UARTIMSC_TXIM_BITS;
        //Setup baud rate
        int rate = 16 * baudrate;
        int div = CLK_SYS_FREQ / rate;
        int frac = (rate * 64) / (CLK_SYS_FREQ % rate);
        uart->ibrd = div;
        uart->fbrd = frac;
        //Line configuration and UART enable
        uart->lcr_h = (3 << UART_UARTLCR_H_WLEN_LSB) | UART_UARTLCR_H_FEN_BITS;
        uart->cr = UART_UARTCR_UARTEN_BITS | UART_UARTCR_TXE_BITS | UART_UARTCR_RXE_BITS;
    }

private:
    uart_hw_t * const uart;
    FastMutex txMutex;                ///< Mutex locked during transmission
    FastMutex rxMutex;                ///< Mutex locked during reception
    Semaphore txLowWaterFlag;
    Queue<uint8_t, 256> rxQueue;
};

class RP2040PL011Serial0 : public RP2040PL011SerialBase
{
public:
    RP2040PL011Serial0(int baudrate) : RP2040PL011SerialBase(uart0_hw)
    {
        assert(internal::uart0Handler == nullptr);
        internal::uart0Handler = this;
        unreset_block_wait(RESETS_RESET_UART0_BITS);
        init(baudrate);
        //UART IRQ saves context: its priority must be the lowest possible
        NVIC_SetPriority(UART0_IRQ_IRQn, 3);
        NVIC_EnableIRQ(UART0_IRQ_IRQn);
    }

    ~RP2040PL011Serial0()
    {
        NVIC_DisableIRQ(UART0_IRQ_IRQn);
        NVIC_ClearPendingIRQ(UART0_IRQ_IRQn);
        internal::uart0Handler = nullptr;
    }
};

class RP2040PL011Serial1 : public RP2040PL011SerialBase
{
public:
    RP2040PL011Serial1(int baudrate) : RP2040PL011SerialBase(uart1_hw)
    {
        assert(internal::uart1Handler == nullptr);
        internal::uart1Handler = this;
        unreset_block_wait(RESETS_RESET_UART1_BITS);
        init(baudrate);
        //UART IRQ saves context: its priority must be the lowest possible
        NVIC_SetPriority(UART1_IRQ_IRQn, 3);
        NVIC_EnableIRQ(UART1_IRQ_IRQn);
    }

    ~RP2040PL011Serial1()
    {
        NVIC_DisableIRQ(UART1_IRQ_IRQn);
        NVIC_ClearPendingIRQ(UART1_IRQ_IRQn);
        internal::uart1Handler = nullptr;
    }
};

} //namespace miosix
