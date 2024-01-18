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

class RP2040UART0
{
public:
    static inline uart_hw_t *get() { return uart0_hw; }
    static inline void enable()
    {
        unreset_block_wait(RESETS_RESET_UART0_BITS);
    }
};

template <typename T>
class RP2040PL011Serial : public Device
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
    RP2040PL011Serial(int baudrate) : Device(Device::TTY)
    {
        T::enable();
        int rate = 16 * baudrate;
        int div = CLK_SYS_FREQ / rate;
        int frac = (rate * 64) / (CLK_SYS_FREQ % rate);
        T::get()->ibrd = div;
        T::get()->fbrd = frac;
        T::get()->lcr_h = (3 << UART_UARTLCR_H_WLEN_LSB) | UART_UARTLCR_H_FEN_BITS;
        T::get()->cr = UART_UARTCR_UARTEN_BITS | UART_UARTCR_TXE_BITS | UART_UARTCR_RXE_BITS;
    }
    
    /**
     * Read a block of data
     * \param buffer buffer where read data will be stored
     * \param size buffer size
     * \param where where to read from
     * \return number of bytes read or a negative number on failure. Note that
     * it is normal for this function to return less character than the amount
     * asked
     */
    ssize_t readBlock(void *buffer, size_t size, off_t where)
    {
        if (size == 0) return 0;
        Lock<FastMutex> lock(rxMutex);
        uint8_t *bytes = reinterpret_cast<uint8_t *>(buffer);
        while (T::get()->fr & UART_UARTFR_RXFE_BITS) {}
        size_t i = 0;
        bytes[i++] = (uint8_t)T::get()->dr;
        while (i<size) {
            //Wait a bit for next character, but just a bit
            for (int j=0; j<20; j++) {
                if (!(T::get()->fr & UART_UARTFR_RXFE_BITS))
                    break;
            }
            if (T::get()->fr & UART_UARTFR_RXFE_BITS)
                break;
            bytes[i++] = (uint8_t)T::get()->dr;
        }
        return i;
    }
    
    /**
     * Write a block of data
     * \param buffer buffer where take data to write
     * \param size buffer size
     * \param where where to write to
     * \return number of bytes written or a negative number on failure
     */
    ssize_t writeBlock(const void *buffer, size_t size, off_t where)
    {
        Lock<FastMutex> lock(txMutex);
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(buffer);
        for (size_t i=0; i<size; i++)
        {
            while (T::get()->fr & UART_UARTFR_TXFF_BITS) {}
            T::get()->dr = bytes[i];
        }
        return size;
    }
    
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
    void IRQwrite(const char *str)
    {
        for (int i=0; str[i] != '\0'; i++)
        {
            while (T::get()->fr & UART_UARTFR_TXFF_BITS) {}
            T::get()->dr = str[i];
        }
    }
    
    /**
     * Performs device-specific operations
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    int ioctl(int cmd, void *arg) { return 0; };
    
    /**
     * Destructor
     */
    ~RP2040PL011Serial() {};
    
private:
    FastMutex txMutex;                ///< Mutex locked during transmission
    FastMutex rxMutex;                ///< Mutex locked during reception
};

} //namespace miosix
