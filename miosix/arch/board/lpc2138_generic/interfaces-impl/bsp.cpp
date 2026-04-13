/***************************************************************************
 *   Copyright (C) 2008-2026 by Terraneo Federico                          *
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

#include <sys/ioctl.h>
#include "interfaces/bsp.h"
#include "interfaces_private/bsp_private.h"
#include "interfaces/serial.h"
#include "drivers/sdmmc/sd_lpc2000.h"
#include "interfaces/poweroff.h"
#include "miosix_settings.h"
#include "interfaces/arch_registers.h"
#include "filesystem/file_access.h"
#include "filesystem/console/console_device.h"

namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    //Clearing PINSEL registers. All pin are GPIO by default
    PINSEL0=0;
    PINSEL1=0;
    // Configure all pins as input by default
    IODIR0=0;
    IODIR1=0;

    //Init serial port
    IRQsetDefaultConsole(
        intrusive_ref_ptr<Device>(new LPC2000Serial(0,SERIAL_PORT_SPEED)));
}

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    basicFilesystemSetup(SPISDDriver::instance());
    #endif //WITH_FILESYSTEM
}

void shutdown()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    FastGlobalIrqLock::lock();
    for(;;) PCON|=0b10; //PD bit
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
