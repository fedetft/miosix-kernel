/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *   Copyright (C) 2023, 2024 by Daniele Cattaneo                          *
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

//LEDs
typedef Gpio<PB,0> led1;
typedef Gpio<PB,7> led2;
typedef Gpio<PB,14> led3;

//User button
typedef Gpio<PC,13> btn;

//Serial port 
namespace serial {
typedef Gpio<PD,8> tx;
typedef Gpio<PD,9> rx;
}

//MicroSD card slot exposed on ST Zio connector
namespace sdio {
typedef Gpio<PC,8>  d0;
typedef Gpio<PC,9>  d1;
typedef Gpio<PC,10> d2;
typedef Gpio<PC,11> d3;
typedef Gpio<PC,12> ck;
typedef Gpio<PD,2>  cmd;
}

//USB OTG A/B port
namespace usb {
typedef Gpio<PA,9>  vbus;
typedef Gpio<PA,10> id;
typedef Gpio<PA,11> dm;
typedef Gpio<PA,12> dp;
typedef Gpio<PG,6>  on;
}

//Gpios that connect the ethernet transceiver to the microcontroller
namespace rmii {
typedef Gpio<PG,11> txen;
typedef Gpio<PG,13> txd0;
typedef Gpio<PB,13> txd1;
typedef Gpio<PC,4>  rxd0;
typedef Gpio<PC,5>  rxd1;
typedef Gpio<PA,7>  crsdv;
typedef Gpio<PC,1>  mdc;
typedef Gpio<PA,2>  mdio;
typedef Gpio<PA,1>  refclk;
}

//32kHz clock pins used by the RTC
namespace rtc {
typedef Gpio<PC,14> osc32in;
typedef Gpio<PC,15> osc32out;
}

//SWD lines
namespace swd {
typedef Gpio<PB,3>  swo;
typedef Gpio<PA,13> tms;
typedef Gpio<PA,14> tck;
}

//Free pins
namespace unused {
typedef Gpio<PA,0>  pa0;
typedef Gpio<PA,3>  pa3;
typedef Gpio<PA,4>  pa4;
typedef Gpio<PA,5>  pa5;
typedef Gpio<PA,6>  pa6;
typedef Gpio<PA,8>  pa8;
typedef Gpio<PA,15> pa15;
typedef Gpio<PB,1>  pb1;
typedef Gpio<PB,2>  pb2; //BOOT1
typedef Gpio<PB,4>  pb4;
typedef Gpio<PB,5>  pb5;
typedef Gpio<PB,6>  pb6;
typedef Gpio<PB,8>  pb8;
typedef Gpio<PB,9>  pb9;
typedef Gpio<PB,10> pb10;
typedef Gpio<PB,11> pb11;
typedef Gpio<PB,12> pb12;
typedef Gpio<PB,15> pb15;
typedef Gpio<PC,0>  pc0;
typedef Gpio<PC,2>  pc2;
typedef Gpio<PC,3>  pc3;
typedef Gpio<PC,6>  pc6;
typedef Gpio<PC,7>  pc7;
typedef Gpio<PD,0>  pd0;
typedef Gpio<PD,1>  pd1;
typedef Gpio<PD,3>  pd3;
typedef Gpio<PD,4>  pd4;
typedef Gpio<PD,5>  pd5;
typedef Gpio<PD,6>  pd6;
typedef Gpio<PD,7>  pd7;
typedef Gpio<PD,10> pd10;
typedef Gpio<PD,11> pd11;
typedef Gpio<PD,12> pd12;
typedef Gpio<PD,13> pd13;
typedef Gpio<PD,14> pd14;
typedef Gpio<PD,15> pd15;
typedef Gpio<PE,0>  pe0;
typedef Gpio<PE,1>  pe1;
typedef Gpio<PE,2>  pe2;
typedef Gpio<PE,3>  pe3;
typedef Gpio<PE,4>  pe4;
typedef Gpio<PE,5>  pe5;
typedef Gpio<PE,6>  pe6;
typedef Gpio<PE,7>  pe7;
typedef Gpio<PE,8>  pe8;
typedef Gpio<PE,9>  pe9;
typedef Gpio<PE,10> pe10;
typedef Gpio<PE,11> pe11;
typedef Gpio<PE,12> pe12;
typedef Gpio<PE,13> pe13;
typedef Gpio<PE,14> pe14;
typedef Gpio<PE,15> pe15;
typedef Gpio<PF,0>  pf0; //PH0
typedef Gpio<PF,1>  pf1; //PH1
typedef Gpio<PF,2>  pf2;
typedef Gpio<PF,3>  pf3;
typedef Gpio<PF,4>  pf4;
typedef Gpio<PF,5>  pf5;
typedef Gpio<PF,6>  pf6;
typedef Gpio<PF,7>  pf7;
typedef Gpio<PF,8>  pf8;
typedef Gpio<PF,9>  pf9;
typedef Gpio<PF,10> pf10;
typedef Gpio<PF,11> pf11;
typedef Gpio<PF,12> pf12;
typedef Gpio<PF,13> pf13;
typedef Gpio<PF,14> pf14;
typedef Gpio<PF,15> pf15;
typedef Gpio<PG,0>  pg0;
typedef Gpio<PG,1>  pg1;
typedef Gpio<PG,2>  pg2;
typedef Gpio<PG,3>  pg3;
typedef Gpio<PG,4>  pg4;
typedef Gpio<PG,5>  pg5;
typedef Gpio<PG,7>  pg7;
typedef Gpio<PG,8>  pg8;
typedef Gpio<PG,9>  pg9;
typedef Gpio<PG,10> pg10;
typedef Gpio<PG,12> pg12;
typedef Gpio<PG,14> pg14;
typedef Gpio<PG,15> pg15;
typedef Gpio<PH,0>  ph0;
typedef Gpio<PH,1>  ph1;
typedef Gpio<PH,2>  ph2;
typedef Gpio<PH,3>  ph3;
typedef Gpio<PH,4>  ph4;
typedef Gpio<PH,5>  ph5;
typedef Gpio<PH,6>  ph6;
typedef Gpio<PH,7>  ph7;
typedef Gpio<PH,8>  ph8;
typedef Gpio<PH,9>  ph9;
typedef Gpio<PH,10> ph10;
typedef Gpio<PH,11> ph11;
typedef Gpio<PH,12> ph12;
typedef Gpio<PH,13> ph13;
typedef Gpio<PH,14> ph14;
typedef Gpio<PH,15> ph15;
}

} //namespace miosix
