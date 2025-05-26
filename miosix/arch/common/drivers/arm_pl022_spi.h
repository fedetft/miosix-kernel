/***************************************************************************
 *   Copyright (C) 2025 by Daniele Cattaneo                                *
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

#include "kernel/thread.h"
#include "kernel/lock.h"

namespace miosix {

/**
 * Vendor-independent driver for the ARM PL022 SPI hardware. Master mode only.
 */
class PL022Spi
{
public:
    /**
     * Constructor, initializes the SPI port.
     * Calls errorHandler(Error::UNEXPECTED) if the port is already being used
     * by another instance of this driver or another driver.
     * \param base Base address of the memory mapped register bank of the
     * peripheral.
     * \param irqn The IRQ number for the peripheral, or a negative number if
     * there is no IRQ available.
     * \param clk The clock of the peripheral in Hz. Use to compute bit rate.
     * 
     * \note Since this peripheral class is vendor-independent, it cannot
     * take the peripheral out of reset or configure the GPIOs. This must be
     * done in a vendor-dependent way outside of this class. Once this is
     * done, you must call the initialize() method before using the peripheral.
     */
    PL022Spi(void *base, int irqn, unsigned int clk) noexcept
        : spi(reinterpret_cast<Regs*>(base)), irqn(irqn), peripheralClock(clk) { }

    /**
     * Destructor.
     */
    ~PL022Spi() noexcept;

    /**
     * Initialize the hardware.
     * \param bitrate Initially configured serial clock frequency upper bound in
     * Hz. The actual clock rate programmed might be lower.
     * \param spo Clock polarity. `false' keeps the SCK pin low when the line is
     * idle, while `true' keeps SCK high instead.
     * \param sph Clock phase. `false' if data in/out must be sampled at the
     * rising edge of the clock, `true' if it must be sampled at the falling
     * edge.
     */
    void initialize(unsigned int bitrate, bool spo, bool sph) noexcept;

    /**
     * Changes the current serial clock to the specified one.
     * \param The clock rate to be programmed in Hz. This is an upper bound;
     * the actual clock rate might be lower.
     */
    void setBitrate(unsigned int bitrate) noexcept;

    /**
     * Send and receive a single word.
     * \param data The word to be transmitted
     * \param wordSize The word size; only the least significant wordSize bits
     * of data will be transmitted, and only wordSize bits are going to be
     * received.
     * This parameter specifies the number of clocks that correspond to each
     * word and must be correct according to what the receiving peripheral
     * expects.
     * By default it is 8 bits.
     * \returns The data coming back from the SI pin.
     */
    unsigned short sendRecv(unsigned short data, unsigned wordSize=8) noexcept;

    /**
     * Send and receive a string of words.
     * \param send The string of words to be transmitted.
     * \param recv Buffer for the words received.
     * \param len The number of words to be sent and simultaneously received.
     * \param wordSize The word size; only the least significant wordSize bits
     * of each data word will be transmitted, and only wordSize bits are going
     * to be received for each data word.
     * This parameter specifies the number of clocks that correspond to each
     * word and must be correct according to what the receiving peripheral
     * expects.
     * By default it is 8 bits.
     */
    void sendRecv(const unsigned short send[], unsigned short recv[], size_t len, unsigned wordSize=8) noexcept;
    void sendRecv(const unsigned char send[], unsigned char recv[], size_t len, unsigned wordSize=8) noexcept;

    /**
     * Send a string of words, ignoring the response being trasmitted back.
     * \param send The string of words to be transmitted.
     * \param len The number of words to be sent.
     * \param wordSize The word size; only the least significant wordSize bits
     * of each data word will be transmitted.
     * This parameter specifies the number of clocks that correspond to each
     * word and must be correct according to what the receiving peripheral
     * expects.
     * By default it is 8 bits.
     */
    void send(const unsigned short send[], size_t len, unsigned wordSize=8) noexcept;
    void send(const unsigned char send[], size_t len, unsigned wordSize=8) noexcept;

    /**
     * Receive a string of words, while repeatedly sending the same data.
     * \param recv Buffer for the words received.
     * \param len The number of words to be received.
     * \param wordSize The word size; only the least significant wordSize bits
     * of each data word will be transmitted.
     * This parameter specifies the number of clocks that correspond to each
     * word and must be correct according to what the receiving peripheral
     * expects.
     * By default it is 8 bits.
     * \param sendDummy Word to be sent to keep the transmission going and
     * the clock clocking.
     */
    void recv(unsigned short recv[], size_t len, unsigned wordSize=8, unsigned short sendDummy=0xFFFF) noexcept;
    void recv(unsigned char recv[], size_t len, unsigned wordSize=8, unsigned short sendDummy=0xFF) noexcept;
    
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

        volatile unsigned int CR0;
        static inline Field CR0_DSS() { return Field(0,3); }
        static inline Field CR0_FRF() { return Field(4,5); }
        static inline Field CR0_SPO() { return Field(6,6); }
        static inline Field CR0_SPH() { return Field(7,7); }
        static inline Field CR0_SCR() { return Field(8,15); }

        volatile unsigned int CR1;
        static inline Field CR1_LBM() { return Field(0,0); }
        static inline Field CR1_SSE() { return Field(1,1); }
        static inline Field CR1_MS()  { return Field(2,2); }
        static inline Field CR1_SOD() { return Field(3,3); }

        volatile unsigned int DR;

        volatile unsigned int SR;
        /// Transmit FIFO empty
        static inline Field SR_TFE() { return Field(0,0); }
        /// Transmit FIFO not full
        static inline Field SR_TNF() { return Field(1,1); }
        /// Receive FIFO not empty
        static inline Field SR_RNE() { return Field(2,2); }
        /// Receive FIFO full
        static inline Field SR_RFF() { return Field(3,3); }
        /// Peripheral busy transmitting/receiving
        static inline Field SR_BSY() { return Field(4,4); }

        volatile unsigned int CPSR;

        volatile unsigned int IMSC;
        volatile unsigned int RIS;
        volatile unsigned int MIS;
        volatile unsigned int ICR;
        /// Receive FIFO overflow
        static inline Field INT_RO() { return Field(0,0); }
        /// Receive timeout (line idle)
        static inline Field INT_RT() { return Field(1,1); }
        /// There are four or more valid entries in the receive FIFO
        static inline Field INT_RX() { return Field(2,2); }
        /// There are four or fewer valid entries in the transmit FIFO
        static inline Field INT_TX() { return Field(3,3); }

        volatile unsigned int DMACR;
        static inline Field DMACR_RXDMAE() { return Field(0,0); }
        static inline Field DMACR_TXDMAE() { return Field(1,1); }
    };

    void IRQhandleInterrupt() noexcept;
    void waitForInterrupt(unsigned int flag) noexcept;
    void waitForEndTransfer() noexcept;

    void setWordSize(unsigned int wordSize) noexcept;
    template<typename D>
    void sendRecvImpl(const D send[], D recv[], size_t len, unsigned wordSize) noexcept;
    template<typename D>
    void sendImpl(const D send[], size_t len, unsigned wordSize) noexcept;
    template<typename D>
    void recvImpl(D recv[], size_t len, unsigned wordSize, D sendDummy) noexcept;
    
    Regs *spi;
    const int irqn;
    const unsigned int peripheralClock;
    Thread *waiting=nullptr;
    unsigned int bitrate;
};

} // namespace miosix
