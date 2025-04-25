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

typedef Gpio<PA,0>  strig;
typedef Gpio<PA,2>  button;
typedef Gpio<PA,13> led;
typedef Gpio<PA,14> hpled;
typedef Gpio<PA,15> sen;

namespace nrf {
typedef Gpio<PA,1>  irq;
typedef Gpio<PA,4>  cs;
typedef Gpio<PA,5>  sck;
typedef Gpio<PA,6>  miso;
typedef Gpio<PA,7>  mosi;
typedef Gpio<PA,8>  ce;
}

namespace cam {
typedef Gpio<PA,3>  irq;
typedef Gpio<PB,11> en;
typedef Gpio<PB,12> cs;
typedef Gpio<PB,13> sck;
typedef Gpio<PB,14> miso;
typedef Gpio<PB,15> mosi;
}

namespace serial {
typedef Gpio<PA,9>  tx;
typedef Gpio<PA,10> rx;
}

} //namespace miosix
