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

/***********************************************************************
* bsp.cpp Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/


#include <cstdlib>
#include <inttypes.h>
#include "interfaces/bsp.h"
#ifdef WITH_FILESYSTEM
#include "kernel/filesystem/filesystem.h"
#endif //WITH_FILESYSTEM
#include "kernel/kernel.h"
#include "kernel/sync.h"
#include "interfaces/delays.h"
#include "interfaces/portability.h"
#include "interfaces/arch_registers.h"
#include "config/miosix_settings.h"
#include "kernel/logging.h"
#include "console-impl.h"

namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    //Enable all gpios
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN;
    //Pins used for FSMC (SRAM):
    //PD 0,1,4,5,7,8,9,10,11,12,14,15
    //PE 0,1,7,8,9,10,11,12,13,14,15
    //PF 0,1,2,3,4,5,12,13,14,15
    //PG 0,1,2,3,4,5
    GPIOA->OSPEEDR=0xaaaaaaaa; //Default to 50MHz speed for all GPIOS
    GPIOB->OSPEEDR=0xaaaaaaaa; //Except SRAM GPIOs that run @ 100MHz
    GPIOC->OSPEEDR=0xaaaaaaaa;
    GPIOD->OSPEEDR=0xfbffefaf;
    GPIOE->OSPEEDR=0xffffeaaf;
    GPIOF->OSPEEDR=0xffaaafff;
    GPIOG->OSPEEDR=0xaaaaafff;

	//Port config (H=high, L=low, PU=pullup, PD=pulldown)
	//  |  PORTA  |  PORTB  |  PORTC  |  PORTD  |  PORTE  |  PORTF  |  PORTG  |
	//--+---------+---------+---------+---------+---------+---------+---------+
	// 0| AF11    | AF11    | IN      | AF12    |         |         |         |
	// 1| AF11    | AF11    | AF11    | AF12    |         |         |         |
	// 2| AF11    | IN PD   | AF11    | AF12    |         |         |         |
	// 3| AF11    | AF0     | AF11    | IN PD   |         |         |         |
	// 4| OUT H   | AF0     | AF11    | AF12    |         |         |         |
	// 5| AF5     | AF5     | AF11    | AF12    |         |         |         |
	// 6| AF5     | IN PD   | OUT L   | IN PD   |         |         |         |
	// 7| AF11    | IN PD   | IN PD   | AF12    |         |         |         |
	// 8| AF0     | AF11    | AF12    | AF12    |         |         |         |
	// 9| AF7     | IN PD   | AF12    | AF12    |         |         |         |
	//10| AF7     | AF11    | AF12    | AF12    |         |         |         |
	//11| AF10    | AF11    | AF12    | AF12    |         |         |         |
	//12| AF10    | AF11    | AF12    | AF12    |         |         |         |
	//13| AF0     | AF11    | IN PU   | IN PD   |         |         |         |
	//14| AF0     | AF12    | IN PD   | AF12    |         |         |         |
	//15| AF0     | AF12    | IN PD   | AF12    |         |         |         |
	//TODO: PC0==mii_irq, requires sw pullup?
	//PC13==sdio_cd, requires sw pu?
    GPIOA->PUPDR=0xaaaaaaaa;
    GPIOB->PUPDR=0xaaaaaaaa;
    GPIOC->PUPDR=0xaaaaaaaa;
    GPIOD->PUPDR=0xaaaaaaaa;
    GPIOE->PUPDR=0xaaaaaaaa;
    GPIOF->PUPDR=0xaaaaaaaa;
    GPIOG->PUPDR=0xaaaaaaaa;

	//FIXME -- begin
	  /* Connect PDx pins to FSMC Alternate function */
  GPIOD->AFR[0]  = 0x00cc00cc;
  GPIOD->AFR[1]  = 0xcc0ccccc;
  /* Configure PDx pins in Alternate function mode */
  GPIOD->MODER   = 0xa2aa0a0a;
  /* Configure PDx pins speed to 100 MHz */
  GPIOD->OSPEEDR = 0xf3ff0f0f;
  /* Configure PDx pins Output type to push-pull */
  GPIOD->OTYPER  = 0x00000000;
  /* No pull-up, pull-down for PDx pins */
  GPIOD->PUPDR   = 0x00000000;

  /* Connect PEx pins to FSMC Alternate function */
  GPIOE->AFR[0]  = 0xc00000cc;
  GPIOE->AFR[1]  = 0xcccccccc;
  /* Configure PEx pins in Alternate function mode */
  GPIOE->MODER   = 0xaaaa800a;
  /* Configure PEx pins speed to 100 MHz */
  GPIOE->OSPEEDR = 0xffffc00f;
  /* Configure PEx pins Output type to push-pull */
  GPIOE->OTYPER  = 0x00000000;
  /* No pull-up, pull-down for PEx pins */
  GPIOE->PUPDR   = 0x00000000;

  /* Connect PFx pins to FSMC Alternate function */
  GPIOF->AFR[0]  = 0x00cccccc;
  GPIOF->AFR[1]  = 0xcccc0000;
  /* Configure PFx pins in Alternate function mode */
  GPIOF->MODER   = 0xaa000aaa;
  /* Configure PFx pins speed to 100 MHz */
  GPIOF->OSPEEDR = 0xff000fff;
  /* Configure PFx pins Output type to push-pull */
  GPIOF->OTYPER  = 0x00000000;
  /* No pull-up, pull-down for PFx pins */
  GPIOF->PUPDR   = 0x00000000;

  /* Connect PGx pins to FSMC Alternate function */
  GPIOG->AFR[0]  = 0x00cccccc;
  GPIOG->AFR[1]  = 0x000000c0;
  /* Configure PGx pins in Alternate function mode */
  GPIOG->MODER   = 0x00080aaa;
  /* Configure PGx pins speed to 100 MHz */
  GPIOG->OSPEEDR = 0x000c0fff;
  /* Configure PGx pins Output type to push-pull */
  GPIOG->OTYPER  = 0x00000000;
  /* No pull-up, pull-down for PGx pins */
  GPIOG->PUPDR   = 0x00000000;

