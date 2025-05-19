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

namespace miosix {

/**
 * RP2040 DMA-enabled driver for the PL022 SPI hardware. Master mode only.
 */
class RP2040PL022SPI
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
    RP2040PL022SPI(int number, unsigned int bitrate, bool spo, bool sph,
                   GpioPin si, GpioPin so, GpioPin sck, GpioPin ce);
    /**
     * Destructor.
     */
    ~RP2040PL022SPI();

    /**
     * Changes the current serial clock to the specified one.
     * \param The clock rate to be programmed in Hz. This is an upper bound;
     * the actual clock rate might be lower.
     */
    void setBitrate(unsigned int bitrate);

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
    unsigned short sendRecv(unsigned short data, unsigned wordSize=8);

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
    void sendRecv(const unsigned short send[], unsigned short recv[], size_t len, unsigned wordSize=8);
    void sendRecv(const unsigned char send[], unsigned char recv[], size_t len, unsigned wordSize=8);

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
    void send(const unsigned short send[], size_t len, unsigned wordSize=8);
    void send(const unsigned char send[], size_t len, unsigned wordSize=8);

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
    void recv(unsigned short recv[], size_t len, unsigned wordSize=8, unsigned short sendDummy=0xFFFF);
    void recv(unsigned char recv[], size_t len, unsigned wordSize=8, unsigned short sendDummy=0xFF);
    
private:
    void IRQhandleInterrupt();
    void IRQhandleDmaInterrupt();

    void setWordSize(unsigned int wordSize);
    template<typename D>
    void sendRecvImpl(const D send[], D recv[], size_t len, unsigned wordSize);
    template<typename D>
    void sendImpl(const D send[], size_t len, unsigned wordSize);
    template<typename D>
    void recvImpl(D recv[], size_t len, unsigned wordSize, D sendDummy);
    
    spi_hw_t *spi;
    Thread *waiting=nullptr;
    GpioPin so;
    unsigned int bitrate;
    unsigned char txDmaCh, rxDmaCh;
};

} // namespace miosix
