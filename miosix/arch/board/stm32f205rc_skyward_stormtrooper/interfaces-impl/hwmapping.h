/***************************************************************************
 *   Copyright (C) 2016 by Silvano Seva for Skyward Experimental           *
 *   Rocketry                                                              *
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

#include "interfaces/gpio.h"

namespace miosix {

//external SPI flash memories, whith dedicated SPI bus
namespace memories {
using sck  = Gpio<PA, 5>;
using miso = Gpio<PA, 6>;
using mosi = Gpio<PA, 7>;
using cs0  = Gpio<PC,15>;
using cs1  = Gpio<PC,14>;
using cs2  = Gpio<PC,13>;
}

//MCP2515 SPI driven CAN interface chip
namespace mcp2515 {
using sck    = Gpio<PB, 3>;
using miso   = Gpio<PB, 4>;
using mosi   = Gpio<PB, 5>;
using tx0rts = Gpio<PB, 7>;
using tx1rts = Gpio<PB, 8>;
using tx2rts = Gpio<PB, 9>;
using interr = Gpio<PA, 8>;
}

namespace can {
using rx1 = Gpio<PA,11>;
using tx1 = Gpio<PA,12>;
using rx2 = Gpio<PB,12>;
using tx2 = Gpio<PB,13>;
}

//analog inputs
namespace analogIn {
using ch0 = Gpio<PA, 4>;
using ch1 = Gpio<PB, 1>;
using ch2 = Gpio<PB, 2>;
using ch3 = Gpio<PC, 5>;
using ch4 = Gpio<PC, 4>;
using ch5 = Gpio<PC, 2>;
using ch6 = Gpio<PC, 6>;
using ch7 = Gpio<PC, 1>;
using ch8 = Gpio<PC, 0>;
}

//general purpose IOs (typically used as digital IO)
namespace gpio {
using gpio0 = Gpio<PA,15>;
using gpio1 = Gpio<PC, 8>;
using gpio2 = Gpio<PC, 9>;
using gpio3 = Gpio<PB,15>;
}

//USART2, connected to RS485 transceiver
namespace usart2 {
using tx  = Gpio<PA, 2>;
using rx  = Gpio<PA, 3>;
using rts = Gpio<PA, 1>;
}

//USART3, connected to RS485 transceiver
namespace usart3 {
using tx  = Gpio<PB,10>;
using rx  = Gpio<PB,11>;
using rts = Gpio<PB,14>;
}

//UART4
namespace uart4 {
using tx  = Gpio<PC,10>;
using rx  = Gpio<PC,11>;
}

//UART5
namespace uart5 {
using tx  = Gpio<PC,12>;
using rx  = Gpio<PD, 2>;
}

//UART6
namespace uart6 {
using tx  = Gpio<PC, 6>;
using rx  = Gpio<PC, 7>;
}
} //namespace miosix
