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

/***********************************************************************
 * bsp.cpp Part of the Miosix Embedded OS.
 * Board support package, this file initializes hardware.
 ************************************************************************/

#include "interfaces/bsp.h"

#include <inttypes.h>
#include <sys/ioctl.h>

#include <cstdlib>

#include "board_settings.h"
#include "config/miosix_settings.h"
#include "drivers/serial.h"
#include "drivers/serial_stm32.h"
#include "drivers/sd_stm32f2_f4_f7.h"
#include "filesystem/console/console_device.h"
#include "filesystem/file_access.h"
#include "interfaces/arch_registers.h"
#include "interfaces/delays.h"
#include "interfaces/portability.h"
#include "kernel/kernel.h"
#include "kernel/logging.h"
#include "kernel/sync.h"

namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    // Enable all gpios
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN |
                    RCC_AHB1ENR_GPIOIEN;
    RCC_SYNC();

    // Default to 50MHz speed for all GPIOS except FMC
    //              port F  E  D  C  B  A  9  8  7  6  5  4  3  2  1  0
    GPIOA->OSPEEDR  = 0b10'10'10'10'10'10'10'10'10'10'10'10'10'10'10'10;
    GPIOB->OSPEEDR  = 0b10'10'10'10'10'10'10'10'10'10'10'10'10'10'10'10;
    GPIOC->OSPEEDR  = 0b10'10'10'10'10'10'10'10'10'10'10'10'10'10'10'10;
    GPIOD->OSPEEDR |= 0b00'00'10'10'10'00'00'00'10'10'10'10'10'10'00'00;
    GPIOE->OSPEEDR |= 0b00'00'00'00'00'00'00'00'00'10'10'10'10'10'00'00;
    GPIOF->OSPEEDR |= 0b00'00'00'00'00'10'10'10'10'10'00'00'00'00'00'00;
    GPIOG->OSPEEDR |= 0b00'10'10'10'10'10'10'10'10'10'00'00'10'00'00'00;
    GPIOH->OSPEEDR |= 0b00'00'00'00'00'00'00'00'00'00'00'10'00'00'10'10;
    GPIOI->OSPEEDR |= 0b00'00'00'00'10'00'00'10'00'00'00'00'00'00'00'00;

    led1::mode(Mode::OUTPUT);
    led2::mode(Mode::OUTPUT);
    btn0::mode(Mode::INPUT);
    btn1::mode(Mode::INPUT);
    btn2::mode(Mode::INPUT);
    btn3::mode(Mode::INPUT);
    sdmmcCD::mode(Mode::INPUT);
    sdmmcWP::mode(Mode::INPUT);

    ledOn();
    delayMs(100);
    ledOff();
    auto tx=Gpio<GPIOB_BASE,10>::getPin(); tx.alternateFunction(7);
    auto rx=Gpio<GPIOB_BASE,11>::getPin(); rx.alternateFunction(7);
    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
        new STM32Serial(3, defaultSerialSpeed, tx, rx)));
}

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    basicFilesystemSetup(SDIODriver::instance());
    #endif  //WITH_FILESYSTEM
}

//
// Shutdown and reboot
//

void shutdown()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif  //WITH_FILESYSTEM

    disableInterrupts();
    for(;;) ;
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif  //WITH_FILESYSTEM

    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

}  // namespace miosix
