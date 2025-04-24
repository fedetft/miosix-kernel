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

class RP2040PL011Serial : public Device
{
public:
    /**
     * Constructor, initializes the serial port.
     * Calls errorHandler(Error::UNEXPECTED) if the port is already being used by
     * another instance of this driver.
     * \param number usart number
     * \param baudrate serial port baudrate
     * \param tx GPIO to configure as usart tx, see datasheet for restrictions
     * \param rx GPIO to configure as usart rx, see datasheet for restrictions
     */
    RP2040PL011Serial(int number, int baudrate, GpioPin tx, GpioPin rx);

    /**
     * Constructor, initializes the serial port.
     * Calls errorHandler(Error::UNEXPECTED) if the port is already being used by
     * another instance of this driver.
     * \param number usart number
     * \param baudrate serial port baudrate
     * \param tx GPIO to configure as usart tx, see datasheet for restrictions
     * \param rx GPIO to configure as usart rx, see datasheet for restrictions
     * \param rts GPIO to configure as usart rts, see datasheet for restrictions
     * \param cts GPIO to configure as usart cts, see datasheet for restrictions
     */
    RP2040PL011Serial(int number, int baudrate, GpioPin tx, GpioPin rx,
                      GpioPin rts, GpioPin cts);
    
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
    ~RP2040PL011Serial();
    
private:

    void commonInit(int number, int baudrate);

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
        uart->imsc = UART_UARTIMSC_RTIM_BITS
                   | UART_UARTIMSC_RXIM_BITS
                   | UART_UARTIMSC_TXIM_BITS;
    }

    uart_hw_t *uart;                  ///< Pointer to the hardware registers
    IRQn_Type irqn;                   ///< Interrupt number
    KernelMutex txMutex;              ///< Mutex locked during transmission
    KernelMutex rxMutex;              ///< Mutex locked during reception
    /// Semaphore flagged when the hardware TX FIFO is ready to receive bytes
    Semaphore txLowWaterFlag;
    /// Software queue used for buffering bytes from the hardware RX FIFO
    DynQueue<unsigned char> rxQueue;
};

} //namespace miosix
