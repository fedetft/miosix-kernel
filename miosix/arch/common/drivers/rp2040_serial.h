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

void uart0IrqImpl();
void uart1IrqImpl();

} // namespace internal

class RP2040PL011SerialBase : public Device
{
public:
    /**
     * \internal
     * Internal PL011 driver base class constructor
     * \param uart Pointer to the hardware registers
     * \param queueSize Size of the RX queue
     */
    RP2040PL011SerialBase(uart_hw_t *uart, unsigned int queueSize) :
        Device(Device::TTY), uart(uart), txLowWaterFlag(1), rxQueue(queueSize)
        {}
    
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
    void init(int baudrate, bool rts, bool cts)
    {
        //Configure interrupts
        uart->ifls = (2<<UART_UARTIFLS_RXIFLSEL_LSB) | (2<<UART_UARTIFLS_TXIFLSEL_LSB);
        enableAllInterrupts();
        //Setup baud rate
        int rate = 16 * baudrate;
        int div = CLK_SYS_FREQ / rate;
        int frac = ((rate * 128) / (CLK_SYS_FREQ % rate) + 1) / 2;
        uart->ibrd = div;
        uart->fbrd = frac;
        //Line configuration and UART enable
        uart->lcr_h = (3 << UART_UARTLCR_H_WLEN_LSB) | UART_UARTLCR_H_FEN_BITS;
        uart->cr = UART_UARTCR_UARTEN_BITS | UART_UARTCR_TXE_BITS |
                UART_UARTCR_RXE_BITS | (rts ? UART_UARTCR_RTSEN_BITS : 0) |
                (cts ? UART_UARTCR_CTSEN_BITS : 0);
    }

private:
    friend void internal::uart0IrqImpl();
    friend void internal::uart1IrqImpl();

    /**
     * \internal the serial port interrupts call this member function.
     * Never call this from user code.
     */
    void IRQhandleInterrupt();

    // Internal function to disable RX interrupts when the buffer is full
    inline void disableRXInterrupts()
    {
        uart->imsc = UART_UARTIMSC_TXIM_BITS;
    }

    // Internal function to enable RX and TX interrupts for normal operation
    inline void enableAllInterrupts()
    {
        uart->imsc = UART_UARTIMSC_RTIM_BITS | UART_UARTIMSC_RXIM_BITS | 
                UART_UARTIMSC_TXIM_BITS;
    }

    uart_hw_t * const uart;           ///< Pointer to the hardware registers
    FastMutex txMutex;                ///< Mutex locked during transmission
    FastMutex rxMutex;                ///< Mutex locked during reception
    /// Semaphore flagged when the hardware TX FIFO is ready to receive bytes
    Semaphore txLowWaterFlag;
    /// Software queue used for buffering bytes from the hardware RX FIFO
    DynQueue<uint8_t> rxQueue;
};

class RP2040PL011Serial0 : public RP2040PL011SerialBase
{
public:
    /**
     * Constructor, initializes the serial port.
     * Calls errorHandler(UNEXPECTED) if the port is already being used by
     * another instance of this driver.
     * \param baudrate serial port baudrate
     * \param rts true to enable the RTS flow control signal
     * \param rts true to enable the CTS flow control signal
     */
    RP2040PL011Serial0(int baudrate, bool rts=false, bool cts=false) 
        : RP2040PL011SerialBase(uart0_hw, 32+baudrate/500)
    {
        if(internal::uart0Handler != nullptr) errorHandler(UNEXPECTED);
        internal::uart0Handler = this;
        unreset_block_wait(RESETS_RESET_UART0_BITS);
        init(baudrate, rts, cts);
        // UART IRQ saves context: its priority must be 3 (see portability.cpp)
        NVIC_SetPriority(UART0_IRQ_IRQn, 3);
        NVIC_EnableIRQ(UART0_IRQ_IRQn);
    }

    /**
     * Destructor
     */
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
    /**
     * Constructor, initializes the serial port.
     * Calls errorHandler(UNEXPECTED) if the port is already being used by
     * another instance of this driver.
     * \param baudrate serial port baudrate
     * \param rts true to enable the RTS flow control signal
     * \param rts true to enable the CTS flow control signal
     */
    RP2040PL011Serial1(int baudrate, bool rts=false, bool cts=false)
        : RP2040PL011SerialBase(uart1_hw, 32+baudrate/500)
    {
        if(internal::uart1Handler != nullptr) errorHandler(UNEXPECTED);
        internal::uart1Handler = this;
        unreset_block_wait(RESETS_RESET_UART1_BITS);
        init(baudrate, rts, cts);
        // UART IRQ saves context: its priority must be 3 (see portability.cpp)
        NVIC_SetPriority(UART1_IRQ_IRQn, 3);
        NVIC_EnableIRQ(UART1_IRQ_IRQn);
    }

    /**
     * Destructor
     */
    ~RP2040PL011Serial1()
    {
        NVIC_DisableIRQ(UART1_IRQ_IRQn);
        NVIC_ClearPendingIRQ(UART1_IRQ_IRQn);
        internal::uart1Handler = nullptr;
    }
};

} //namespace miosix
