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
typedef Gpio<PD,13> a18;
}

//640x480 camera
namespace camera {
typedef Gpio<PC,6>  cd0;
typedef Gpio<PC,7>  cd1;
typedef Gpio<PC,8>  cd2;
typedef Gpio<PC,9>  cd3;
typedef Gpio<PE,4>  cd4;
typedef Gpio<PB,6>  cd5;
typedef Gpio<PE,5>  cd6;
typedef Gpio<PE,6>  cd7;
typedef Gpio<PB,7>  vsync;
typedef Gpio<PA,4>  hsync;
typedef Gpio<PA,6>  dclk;
typedef Gpio<PA,8>  exclk;
typedef Gpio<PC,0>  reset;
typedef Gpio<PC,1>  sda;
typedef Gpio<PC,2>  scl;
}

//2MBytes SPI flash
namespace flash {
typedef Gpio<PB,3>  sck;
typedef Gpio<PB,4>  miso;
typedef Gpio<PB,5>  mosi;
typedef Gpio<PB,8>  cs;
}

//Board to board communication, SPI based
namespace comm {
typedef Gpio<PC,10> sck;
typedef Gpio<PC,11> miso;
typedef Gpio<PC,12> mosi;
typedef Gpio<PA,15> cs;
typedef Gpio<PD,2>  irq;
}

//An unpopulated USB connection
namespace usb {
typedef Gpio<PB,14> dm;
typedef Gpio<PB,15> dp;
}

//Serial port
namespace serial {
typedef Gpio<PA,9>  tx;
typedef Gpio<PA,10> rx;
}

//Unused pins, configured as pulldown
namespace unused {
typedef Gpio<PA,0>  u1;
typedef Gpio<PA,1>  u2;
typedef Gpio<PA,2>  u3;
typedef Gpio<PA,3>  u4;
typedef Gpio<PA,5>  u5;
typedef Gpio<PA,7>  u6;
typedef Gpio<PA,11> u7;
typedef Gpio<PA,12> u8;
typedef Gpio<PA,13> u9;  //Actually swdio
typedef Gpio<PA,14> u10; //Actually swclk
typedef Gpio<PC,13> u11; //Connected to comm::cs as a bug
typedef Gpio<PB,0>  u12;
typedef Gpio<PB,1>  u13;
typedef Gpio<PB,2>  u14; //Connected to GND as it is BOOT1
typedef Gpio<PB,9>  u15;
typedef Gpio<PB,10> u16;
typedef Gpio<PB,11> u17;
typedef Gpio<PB,12> u18;
typedef Gpio<PB,13> u19;
typedef Gpio<PC,3>  u20;
typedef Gpio<PC,4>  u21;
typedef Gpio<PC,5>  u22;
typedef Gpio<PC,14> u23;
typedef Gpio<PC,15> u24;
typedef Gpio<PD,3>  u25;
typedef Gpio<PD,6>  u26; //Connected to comm::cs to simplify PCB routing
typedef Gpio<PE,2>  u27;
typedef Gpio<PE,3>  u28;
typedef Gpio<PF,6>  u29;
typedef Gpio<PF,7>  u30;
typedef Gpio<PF,8>  u31;
typedef Gpio<PF,9>  u32;
typedef Gpio<PF,10> u33;
typedef Gpio<PF,11> u34;
typedef Gpio<PG,6>  u35;
typedef Gpio<PG,7>  u36;
typedef Gpio<PG,8>  u37;
typedef Gpio<PG,9>  u38;
typedef Gpio<PG,10> u39;
typedef Gpio<PG,11> u40;
typedef Gpio<PG,12> u41;
typedef Gpio<PG,13> u42;
typedef Gpio<PG,14> u43;
typedef Gpio<PG,15> u44;
}

} //namespace miosix
