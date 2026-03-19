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
#include "board_settings.h"
#include "mpu/cortexMx_mpu.h"

namespace miosix {

//Replaced by constants in board_settings.h
// unsigned int getPeripheralClock()
// {
//     unsigned int result=SystemCoreClock;
//     //EFM32 has separate prescalers for core and peripherals, so we start
//     //from HFCORECLK, work our way up to HFCLK and then down to HFPERCLK
//     result*=1<<(CMU->HFCORECLKDIV & _CMU_HFCORECLKDIV_HFCORECLKDIV_MASK);
//     result/=1<<(CMU->HFPERCLKDIV & _CMU_HFPERCLKDIV_HFPERCLKDIV_MASK);
//     return result;
// }

//
// Initialization
//

/**
 * This function is the first function called during boot to initialize the
 * platform memory and clock subsystems.
 *
 * Code in this function has several important restrictions:
 * - When this function is called, part of the memory address space may not be
 *   available. This occurs when the board includes an external memory, and
 *   indeed it is the purpose of this very function to enable the external
 *   memory (if present) and map it into the address space!
 * - This function is called before global and static variables in .data/.bss
 *   are initialized. As a consequence, this function and all function it calls
 *   are forbidden from referencing global and static variables
 * - This function is called with the stack pointer pointing to the interrupt
 *   stack. This is in general a small stack, but is the only stack that is
 *   guaranteed to be in the internal memory. The allocation of stack-local
 *   variables and the nesting of function calls should be kept to a minimum
 * - This function is called with interrupts disabled, before the kernel is
 *   started and before the I/O subsystem is enabled. There is thus no way
 *   of printing any debug message.
 *
 * This function should perform the following operations:
 * - Configure the internal memory wait states to support the desired target
 *   operating frequency
 * - Configure the CPU clock (e.g: PLL) to run at the desired target frequency
 * - Enable and configure the external memory (if available)
 *
 * As a postcondition of running this function, the entire memory map as
 * specified in the linker script should be accessible, so the rest of the
 * kernel can use the memory to complete the boot sequence, and the CPU clock
 * should be configured at the desired target frequency so the boot can proceed
 * quickly.
 */
void IRQmemoryAndClockInit()
{
    //Validate frequency
    static_assert(cpuFrequency==oscillatorFrequency, "prescaling unsupported");
    static_assert(peripheralFrequency==oscillatorFrequency, "prescaling unsupported");
    static_assert(oscillatorType==OscillatorType::HFXO
               || oscillatorFrequency==1200000
               || oscillatorFrequency==6600000
               || oscillatorFrequency==11000000
               || oscillatorFrequency==14000000
               || oscillatorFrequency==21000000
               || oscillatorFrequency==28000000, "HFRCO frequency unsupported");

    //Configure flash wait states
    if(cpuFrequency>16000000) MSC->READCTRL=MSC_READCTRL_MODE_WS1;
    else MSC->READCTRL=MSC_READCTRL_MODE_WS0;

    //Configure prescalers
    CMU->HFCORECLKDIV=0;
    CMU->HFPERCLKDIV=CMU_HFPERCLKDIV_HFPERCLKEN;
    //Configure oscillator
    if(oscillatorType==OscillatorType::HFXO)
    {
        //Select HFXO
        CMU->OSCENCMD=CMU_OSCENCMD_HFXOEN;
        //Then switch immediately to HFXO
        CMU->CMD=CMU_CMD_HFCLKSEL_HFXO;
        //Disable HFRCO since we don't need it anymore
        CMU->OSCENCMD=CMU_OSCENCMD_HFRCODIS;
    } else {
        //Pointer to table of HFRCO calibration values in device information page
        unsigned char *diHfrcoCalib=reinterpret_cast<unsigned char*>(0x0fe081dc);
        if(oscillatorFrequency==1200000)
            CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_1MHZ | diHfrcoCalib[0];
        else if(oscillatorFrequency==6600000)
            CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_7MHZ | diHfrcoCalib[1];
        else if(oscillatorFrequency==11000000)
            CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_11MHZ | diHfrcoCalib[2];
        else if(oscillatorFrequency==14000000)
            CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_14MHZ | diHfrcoCalib[3];
        else if(oscillatorFrequency==21000000)
            CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_21MHZ | diHfrcoCalib[4];
        else if(oscillatorFrequency==28000000)
            CMU->HFRCOCTRL=CMU_HFRCOCTRL_BAND_28MHZ | diHfrcoCalib[5];
    }
    
    // Architecture has MPU, enable kernel-level W^X protection
    IRQconfigureMPU();
}

void IRQbspInit()
{
    MSC->CTRL=0; //Generate bus fault on access to unmapped areas

    //
    // Setup GPIOs
    //
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;
    
    //
    // Setup serial port
    //
    if(defaultSerial==0)
    {
        using tx = Gpio<PE,10>;
        using rx = Gpio<PE,11>;
        IRQsetDefaultConsole(intrusive_ref_ptr<Device>(
            new EFM32Serial(defaultSerial,defaultSerialSpeed,
                            tx::getPin(),rx::getPin())));
    } else {
        using tx = Gpio<PC,0>;
        using rx = Gpio<PC,1>;
        IRQsetDefaultConsole(intrusive_ref_ptr<Device>(
            new EFM32Serial(defaultSerial,defaultSerialSpeed,
                            tx::getPin(),rx::getPin())));
    }
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

    FastGlobalIrqLock::lock();

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
    IRQsystemReboot();
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    FastGlobalIrqLock::lock();
    IRQsystemReboot();
}

} //namespace miosix
