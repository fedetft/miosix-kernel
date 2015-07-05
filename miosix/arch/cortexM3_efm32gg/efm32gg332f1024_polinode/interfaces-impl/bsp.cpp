/***************************************************************************
 *   Copyright (C) 2015 by Terraneo Federico                               *
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

void oscillatorInit()
{
    //The startup of the HFXO oscillator was measured and takes less than 125us
    
    MSC->READCTRL=MSC_READCTRL_MODE_WS2; //Two wait states for f>32MHz
    
    unsigned int dontChange=CMU->CTRL & CMU_CTRL_LFXOBUFCUR;
    CMU->CTRL=CMU_CTRL_HFLE                  //We run at a frequency > 32MHz
            | CMU_CTRL_CLKOUTSEL1_LFXOQ      //For when we will enable clock out
            | CMU_CTRL_LFXOTIMEOUT_32KCYCLES //Long timeout for LFXO startup
            | CMU_CTRL_HFXOTIMEOUT_1KCYCLES  //Small timeout for HFXO startup...
            | CMU_CTRL_HFXOGLITCHDETEN       //...but reset timeout if any glitch
            | CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ //We run at a frequency > 32MHz
            | CMU_CTRL_HFXOBOOST_100PCENT    //Maximum startup boost current
            | dontChange;                    //Don't change some of the bits
    
    //First, enable HFXO (and LFXO since we will need it later)
    CMU->OSCENCMD=CMU_OSCENCMD_HFXOEN | CMU_OSCENCMD_LFXOEN;
    //Then switch immediately to HFXO (this stalls the CPU and peripherals
    //till the HFXO is stable)
    CMU->CMD=CMU_CMD_HFCLKSEL_HFXO;
    //Last, disable HFRCO since we don't need it
    CMU->OSCENCMD=CMU_OSCENCMD_HFRCODIS;
}

void IRQbspInit()
{
    //Enable Gpio
    CMU->HFPERCLKEN0|=CMU_HFPERCLKEN0_GPIO;
    GPIO->CTRL=GPIO_CTRL_EM4RET; //GPIOs keep their state in EM4
    
    MSC->WRITECTRL=MSC_WRITECTRL_RWWEN;  //Enable FLASH read while write support
    
    redLed::mode(Mode::OUTPUT_LOW);
    greenLed::mode(Mode::OUTPUT_LOW);
    userButton::mode(Mode::Mode::INPUT_PULL_UP_FILTER);
    loopback32KHzIn::mode(Mode::INPUT);
    loopback32KHzOut::mode(Mode::OUTPUT);
    
    internalSpi::mosi::mode(Mode::OUTPUT_LOW);
    internalSpi::miso::mode(Mode::INPUT_PULL_DOWN); //To prevent it from floating
    internalSpi::sck::mode(Mode::OUTPUT_LOW);
    
    transceiver::cs::mode(Mode::OUTPUT_LOW);
    transceiver::reset::mode(Mode::OUTPUT_LOW);
    transceiver::vregEn::mode(Mode::OUTPUT_LOW);
    transceiver::gpio1::mode(Mode::OUTPUT_LOW);
    transceiver::gpio1::mode(Mode::OUTPUT_LOW);
    transceiver::gpio2::mode(Mode::OUTPUT_LOW);
    transceiver::excChB::mode(Mode::OUTPUT_LOW);
    transceiver::gpio4::mode(Mode::OUTPUT_LOW);
    transceiver::stxon::mode(Mode::OUTPUT_LOW);
    
    flash::cs::mode(Mode::OUTPUT_HIGH);
    flash::hold::mode(Mode::OUTPUT_HIGH);
    
    currentSense::enable::mode(Mode::OUTPUT_LOW);
    //currentSense sense pin remains disabled as it is an analog channel
    
    ledOn();
    delayMs(100);
    ledOff();
    
    //TODO: wait for 32KHz XTAL to startup and then enable 32KHz clock output
    
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
    
    //Serial port is causing some residual consumption
    USART0->CMD=USART_CMD_TXDIS | USART_CMD_RXDIS;
    USART0->ROUTE=0;
    debugConnector::tx::mode(Mode::DISABLED);
    debugConnector::rx::mode(Mode::DISABLED);
    
    //Sequence to enter EM4
    for(int i=0;i<5;i++)
    {
        EMU->CTRL=2<<2;
        EMU->CTRL=3<<2;
    }
    //Should never reach here
    miosix_private::IRQsystemReboot();
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

} //namespace miosix
