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
#include "kernel/kernel.h"
#include "kernel/sync.h"
#include "interfaces/delays.h"
#include "interfaces/portability.h"
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
    //Enable all gpios
    unreset_block_wait(RESETS_RESET_PADS_BANK0_BITS | RESETS_RESET_IO_BANK0_BITS);
    sio_hw->gpio_oe_set = SIO_GPIO_OE_SET_BITS;
    //Initialize GPIO functions
    for(unsigned int i=0; i<NUM_BANK0_GPIOS; i++)
        iobank0_hw->io[i].ctrl=Function::GPIO;

    //Blink the LED, but only on standard pico (not Pico W)
    #ifdef PICO_DEFAULT_LED_PIN
    led::mode(Mode::OUTPUT);
    ledOn();
    delayMs(100);
    ledOff();
    #endif

    //Configure serial
    uart_tx::function(Function::UART);
    uart_tx::mode(Mode::OUTPUT);
    uart_rx::function(Function::UART);
    uart_rx::mode(Mode::INPUT);
    if(defaultSerialFlowctrl)
    {
        uart_cts::function(Function::UART);
        uart_cts::mode(Mode::INPUT);
        uart_rts::function(Function::UART);
        uart_rts::mode(Mode::OUTPUT);
    }
    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
#if defined(STDOUT_REDIRECTED_TO_DCC)
        new ARMDCC()
#elif DEFAULT_SERIAL_ID == 0
        new RP2040PL011Serial0(defaultSerialSpeed,
        defaultSerialFlowctrl, defaultSerialFlowctrl)
#elif DEFAULT_SERIAL_ID == 1
        new RP2040PL011Serial1(defaultSerialSpeed,
        defaultSerialFlowctrl, defaultSerialFlowctrl)
#else
#error "No default serial port selected"
#endif
    ));
}

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    basicFilesystemSetup(intrusive_ref_ptr<Device>());
    #endif //WITH_FILESYSTEM
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
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    disableInterrupts();

    for(;;) ;
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    
    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

} //namespace miosix
