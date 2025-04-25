/***************************************************************************
 *   Copyright (C) 2015 by Terraneo Federico                               *
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

typedef Gpio<PC,11> led;

namespace expansion {
typedef Gpio<PA, 0> gpio2;
typedef Gpio<PA, 1> gpio3;
typedef Gpio<PA, 2> gpio4;
typedef Gpio<PA, 5> spi1sck;
typedef Gpio<PA, 7> spi1mosi;
typedef Gpio<PB, 4> spi1miso;
typedef Gpio<PA,15> spi1cs;
typedef Gpio<PB,13> spi2sck;
typedef Gpio<PC, 3> spi2mosi;
typedef Gpio<PC, 2> spi2miso;
typedef Gpio<PB,12> spi2cs;

}

namespace button {
typedef Gpio<PE, 2> b1;
typedef Gpio<PE, 3> b2;
typedef Gpio<PE, 4> b3;
typedef Gpio<PE, 5> b4;
}

namespace usb {
typedef Gpio<PB,14> dm;
typedef Gpio<PB,15> dp;
}

namespace display {
typedef Gpio<PC, 4> vregEn;
typedef Gpio<PG, 3> reset;
typedef Gpio<PG, 9> cs;
typedef Gpio<PG,13> sck;
typedef Gpio<PG,14> mosi;
typedef Gpio<PG, 7> dotclk;
typedef Gpio<PC, 6> hsync;
typedef Gpio<PA, 4> vsync;
typedef Gpio<PF,10> en;
typedef Gpio<PG, 6> r5;
typedef Gpio<PB, 1> r4;
typedef Gpio<PA,12> r3;
typedef Gpio<PA,11> r2;
typedef Gpio<PB, 0> r1;
typedef Gpio<PC,10> r0;
typedef Gpio<PD, 3> g5;
typedef Gpio<PC, 7> g4;
typedef Gpio<PB,11> g3;
typedef Gpio<PB,10> g2;
typedef Gpio<PG,10> g1;
typedef Gpio<PA, 6> g0;
typedef Gpio<PB, 9> b5;
typedef Gpio<PB, 6> b4;
typedef Gpio<PA, 3> b3;
typedef Gpio<PG,12> b2;
typedef Gpio<PG,11> b1;
typedef Gpio<PD, 6> b0;
}

namespace unused {
typedef Gpio<PA, 8> u1;
typedef Gpio<PB, 3> u2;
typedef Gpio<PB, 7> u3;
typedef Gpio<PC, 5> u4;
typedef Gpio<PC, 9> u5;
typedef Gpio<PC,13> u6;
typedef Gpio<PC,14> u7;
typedef Gpio<PC,15> u8;
typedef Gpio<PD, 4> u9;
typedef Gpio<PD, 5> u10;
typedef Gpio<PD,11> u11;
typedef Gpio<PD,12> u12;
typedef Gpio<PD,13> u13;
typedef Gpio<PE, 6> u14;
typedef Gpio<PF, 6> u15;
typedef Gpio<PF, 7> u16;
typedef Gpio<PF, 8> u17;
typedef Gpio<PF, 9> u18;
typedef Gpio<PG, 2> u19;
}

} //namespace miosix
