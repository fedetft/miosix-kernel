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

#include "interfaces/arch_registers.h"
#include "interfaces/gpio.h"
#include "kernel/thread.h"
#include "kernel/lock.h"
#include "arm_pl022_spi.h"

namespace miosix {

/**
 * RP2040 no DMA driver for the PL022 SPI hardware. Master mode only.
 */
class RP2040PL022Spi: public PL022Spi
{
public:
    RP2040PL022Spi(int number, unsigned int bitrate, bool spo, bool sph,
        GpioPin si, GpioPin so, GpioPin sck, GpioPin ce) noexcept
            : PL022Spi(getBase(number),getIrqn(number),peripheralFrequency)
    {
        GlobalIrqLock lock;
        switch(number)
        {
            case 0:
                unreset_block_wait(RESETS_RESET_SPI0_BITS);
                break;
            case 1:
                unreset_block_wait(RESETS_RESET_SPI1_BITS);
                break;
            default:
                errorHandler(Error::UNEXPECTED);
        }
        initialize(bitrate,spo,sph);
        si.function(Function::SPI); si.mode(Mode::INPUT); si.fast();
        so.function(Function::SPI); so.mode(Mode::OUTPUT); so.fast();
        sck.function(Function::SPI); sck.mode(Mode::OUTPUT); sck.fast();
        if (ce.isValid())
        {
            ce.function(Function::SPI); ce.mode(Mode::OUTPUT); ce.fast();
        }
    }

private:
    static void *getBase(int n) { return reinterpret_cast<void*>(n==0 ? spi0_hw : spi1_hw); }
    static unsigned int getIrqn(int n) { return n==0 ? SPI0_IRQ_IRQn : SPI1_IRQ_IRQn; }
};

/**
 * RP2040 DMA-enabled driver for the PL022 SPI hardware. Master mode only.
 */
class RP2040PL022DmaSpi
{
public:
    /**
     * Constructor.
     * \param number The ID of the peripheral (0 or 1)
     * \param bitrate Initially configured serial clock frequency upper bound in
     * Hz. The actual clock rate programmed might be lower.
     * \param spo Clock polarity. `false' keeps the SCK pin low when the line is
     * idle, while `true' keeps SCK high instead.
     * \param sph Clock phase. `false' if data in/out must be sampled at the
     * rising edge of the clock, `true' if it must be sampled at the falling
     * edge.
     * \param si Serial in pin.
     * \param so Serial out pin.
     * \param sck Serial clock pin.
     * \param ce Chip enable pin (optional).
     */
    RP2040PL022DmaSpi(int number, unsigned int bitrate, bool spo, bool sph,
                   GpioPin si, GpioPin so, GpioPin sck, GpioPin ce) noexcept;

    /**
     * Destructor.
     */
    ~RP2040PL022DmaSpi() noexcept;

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
    void IRQhandleInterrupt() noexcept;
    void IRQhandleDmaInterrupt() noexcept;

    void setWordSize(unsigned int wordSize) noexcept;
    template<typename D>
    void sendRecvImpl(const D send[], D recv[], size_t len, unsigned wordSize) noexcept;
    template<typename D>
    void sendImpl(const D send[], size_t len, unsigned wordSize) noexcept;
    template<typename D>
    void recvImpl(D recv[], size_t len, unsigned wordSize, D sendDummy) noexcept;
    
    spi_hw_t *spi;
    Thread *waiting=nullptr;
    unsigned int bitrate;
    unsigned char txDmaCh, rxDmaCh;
};

} // namespace miosix
