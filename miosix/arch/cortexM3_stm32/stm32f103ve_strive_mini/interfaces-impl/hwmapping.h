
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
 ***************************************************************************
 *
 * *****************
 * Version 1.01 beta
 * 01/03/2011
 * *****************
 */

#ifndef HWMAPPING_H
#define HWMAPPING_H

#ifdef _MIOSIX
#include "interfaces/gpio.h"
#else //_MIOSIX
#include "gpio.h"

//These two functions are only available from the bootloader
/**
 * Enable +3v3b and +1v8 domains, etc...
 */
void powerOn();

/**
 * Disable +3v3b and +1v8 domains, etc...
 */
void powerOff();

#endif //_MIOSIX

//
// All GPIOs are mapped here
//

//LED
//typedef Gpio<GPIOB_BASE,5> hwled; //Active high

//Display interface
namespace disp {
typedef Gpio<GPIOD_BASE, 13> ncpEn;
typedef Gpio<GPIOE_BASE, 1>  reset;

typedef Gpio<GPIOD_BASE, 4>  rd;
typedef Gpio<GPIOD_BASE, 5>  wr;

typedef Gpio<GPIOD_BASE, 7>  cs;
typedef Gpio<GPIOD_BASE, 11> rs;

typedef Gpio<GPIOD_BASE, 14> d0;
typedef Gpio<GPIOD_BASE, 15> d1;
typedef Gpio<GPIOD_BASE, 0>  d2;
typedef Gpio<GPIOD_BASE, 1>  d3;
typedef Gpio<GPIOE_BASE, 7>  d4;
typedef Gpio<GPIOE_BASE, 8>  d5;
typedef Gpio<GPIOE_BASE, 9>  d6;
typedef Gpio<GPIOE_BASE, 10> d7;
typedef Gpio<GPIOE_BASE, 11> d8;
typedef Gpio<GPIOE_BASE, 12> d9;
typedef Gpio<GPIOE_BASE, 13> d10;
typedef Gpio<GPIOE_BASE, 14> d11;
typedef Gpio<GPIOE_BASE, 15> d12;
typedef Gpio<GPIOD_BASE, 8>  d13;
typedef Gpio<GPIOD_BASE, 9>  d14;
typedef Gpio<GPIOD_BASE, 10> d15;
}
//MicroSD connections
namespace sd {
//typedef Gpio<GPIOA_BASE,8>  cardDetect; -- is absent on Strive board
typedef Gpio<GPIOC_BASE,8>  d0;  //Handled by hardware (SDIO)
typedef Gpio<GPIOC_BASE,9>  d1;  //Handled by hardware (SDIO)
typedef Gpio<GPIOC_BASE,10> d2;  //Handled by hardware (SDIO)
typedef Gpio<GPIOC_BASE,11> d3;  //Handled by hardware (SDIO)
typedef Gpio<GPIOC_BASE,12> clk; //Handled by hardware (SDIO)
typedef Gpio<GPIOD_BASE,2>  cmd; //Handled by hardware (SDIO), 100k to +3v3b
}

//USB connections
namespace usb {
typedef Gpio<GPIOA_BASE,11> dm;     //Handled by hardware (USB) D-
typedef Gpio<GPIOA_BASE,12> dp;     //Handled by hardware (USB) D+
typedef Gpio<GPIOC_BASE,13> detect; //1K pullup connected to 3.3V via bipolar PNP.
                                    //Pull this pin down to enable detection.
}

//Debug/bootloader serial port
namespace boot {
typedef Gpio<GPIOB_BASE,2>  detect; //BOOT1 (10k to ground)
typedef Gpio<GPIOA_BASE,9>  tx;     //Handled by hardware (USART1)
typedef Gpio<GPIOA_BASE,10> rx;     //Handled by hardware (USART1)
}

#endif //HWMAPPING_H
