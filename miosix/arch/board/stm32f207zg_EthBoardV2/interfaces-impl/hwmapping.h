/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

typedef Gpio<PE,2> led;

//Spare GPIOs, brought out to a connector
namespace gpio {
typedef Gpio<PE,3> g0;
typedef Gpio<PE,4> g1;
typedef Gpio<PE,5> g2;
typedef Gpio<PE,6> g3;
}

//Gpios that connect the ethernet transceiver to the microcontroller
namespace mii {
typedef Gpio<PC,1>  mdc;
typedef Gpio<PA,2>  mdio;
typedef Gpio<PA,8>  clk;
typedef Gpio<PC,6>  res;
typedef Gpio<PC,0>  irq;
typedef Gpio<PA,3>  col;
typedef Gpio<PA,0>  crs;
typedef Gpio<PA,1>  rxc;
typedef Gpio<PA,7>  rxdv;
typedef Gpio<PB,10> rxer;
typedef Gpio<PC,4>  rxd0;
typedef Gpio<PC,5>  rxd1;
typedef Gpio<PB,0>  rxd2;
typedef Gpio<PB,1>  rxd3;
typedef Gpio<PB,11> txen;
typedef Gpio<PC,3>  txc;
typedef Gpio<PB,12> txd0;
typedef Gpio<PB,13> txd1;
typedef Gpio<PC,2>  txd2;
typedef Gpio<PB,8>  txd3;
}

//External SRAM, connected to FSMC
namespace sram {
typedef Gpio<PD,7>  cs1;
typedef Gpio<PD,4>  oe;
typedef Gpio<PD,5>  we;
typedef Gpio<PE,0>  lb;
typedef Gpio<PE,1>  ub;
typedef Gpio<PD,14> d0;
typedef Gpio<PD,15> d1;
typedef Gpio<PD,0>  d2;
typedef Gpio<PD,1>  d3;
typedef Gpio<PE,7>  d4;
typedef Gpio<PE,8>  d5;
typedef Gpio<PE,9>  d6;
typedef Gpio<PE,10> d7;
typedef Gpio<PE,11> d8;
typedef Gpio<PE,12> d9;
typedef Gpio<PE,13> d10;
typedef Gpio<PE,14> d11;
typedef Gpio<PE,15> d12;
typedef Gpio<PD,8>  d13;
typedef Gpio<PD,9>  d14;
typedef Gpio<PD,10> d15;
typedef Gpio<PF,0>  a0;
typedef Gpio<PF,1>  a1;
typedef Gpio<PF,2>  a2;
typedef Gpio<PF,3>  a3;
typedef Gpio<PF,4>  a4;
typedef Gpio<PF,5>  a5;
typedef Gpio<PF,12> a6;
typedef Gpio<PF,13> a7;
typedef Gpio<PF,14> a8;
typedef Gpio<PF,15> a9;
typedef Gpio<PG,0>  a10;
typedef Gpio<PG,1>  a11;
typedef Gpio<PG,2>  a12;
typedef Gpio<PG,3>  a13;
typedef Gpio<PG,4>  a14;
typedef Gpio<PG,5>  a15;
typedef Gpio<PD,11> a16;
typedef Gpio<PD,12> a17;
}

//Connections to the optional nRF24L01 radio module
namespace nrf {
typedef Gpio<PA,4> cs;
typedef Gpio<PA,5> sck;
typedef Gpio<PA,6> miso;
typedef Gpio<PB,5> mosi;
typedef Gpio<PF,6> ce;
typedef Gpio<PF,7> irq;
}

//Serial port
namespace serial {
typedef Gpio<PA,9>  tx;
typedef Gpio<PA,10> rx;
}

//USB host port
namespace usbhost {
typedef Gpio<PA,11> dm;
typedef Gpio<PA,12> dp;
}

//USB device port
namespace usbdevice {
typedef Gpio<PB,14> dm;
typedef Gpio<PB,15> dp;
}

//MicroSD card slot
namespace sdio {
typedef Gpio<PC,8>  d0;
typedef Gpio<PC,9>  d1;
typedef Gpio<PC,10> d2;
typedef Gpio<PC,11> d3;
typedef Gpio<PC,12> ck;
typedef Gpio<PC,13> cd;
typedef Gpio<PD,2>  cmd;
}

//JTAG port
namespace jtag {
typedef Gpio<PA,15> tdi;
typedef Gpio<PB,3>  tdo;
typedef Gpio<PA,13> tms;
typedef Gpio<PA,14> tck;
typedef Gpio<PB,4>  trst;
}

//Unused pins, configured as pulldown
namespace unused {
typedef Gpio<PB,2>  u1;  //Connected to ground, as it is boot1
typedef Gpio<PB,6>  u2;
typedef Gpio<PB,7>  u3;
typedef Gpio<PB,9>  u4;
typedef Gpio<PC,7>  u5;
typedef Gpio<PC,14> u6;
typedef Gpio<PC,15> u7;
typedef Gpio<PD,3>  u8;
typedef Gpio<PD,6>  u9;
typedef Gpio<PD,13> u10;
typedef Gpio<PF,8>  u11;
typedef Gpio<PF,9>  u12;
typedef Gpio<PF,10> u13;
typedef Gpio<PF,11> u14;
typedef Gpio<PG,6>  u15;
typedef Gpio<PG,7>  u16;
typedef Gpio<PG,8>  u17;
typedef Gpio<PG,9>  u18;
typedef Gpio<PG,10> u19;
typedef Gpio<PG,11> u20;
typedef Gpio<PG,12> u21;
typedef Gpio<PG,13> u22;
typedef Gpio<PG,14> u23;
typedef Gpio<PG,15> u24;
}

} //namespace miosix
