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
#include "interfaces_private/smp.h"
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
#include "drivers/rp2040_spi.h"
#include "drivers/sw_spi.h"
#include "drivers/spi_sd.h"

namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    // Update SystemCoreClock
    SystemCoreClock = cpuFrequency;
    
    //Enable all gpios
    unreset_block_wait(RESETS_RESET_PADS_BANK0_BITS | RESETS_RESET_IO_BANK0_BITS);
    sio_hw->gpio_oe_set = SIO_GPIO_OE_SET_BITS;
    //Initialize GPIO functions
    for(unsigned int i=0; i<NUM_BANK0_GPIOS; i++)
        iobank0_hw->io[i].ctrl=toUint(Function::GPIO);

    //Blink the LED, but only on standard pico (not Pico W)
    #ifdef PICO_DEFAULT_LED_PIN
    led::mode(Mode::OUTPUT);
    ledOn();
    delayMs(100);
    ledOff();
    #endif

    IRQsetDefaultConsole(
        RP2040SerialBase::get<defaultSerialTxPin,defaultSerialRxPin,
        defaultSerialRtsPin,defaultSerialCtsPin>(
            defaultSerial,defaultSerialSpeed,
            defaultSerialFlowctrl,defaultSerialDma));
}

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    if(!enableSdCard)
    {
        basicFilesystemSetup(intrusive_ref_ptr<Device>());
    } else {
        if(defaultSdCardDriver==SdCardDriverType::Dma)
        {
            auto spi=new RP2040PL022DmaSpi(
                defaultSdCardSPI,100*1000,false,false,
                defaultSdCardSPISiPin::getPin(),
                defaultSdCardSPISoPin::getPin(),
                defaultSdCardSPISckPin::getPin(),
                GpioPin());
            auto sd=new SPISD<RP2040PL022DmaSpi>(
                std::unique_ptr<RP2040PL022DmaSpi>(spi),
                defaultSdCardSPICsPin::getPin());
            basicFilesystemSetup(intrusive_ref_ptr<SPISD<RP2040PL022DmaSpi>>(sd));
        } else if(defaultSdCardDriver==SdCardDriverType::NoDma) {
            auto spi=new RP2040PL022Spi(
                defaultSdCardSPI,100*1000,false,false,
                defaultSdCardSPISiPin::getPin(),
                defaultSdCardSPISoPin::getPin(),
                defaultSdCardSPISckPin::getPin(),
                GpioPin());
            auto sd=new SPISD<RP2040PL022Spi>(
                std::unique_ptr<RP2040PL022Spi>(spi),
                defaultSdCardSPICsPin::getPin());
            basicFilesystemSetup(intrusive_ref_ptr<SPISD<RP2040PL022Spi>>(sd));
        } else { // SdCardDriverType::Software
            using SPIType=SwSPI<defaultSdCardSPISiPin,defaultSdCardSPISoPin,
                defaultSdCardSPISckPin>;
            auto spi=new SPIType(100*1000);
            auto sd=new SPISD<SPIType>(
                std::unique_ptr<SPIType>(spi),
                defaultSdCardSPICsPin::getPin());
            basicFilesystemSetup(intrusive_ref_ptr<SPISD<SPIType>>(sd));
        }
    }
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

    FastGlobalIrqLock::lock();
    // Everybody, go to sleep!
    clocks_hw->sleep_en0=0;
    clocks_hw->sleep_en1=0;
    #ifdef WITH_SMP
    IRQlockupOtherCores();
    #endif
    FastGlobalLockFromIrq::unlock();
    for(;;) __WFE();
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    
    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    FastGlobalIrqLock::lock();
    IRQsystemReboot();
}

} //namespace miosix
