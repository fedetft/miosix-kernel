/***************************************************************************
 *   Copyright (C) 2023 by Terraneo Federico                               *
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
#include "board_settings.h"

namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    MSC->CTRL=0; //Generate bus fault on access to unmapped areas

    //
    // Setup GPIOs
    //
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

    //
    // Setup clocks, as when we get here we're still running with HFRCO
    //
    //Configure flash wait states and dividers
    CMU->HFCORECLKDIV=0;
    CMU->HFPERCLKDIV=CMU_HFPERCLKDIV_HFPERCLKEN;
    #if EFM32_HFXO_FREQ>16000000 || EFM32_HFRCO_FREQ>16000000
    MSC->READCTRL=MSC_READCTRL_MODE_WS1;
    #else
    MSC->READCTRL=MSC_READCTRL_MODE_WS0;
    #endif

    #if defined(EFM32_HFXO_FREQ)
    //Select HFXO
    CMU->OSCENCMD=CMU_OSCENCMD_HFXOEN;
    //Then switch immediately to HFXO
    CMU->CMD=CMU_CMD_HFCLKSEL_HFXO;
    //Disable HFRCO since we don't need it anymore
    CMU->OSCENCMD=CMU_OSCENCMD_HFRCODIS;
    #elif defined(EFM32_HFRCO_FREQ)
    //Pointer to table of HFRCO calibration values in device information page
    unsigned char *diHfrcoCalib=reinterpret_cast<unsigned char*>(0x0fe081dc);
    #if EFM32_HFRCO_FREQ==1000000
    CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_1MHZ | diHfrcoCalib[0];
    #elif EFM32_HFRCO_FREQ==7000000
    CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_7MHZ | diHfrcoCalib[1];
    #elif EFM32_HFRCO_FREQ==11000000
    CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_11MHZ | diHfrcoCalib[2];
    #elif EFM32_HFRCO_FREQ==14000000
    CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_14MHZ | diHfrcoCalib[3];
    #elif EFM32_HFRCO_FREQ==21000000
    CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_21MHZ | diHfrcoCalib[4];
    #elif EFM32_HFRCO_FREQ==28000000
    CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_28MHZ | diHfrcoCalib[5];
    #else
    #error EFM32_HFRCO_FREQ not valid
    #endif
    #else
    #error EFM32_HFXO_FREQ nor EFM32_HFRCO_FREQ defined
    #endif

    //This function initializes the SystemCoreClock variable. It is put here
    //so as to get the right value
    SystemCoreClockUpdate();
    
    //
    // Setup serial port
    //
    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
        new EFM32Serial(defaultSerial,defaultSerialSpeed)));
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

    disableInterrupts();

//     //Serial port is causing some residual consumption
//     USART0->CMD=USART_CMD_TXDIS | USART_CMD_RXDIS;
//     USART0->ROUTE=0;
//     debugConnector::tx::mode(Mode::DISABLED);
//     debugConnector::rx::mode(Mode::DISABLED);
//
//     //Sequence to enter EM4
//     for(int i=0;i<5;i++)
//     {
//         EMU->CTRL=2<<2;
//         EMU->CTRL=3<<2;
//     }
//     //Should never reach here
    miosix_private::IRQsystemReboot();
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

} //namespace miosix
