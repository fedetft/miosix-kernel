/***************************************************************************
 *   Copyright (C) 2016 by Terraneo Federico                               *
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

namespace leds {
using led0  = Gpio<PB, 0>; //Green, mapped to ledOn()/ledOff()
using led1  = Gpio<PA,15>; //Green
using led2  = Gpio<PB, 4>; //Red
using led3  = Gpio<PC, 4>; //Red
using led4  = Gpio<PC, 5>; //Red
using led5  = Gpio<PF, 6>; //Red
using led6  = Gpio<PD, 3>; //Red
using led7  = Gpio<PD, 4>; //Red
using led8  = Gpio<PG, 2>; //Red
using led9  = Gpio<PG, 3>; //Red
} //namespace leds

namespace sensors {

//Shared SPI bus among sensors
using sck   = Gpio<PA, 5>;
using miso  = Gpio<PA, 6>;
using mosi  = Gpio<PA, 7>;

//Shared I2C bus among sensors
using sda   = Gpio<PB, 7>;
using scl   = Gpio<PB, 8>;

//Gyro, SPI, 2MHz SCK max
namespace fxas21002 {
using cs    = Gpio<PG,10>;
using int1  = Gpio<PD, 7>;
using int2  = Gpio<PA, 8>;
} //namespace fxas21002

//Barometer, SPI, <FIXME>MHz SCK max
namespace lps331 {
using cs    = Gpio<PE, 4>;
using int1  = Gpio<PA, 2>;
using int2  = Gpio<PA, 3>;    
} //namespace lps331

//IMU, SPI, 10MHz SCK max
namespace lsm9ds {
using csg   = Gpio<PG, 9>;
using csm   = Gpio<PG,11>;
using drdyg = Gpio<PB,14>;
using int1g = Gpio<PD,11>;
using int1m = Gpio<PD,12>;
using int2m = Gpio<PC,13>;
} //namespace lsm9ds

//IMU, SPI, 10MHz SCK max
namespace max21105 {
using cs    = Gpio<PE, 2>;
using int1  = Gpio<PE, 5>;
using int2  = Gpio<PE, 6>;
} //namespace max21105

//Thermocouple, SPI, 5MHz SCK max
namespace max31856 {
using cs    = Gpio<PB,11>;
using drdy  = Gpio<PB, 9>;
using fault = Gpio<PF,10>;
} //namespace max31856

//Barometer, I2C, 400KHz SCL max
namespace mpl3115 {
using int1  = Gpio<PA, 1>;
using int2  = Gpio<PA, 4>;
} //namespace mpl5115

//IMU, SPI, 1MHz SCK max
namespace mpu9250 {
using cs    = Gpio<PD,13>;
using int1  = Gpio<PB,15>;
} //namespace mpu9250

//Barometer, SPI, 20MHz SCK max
namespace ms5803 {
using cs    = Gpio<PB,10>;
} //namespace ms5803

} //namespace sensors

namespace can {
using tx1   = Gpio<PA,12>;
using rx1   = Gpio<PA,11>;
using tx2   = Gpio<PB,13>;
using rx2   = Gpio<PB,12>;
} //namespace can

namespace eth {
using miso  = Gpio<PF, 8>;
using mosi  = Gpio<PF, 9>;
using sck   = Gpio<PF, 7>;
using cs    = Gpio<PE, 3>;
using int1  = Gpio<PC, 1>;
} //namespace eth

namespace wireless {
//FIXME cs is missing
using miso  = Gpio<PG,12>;
using mosi  = Gpio<PG,14>;
using sck   = Gpio<PG,13>;
} //namespace wireless

namespace batteryManager {
using currentSense = Gpio<PB, 1>; //analog
using voltageSense = Gpio<PC, 3>; //analog  
} //namespace batteryManager

namespace misc {
using wkup     = Gpio<PA, 0>;
using adc1     = Gpio<PC, 2>; //analog
using usart6tx = Gpio<PC, 6>;
using usart6rx = Gpio<PC, 7>;
using bootsel0 = Gpio<PG, 6>;
using bootsel1 = Gpio<PG, 7>;
} //namespace misc

namespace piksi {
using tx       = Gpio<PD, 5>;
using rx       = Gpio<PD, 6>;
} //namespace piksi

} //namespace miosix