/*-- FSMC Configuration ------------------------------------------------------*/
  /* Enable the FSMC interface clock */
  RCC->AHB3ENR         = 0x00000001;

  /* Configure and enable Bank1_SRAM2 */
  FSMC_Bank1->BTCR[2]  = 0x00001015;
  FSMC_Bank1->BTCR[3]  = 0x00010400;
  FSMC_Bank1E->BWTR[2] = 0x0fffffff;
	//FIXME -- end
    
    sram::cs1::mode(Mode::OUTPUT);
    sram::cs1::high();
    
    led::mode(Mode::OUTPUT);
    sdCardDetect::mode(Mode::INPUT_PULL_UP);
    ledOn();
    delayMs(100);
    ledOff();
    #ifndef STDOUT_REDIRECTED_TO_DCC
    IRQstm32f2serialPortInit();
    #endif //STDOUT_REDIRECTED_TO_DCC
}

void bspInit2()
{
    //Nothing to do
}

//
// Shutdown and reboot
//

/**
This function disables filesystem (if enabled), serial port (if enabled) and
puts the processor in deep sleep mode.<br>
Wakeup occurs when PA.0 goes high, but instead of sleep(), a new boot happens.
<br>This function does not return.<br>
WARNING: close all files before using this function, since it unmounts the
filesystem.<br>
When in shutdown mode, power consumption of the miosix board is reduced to ~
5uA??, however, true power consumption depends on what is connected to the GPIO
pins. The user is responsible to put the devices connected to the GPIO pin in the
minimal power consumption mode before calling shutdown(). Please note that to
minimize power consumption all unused GPIO must not be left floating.
*/
void shutdown()
{
    pauseKernel();
    #ifdef WITH_FILESYSTEM
    Filesystem& fs=Filesystem::instance();
    if(fs.isMounted()) fs.umount();
    #endif //WITH_FILESYSTEM
    //Disable interrupts
    disableInterrupts();

    /*
    Removed because low power mode causes issues with SWD programming
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; //Fuckin' clock gating...  
    PWR->CR |= PWR_CR_PDDS; //Select standby mode
    PWR->CR |= PWR_CR_CWUF;
    PWR->CSR |= PWR_CSR_EWUP; //Enable PA.0 as wakeup source
    
    SCB->SCR |= SCB_SCR_SLEEPDEEP;
    __WFE();
    NVIC_SystemReset();
    */
    for(;;) ;
}

void reboot()
{
    while(!Console::txComplete()) ;
    pauseKernel();
    //Turn off drivers
    #ifdef WITH_FILESYSTEM
    Filesystem::instance().umount();
    #endif //WITH_FILESYSTEM
    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

};//namespace miosix
