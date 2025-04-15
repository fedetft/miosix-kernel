/***************************************************************************
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
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
#include <sys/ioctl.h>
#include "interfaces/bsp.h"
#include "interfaces_private/bsp_private.h"
#include "kernel/thread.h"
#include "kernel/sync.h"
#include "interfaces/delays.h"
#include "interfaces/poweroff.h"
#include "interfaces/arch_registers.h"
#include "config/miosix_settings.h"
#include "kernel/logging.h"
#include "filesystem/file_access.h"
#include "filesystem/console/console_device.h"
#include "drivers/serial.h"
#include "drivers/dcc.h"
#include "board_settings.h"

namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    SystemCoreClockUpdate();
    //Enable all gpios
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN |
                   RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN |
                   RCC_AHBENR_GPIOFEN;
    RCC_SYNC();
    GPIOA->OSPEEDR=0xffffffff; //Default to 50MHz speed for all GPIOS
    GPIOB->OSPEEDR=0xffffffff;
    GPIOC->OSPEEDR=0xffffffff;
    GPIOD->OSPEEDR=0xffffffff;
    GPIOF->OSPEEDR=0xffffffff;
    led1::mode(Mode::OUTPUT);
    led2::mode(Mode::OUTPUT);
    ledOn();
    delayMs(100);
    ledOff();
    DefaultConsole::instance().IRQset(
        STM32SerialBase::get<defaultSerialTxPin,defaultSerialRxPin,
        defaultSerialRtsPin,defaultSerialCtsPin>(
            defaultSerial,defaultSerialSpeed,
            defaultSerialFlowctrl,defaultSerialDma));
}

void bspInit2()
{
//     #ifdef WITH_FILESYSTEM
//     basicFilesystemSetup();
//     #endif //WITH_FILESYSTEM
}

//
// Shutdown and reboot
//

void shutdown()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    fastGlobalIrqLock();
    for(;;) ;
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    
    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    fastGlobalIrqLock();
    IRQsystemReboot();
}

} //namespace miosix
