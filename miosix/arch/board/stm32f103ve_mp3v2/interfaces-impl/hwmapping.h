
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
namespace miosix {
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

//Buttons
typedef Gpio<PA,0>  button1; //Active low, generates irq for boot
typedef Gpio<PA,1>  button2; //Active low

//LED
typedef Gpio<PB,12> led; //Active high

//Display interface
namespace disp {
typedef Gpio<PB,0>  yp; //Touchscreen Y+ (analog)
typedef Gpio<PB,1>  ym; //Touchscreen Y- (analog)
typedef Gpio<PC,3>  xp; //Touchscreen X+ (analog, with 330ohm in series)
typedef Gpio<PC,4>  xm; //Touchscreen X- (analog)
typedef Gpio<PD,3>  ncpEn; //Active high
typedef Gpio<PD,14> d0;  //Handled by hardware (FSMC)
typedef Gpio<PD,15> d1;  //Handled by hardware (FSMC)
typedef Gpio<PD,0>  d2;  //Handled by hardware (FSMC)
typedef Gpio<PD,1>  d3;  //Handled by hardware (FSMC)
typedef Gpio<PE,7>  d4;  //Handled by hardware (FSMC)
typedef Gpio<PE,8>  d5;  //Handled by hardware (FSMC)
typedef Gpio<PE,9>  d6;  //Handled by hardware (FSMC)
typedef Gpio<PE,10> d7;  //Handled by hardware (FSMC)
typedef Gpio<PE,11> d8;  //Handled by hardware (FSMC)
typedef Gpio<PE,12> d9;  //Handled by hardware (FSMC)
typedef Gpio<PE,13> d10; //Handled by hardware (FSMC)
typedef Gpio<PE,14> d11; //Handled by hardware (FSMC)
typedef Gpio<PE,15> d12; //Handled by hardware (FSMC)
typedef Gpio<PD,8>  d13; //Handled by hardware (FSMC)
typedef Gpio<PD,9>  d14; //Handled by hardware (FSMC)
typedef Gpio<PD,10> d15; //Handled by hardware (FSMC)
typedef Gpio<PD,11> rs;  //Handled by hardware (FSMC)
typedef Gpio<PD,4>  rd;  //Handled by hardware (FSMC)
typedef Gpio<PD,5>  wr;  //Handled by hardware (FSMC)
typedef Gpio<PD,7>  cs;  //Handled by hardware (FSMC), 100k to +3v3a
typedef Gpio<PD,6>  reset; //Active low
}

//Audio DSP connections
namespace dsp {
typedef Gpio<PA,4>  xcs;
typedef Gpio<PA,5>  sclk;   //Handled by hardware (SPI1)
typedef Gpio<PA,6>  so;     //Handled by hardware (SPI1)
typedef Gpio<PA,7>  si;     //Handled by hardware (SPI1)
typedef Gpio<PB,5>  xreset;
typedef Gpio<PE,2>  dreq;   //Generates irq ?
typedef Gpio<PE,3>  xdcs;
}

//MicroSD connections
namespace sd {
typedef Gpio<PA,8>  cardDetect;
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
typedef Gpio<PE,4>  detect; //1K5 pullup connected to D+
}

//USB Battery charger
namespace charger {
typedef Gpio<PB,7>  done;
typedef Gpio<PB,9>  seli; //100k to ground
}

//Power management
namespace pwrmgmt {
typedef Gpio<PC,6>  vcc3v3bEn; //100k to +3v3a (active low)
typedef Gpio<PC,7>  vcc1v8En;  //100k to ground
typedef Gpio<PE,5>  vbatEn;    //If low allows to measure vbat
typedef Gpio<PC,5>  vbat;      //Analog in to measure vbat
}

//Externam FLASH
namespace xflash {
typedef Gpio<PB,6>  cs;  //100k to +3v3b
typedef Gpio<PB,13> sck; //Handled by hardware (SPI2)
typedef Gpio<PB,14> so;  //Handled by hardware (SPI2)
typedef Gpio<PB,15> si;  //Handled by hardware (SPI2)
}

//Accelerometer
namespace accel {
typedef Gpio<PC,0>  x; //Analog
typedef Gpio<PC,1>  y; //Analog
typedef Gpio<PC,2>  z; //Analog
}

//Debug/bootloader serial port
namespace boot {
typedef Gpio<PB,2>  detect; //BOOT1 (10k to ground)
typedef Gpio<PA,9>  tx;     //Handled by hardware (USART1)
typedef Gpio<PA,10> rx;     //Handled by hardware (USART1)
}

//Expansion
namespace expansion {
typedef Gpio<PE,0>  exp0;
typedef Gpio<PE,1>  exp1;
typedef Gpio<PA,2>  tx2;
typedef Gpio<PA,3>  rx2;
typedef Gpio<PB,10> tx3;
typedef Gpio<PB,11> rx3;
}

//Unused pins
typedef Gpio<PA,13> pa13;
typedef Gpio<PA,14> pa14;
typedef Gpio<PA,15> pa15;
typedef Gpio<PB,3>  pb3;
typedef Gpio<PB,4>  pb4;
typedef Gpio<PC,13> pc13;
typedef Gpio<PC,14> pc14; //This is actually for the 32KHz xtal
typedef Gpio<PC,15> pc15; //This is actually for the 32KHz xtal
typedef Gpio<PD,12> pd12;
typedef Gpio<PD,13> pd13;
typedef Gpio<PE,6>  pe6;
typedef Gpio<PB,8>  pb8; //used to be Charger::en

#ifdef _MIOSIX
} //namespace miosix
#endif //_MIOSIX

#endif //HWMAPPING_H
