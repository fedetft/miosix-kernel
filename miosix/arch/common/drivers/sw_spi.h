/***************************************************************************
 *   Copyright (C) 2011, 2012 by Terraneo Federico                         *
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

#include "interfaces/gpio.h"

namespace miosix {

/**
 * Bit-banging SPI driver compatible with hardware-based SPI drivers. Mode 0
 * only (CPOL=0, CPHA=0).
 */
template<typename SI, typename SO, typename SCK, typename CE=NullGpio>
class SwSPI
{
public:
    /**
     * Constructor.
     * \param bitrate Initially configured serial clock frequency upper bound in
     * Hz. The actual clock rate programmed might be lower.
     */
    SwSPI(unsigned int bitrate) noexcept
    {
        SI::mode(Mode::INPUT);
        SO::mode(Mode::OUTPUT);
        SCK::mode(Mode::OUTPUT);
        CE::mode(Mode::OUTPUT);
        CE::high();
        setBitrate(bitrate);
    }

    /**
     * Changes the current serial clock to the specified one.
     * \param The clock rate to be programmed in Hz. This is an upper bound;
     * the actual clock rate might be lower.
     */
    void setBitrate(unsigned int bitrate) noexcept
    {
        // Topping off at about 1MHz
        // You can't get much faster with bitbanging anyway
        clkPeriodUs=((1000000U/bitrate)+1)/2;
    }

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
    unsigned short sendRecv(unsigned short data, unsigned wordSize=8) const noexcept
    {
        CE::low();
        unsigned short res=sendRecvImpl(data,wordSize);
        CE::high();
        return res;
    }

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
    void sendRecv(const unsigned short send[], unsigned short recv[], size_t len, unsigned wordSize=8) const noexcept
    {
        CE::low();
        for(size_t i=0; i<len; i++) recv[i]=sendRecvImpl(send[i],wordSize);
        CE::high();
    }
    void sendRecv(const unsigned char send[], unsigned char recv[], size_t len, unsigned wordSize=8) const noexcept
    {
        CE::low();
        for(size_t i=0; i<len; i++) recv[i]=sendRecvImpl(send[i],wordSize);
        CE::high();
    }

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
    void send(const unsigned short send[], size_t len, unsigned wordSize=8) const noexcept
    {
        CE::low();
        for(size_t i=0; i<len; i++) sendRecvImpl(send[i],wordSize);
        CE::high();
    }
    void send(const unsigned char send[], size_t len, unsigned wordSize=8) const noexcept
    {
        CE::low();
        for(size_t i=0; i<len; i++) sendRecvImpl(send[i],wordSize);
        CE::high();
    }

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
    void recv(unsigned short recv[], size_t len, unsigned wordSize=8, unsigned short sendDummy=0xffff) const noexcept
    {
        CE::low();
        for(size_t i=0; i<len; i++) recv[i]=sendRecvImpl(sendDummy,wordSize);
        CE::high();
    }
    void recv(unsigned char recv[], size_t len, unsigned wordSize=8, unsigned short sendDummy=0xff) const noexcept
    {
        CE::low();
        for(size_t i=0; i<len; i++) recv[i]=sendRecvImpl(sendDummy,wordSize);
        CE::high();
    }

private:
    inline void delay() const
    {
        // Yes we are doing the check of which API to use at every bit.
        // But this is supposed to waste time anyway so whatever.
        if(clkPeriodUs>0)
        {
            if(clkPeriodUs<50) delayUs(clkPeriodUs);
            else Thread::nanoSleep((long long)clkPeriodUs*1000LL);
        } else {
            for(int i=0;i<2;i++) asm volatile("nop");
        }
    }

    unsigned short sendRecvImpl(unsigned short data, unsigned wordSize) const noexcept
    {
        unsigned short result=0;
        unsigned int mask=1U<<(wordSize-1);
        for(unsigned int i=0;i<wordSize;i++)
        {
            if(data & mask) SO::high(); else SO::low();
            SCK::high();
            delay();
            if(SI::value()) result |= mask;
            SCK::low();
            delay();
            mask>>=1;
        }
        return result;
    }

    unsigned int clkPeriodUs;
};

} // namespace miosix
