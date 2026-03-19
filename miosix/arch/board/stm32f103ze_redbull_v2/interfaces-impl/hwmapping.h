/***************************************************************************
 *   Copyright (C) 2011 by Terraneo Federico                               *
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

#ifndef HWMAPPING_H
#define HWMAPPING_H

#include "interfaces/gpio.h"

//
// All GPIOs are mapped here
//

//LED
//typedef Gpio<PB,5> hwled; //Active high

namespace miosix {

//Display interface
namespace disp {
typedef Gpio<PA, 1> backlight;

typedef Gpio<PD, 4>  rd;
typedef Gpio<PD, 5>  wr;

typedef Gpio<PG, 11>  nfcs; //nCS for AT45DB161B if present on TFT module
typedef Gpio<PG, 15>  sdcs; //CS for MMC_SPI if present on TFT module

typedef Gpio<PG, 12> cs;
typedef Gpio<PF, 0> rs;

typedef Gpio<PD, 14> d0;
typedef Gpio<PD, 15> d1;
typedef Gpio<PD, 0>  d2;
typedef Gpio<PD, 1>  d3;
typedef Gpio<PE, 7>  d4;
typedef Gpio<PE, 8>  d5;
typedef Gpio<PE, 9>  d6;
typedef Gpio<PE, 10> d7;
typedef Gpio<PE, 11> d8;
typedef Gpio<PE, 12> d9;
typedef Gpio<PE, 13> d10;
typedef Gpio<PE, 14> d11;
typedef Gpio<PE, 15> d12;
typedef Gpio<PD, 8>  d13;
typedef Gpio<PD, 9>  d14;
typedef Gpio<PD, 10> d15;
}
//MicroSD connections
namespace sd {
typedef Gpio<PC,7>  cardDetect;
typedef Gpio<PC,8>  d0;  //Handled by hardware (SDIO)
typedef Gpio<PC,9>  d1;  //Handled by hardware (SDIO)
typedef Gpio<PC,10> d2;  //Handled by hardware (SDIO)
typedef Gpio<PC,11> d3;  //Handled by hardware (SDIO)
typedef Gpio<PC,12> clk; //Handled by hardware (SDIO)
typedef Gpio<PD,2>  cmd; //Handled by hardware (SDIO), 100k to +3v3b
}

//USB connections
namespace usb {
typedef Gpio<PA,11> dm;     //Handled by hardware (USB) D-
typedef Gpio<PA,12> dp;     //Handled by hardware (USB) D+
typedef Gpio<PB,5> detect; //1K pullup connected to 3.3V via bipolar PNP.
                                   //Pull this pin down to enable detection if jumper is not set
}

//SPI1 is connected to AT45DB011B (1Mbit) serial flash if J1 and J2 (DAC1 and DAC0) jumpers are set
namespace spi1 {
typedef Gpio<PA,5> sck;
typedef Gpio<PA,6> miso;
typedef Gpio<PA,7> mosi;
typedef Gpio<PA,4> nss;
}

//SPI2 is connected to ADS7843 touchscreen controller
namespace spi2 {
typedef Gpio<PB,13> sck;
typedef Gpio<PB,14> miso;
typedef Gpio<PB,15> mosi;
typedef Gpio<PB,12> ntouchss;  //used to select touch screen controller
typedef Gpio<PG,7> touchint; //touchscreen controller interrupt
}

//I2C1 is connected to 24C02WI 256-byte EEPROM
namespace i2c1 {
typedef Gpio<PB,7> sda;
typedef Gpio<PB,6> scl;
}

//Debug/bootloader serial port
namespace boot {
typedef Gpio<PB,2>  detect; //BOOT1 (10k to ground)
typedef Gpio<PA,9>  tx;     //Handled by hardware (USART1)
typedef Gpio<PA,10> rx;     //Handled by hardware (USART1)
}

namespace leds {
typedef Gpio<PF,6>   led1;
typedef Gpio<PF,7>   led2;
typedef Gpio<PF,8>   led3;
typedef Gpio<PF,9>   led4;
typedef Gpio<PF,10>  led5;
}

namespace sound {
typedef Gpio<PB,2>  buzzer; //if JP5 jumper is set (shared with boot::detect)
}

namespace buttons {
    typedef Gpio<PA,0> wakeup;
    typedef Gpio<PC,13> tamper;
    typedef Gpio<PA,8> user1;
    typedef Gpio<PD,3> user2;
}

} //namespace miosix

#endif //HWMAPPING_H
