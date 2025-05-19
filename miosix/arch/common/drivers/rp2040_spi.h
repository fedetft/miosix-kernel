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

class RP2040PL022SPI
{
public:
    RP2040PL022SPI(int number, unsigned int bitrate, bool spo, bool sph,
                   GpioPin si, GpioPin so, GpioPin sck, GpioPin ce);
    ~RP2040PL022SPI();

    void setBitrate(unsigned int bitrate);

    unsigned short sendRecv(unsigned short data, unsigned wordSize=8);

    void sendRecv(const unsigned short send[], unsigned short recv[], size_t len, unsigned wordSize=8);
    void sendRecv(const unsigned char send[], unsigned char recv[], size_t len, unsigned wordSize=8);

    void send(const unsigned short send[], size_t len, unsigned wordSize=8);
    void send(const unsigned char send[], size_t len, unsigned wordSize=8);

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
