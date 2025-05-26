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

#pragma once

#include "filesystem/console/console_device.h"
#include "kernel/sync.h"
#include "kernel/queue.h"

namespace miosix {

/**
 * Vendor-independent driver for the ARM PL011 serial hardware.
 */
class PL011Serial : public Device
{
public:
    /**
     * Constructor, initializes the serial port.
     * Calls errorHandler(Error::UNEXPECTED) if the port is already being used by
     * another instance of this driver.
     * \param base Base address of the memory mapped register bank of the
     * peripheral.
     * \param irqn The IRQ number for the peripheral.
     * \param clk The clock of the peripheral in Hz. Use to compute bit rate.
     * \param rxQueueLen The length of the receive queue. Should be sufficient
     * for storing 1ms of uninterrupted data. Suggested value is
     * 32+baudrate/500.
     *
     * \note Since this peripheral class is vendor-independent, it cannot
     * take the peripheral out of reset or configure the GPIOs. This must be
     * done in a vendor-dependent way outside of this class. Once this is
     * done, you must call the initialize() method before using the peripheral.
     */
    PL011Serial(void *base, unsigned int irqn, unsigned int clk, unsigned int rxQueueLen) noexcept
        : Device(Device::TTY), uart(reinterpret_cast<Regs*>(base)), irqn(irqn),
          peripheralClock(clk), txLowWaterFlag(1), rxQueue(rxQueueLen) { }
    
    /**
     * Initialize the hardware.
     * \param baudrate serial port baudrate
     * \param hwFlowCtl true if hardware flow control (RTS/CTS) should be enabled
     */
    void initialize(GlobalIrqLock& lock, unsigned int baudrate, bool hwFlowCtl) noexcept;
    
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
    ~PL011Serial();
    
private:
    /// \private Hardware register definition
    /// Not using the arch_registers definitions because they might change from
    /// vendor to vendor.
    struct Regs
    {
        struct Field
        {
            Field(unsigned char lsb, unsigned char msb) : lsb(lsb), msb(msb) {}
            unsigned int mask() { return ((1<<(1+msb-lsb))-1) << lsb; } 
            unsigned int get(unsigned int v) { return (v & mask()) >> lsb; }
            unsigned int put(unsigned int v, unsigned int old=0)
            {
                return (old & ~mask()) | ((v << lsb) & mask());
            }
            const unsigned char lsb, msb;
        };

        volatile unsigned int DR;
        static inline Field DR_DATA() { return Field(0,7); }
        static inline Field DR_FE() { return Field(8,8); }
        static inline Field DR_PE() { return Field(9,9); }
        static inline Field DR_BE() { return Field(10,10); }
        static inline Field DR_OE() { return Field(11,11); }

        union
        {
            volatile unsigned int RSR;
            volatile unsigned int ECR;
        };
        static inline Field RSR_FE() { return Field(0,0); }
        static inline Field RSR_PE() { return Field(1,1); }
        static inline Field RSR_BE() { return Field(2,2); }
        static inline Field RSR_OE() { return Field(3,3); }

        unsigned int _reserved0[4];
        volatile unsigned int FR;
        static inline Field FR_CTS() { return Field(0,0); }
        static inline Field FR_DSR() { return Field(1,1); }
        static inline Field FR_DCD() { return Field(2,2); }
        static inline Field FR_BUSY() { return Field(3,3); }
        static inline Field FR_RXFE() { return Field(4,4); }
        static inline Field FR_TXFF() { return Field(5,5); }
        static inline Field FR_RXFF() { return Field(6,6); }
        static inline Field FR_TXFE() { return Field(7,7); }
        static inline Field FR_RI() { return Field(8,8); }

        unsigned int _reserved1;
        volatile unsigned int ILPR;

        volatile unsigned int IBRD;
        volatile unsigned int FBRD;
        volatile unsigned int LCR_H;
        static inline Field LCR_H_BRK() { return Field(0,0); }
        static inline Field LCR_H_PEN() { return Field(1,1); }
        static inline Field LCR_H_EPS() { return Field(2,2); }
        static inline Field LCR_H_STP2() { return Field(3,3); }
        static inline Field LCR_H_FEN() { return Field(4,4); }
        static inline Field LCR_H_WLEN() { return Field(5,6); }
        static inline Field LCR_H_SPS() { return Field(7,7); }

        volatile unsigned int CR;
        static inline Field CR_UARTEN() { return Field(0,0); }
        static inline Field CR_SIREN() { return Field(1,1); }
        static inline Field CR_SIRLP() { return Field(2,2); }
        static inline Field CR_LBE() { return Field(7,7); }
        static inline Field CR_TXE() { return Field(8,8); }
        static inline Field CR_RXE() { return Field(9,9); }
        static inline Field CR_DTR() { return Field(10,10); }
        static inline Field CR_RTS() { return Field(11,11); }
        static inline Field CR_OUT1() { return Field(12,12); }
        static inline Field CR_OUT2() { return Field(13,13); }
        static inline Field CR_RTSEN() { return Field(14,14); }
        static inline Field CR_CTSEN() { return Field(15,15); }

        volatile unsigned int IFLS;
        static inline Field IFLS_TXIFLSEL() { return Field(0,2); }
        static inline Field IFLS_RXIFLSEL() { return Field(3,5); }

        volatile unsigned int IMSC;
        volatile unsigned int RIS;
        volatile unsigned int MIS;
        volatile unsigned int ICR;
        static inline Field INT_RI() { return Field(0,0); }
        static inline Field INT_CTS() { return Field(1,1); }
        static inline Field INT_DCD() { return Field(2,2); }
        static inline Field INT_DSR() { return Field(3,3); }
        static inline Field INT_RX() { return Field(4,4); }
        static inline Field INT_TX() { return Field(5,5); }
        static inline Field INT_RT() { return Field(6,6); }
        static inline Field INT_FE() { return Field(7,7); }
        static inline Field INT_PE() { return Field(8,8); }
        static inline Field INT_BE() { return Field(9,9); }
        static inline Field INT_OE() { return Field(10,10); }

        volatile unsigned int DMACR;
        static inline Field INT_RXDMAE() { return Field(0,0); }
        static inline Field INT_TXDMAE() { return Field(1,1); }
        static inline Field INT_DMAONERR() { return Field(2,2); }
    };

    /**
     * \internal the serial port interrupts call this member function.
     * Never call this from user code.
     */
    void IRQhandleInterrupt() noexcept;

    // Internal function to disable RX interrupts when the buffer is full
    inline void disableRXInterrupts() noexcept
    {
        uart->IMSC = Regs::INT_TX().mask();
    }

    // Internal function to enable RX and TX interrupts for normal operation
    inline void enableAllInterrupts() noexcept
    {
        uart->IMSC = Regs::INT_RT().mask()
                   | Regs::INT_RX().mask()
                   | Regs::INT_TX().mask();
    }

    Regs *uart;                 ///< Pointer to the hardware registers
    const unsigned int irqn;    ///< Interrupt number
    const unsigned int peripheralClock;
    KernelMutex txMutex;        ///< Mutex locked during transmission
    KernelMutex rxMutex;        ///< Mutex locked during reception
    /// Semaphore flagged when the hardware TX FIFO is ready to receive bytes
    Semaphore txLowWaterFlag;
    /// Software queue used for buffering bytes from the hardware RX FIFO
    DynQueue<unsigned char> rxQueue;
};

} //namespace miosix
