/***************************************************************************
 *   Copyright (C) 2018 by Terraneo Federico                               *
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

namespace interfaces {

namespace spi1 {
using sck   = Gpio<PA, 5>;
using miso  = Gpio<PA, 6>;
using mosi  = Gpio<PA, 7>;
} //namespace spi1

namespace spi2 {
using sck   = Gpio<PB, 13>;
using miso  = Gpio<PB, 14>;
using mosi  = Gpio<PB, 15>;
} //namespace spi1

namespace i2c {
using scl   = Gpio<PB, 8>;
using sda   = Gpio<PB, 9>;
} //namespace i2c

namespace uart4 {
using tx    = Gpio<PA, 0>;
using rx    = Gpio<PA, 1>;
} //namespace uart4

namespace can {
using rx    = Gpio<PA, 11>;
using tx    = Gpio<PA, 12>;
} // namespace can
} //namespace interfaces

namespace sensors {
    
namespace adis16405 {
using cs    = Gpio<PA, 8>;
using dio1  = Gpio<PB, 4>;
using nrst  = Gpio<PD, 5>;
using ckIn  = Gpio<PD, 14>;
} //namespace adis16405

namespace ad7994 {
using ab      = Gpio<PB, 1>;
using nconvst = Gpio<PG, 9>;
} //namespace ad7994

namespace max21105 {
using cs    = Gpio<PC, 1>;
} //namespace max21105

namespace mpu9250 {
using cs    = Gpio<PC, 3>;
} //namespace mpu9250

namespace ms5803 {
using cs    = Gpio<PD, 7>;
} //namespace ms5803
} //namespace sensors

namespace actuators {
namespace hbridgel {
using ena   = Gpio<PD, 11>;
using in    = Gpio<PD, 13>;
using csens = Gpio<PF, 8>;
} //namespace hbridgel

namespace hbridger {
using ena   = Gpio<PD, 9>;
using in    = Gpio<PD, 12>;
using csens = Gpio<PF, 6>;
} //namespace hbridger
} //namespace actuators

namespace InAir9B {
using dio0  = Gpio<PB, 0>;
using dio1  = Gpio<PC, 4>;
using dio2  = Gpio<PA, 4>;
using dio3  = Gpio<PC, 2>;
using cs    = Gpio<PF, 9>;
using nrst  = Gpio<PF, 7>;
} //namespace InAir9B
} //namespace miosix
