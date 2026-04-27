/***************************************************************************
 *   Copyright (C) 2023-2026 by Alberto Nidasio, Davide Mor, Raul Radu     *
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
#include "interfaces_private/bsp_private.h"

#include <inttypes.h>
#include <sys/ioctl.h>

#include <cstdlib>

#include "board_settings.h"
#include "miosix_settings.h"
#include "drivers/sdmmc/stm32f2_f4_f7_sd.h"
#include "drivers/serial/stm32f7_serial.h"
#include "filesystem/console/console_device.h"
#include "filesystem/file_access.h"
#include "interfaces/arch_registers.h"
#include "interfaces/delays.h"
#include "interfaces/poweroff.h"
#include "kernel/thread.h"
#include "kernel/logging.h"
#include "kernel/sync.h"

namespace miosix
{

//
// Initialization
//

static void sdramCommandWait()
{
    for (int i = 0; i < 0xffff; i++)
        if ((FMC_Bank5_6->SDSR & FMC_SDSR_BUSY) == 0)
            return;
}

void configureSdram()
{
    // Enable gpios used by the ram
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
                    RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN |
                    RCC_AHB1ENR_GPIOFEN | RCC_AHB1ENR_GPIOGEN;
    RCC_SYNC();

    // On the compute unit with F767ZI, the SDRAM pins are:
    // - PG8:  FMC_SDCLK  (sdram clock)
    // - PB5:  FMC_SDCKE1 (sdram bank 2 clock enable)
    // - PB6:  FMC_SDNE1  (sdram bank 2 chip enable)
    // - PF0:  FMC_A0
    // - PF1:  FMC_A1
    // - PF2:  FMC_A2
    // - PF3:  FMC_A3
    // - PF4:  FMC_A4
    // - PF5:  FMC_A5
    // - PF12: FMC_A6
    // - PF13: FMC_A7
    // - PF14: FMC_A8
    // - PF15: FMC_A9
    // - PG0:  FMC_A10
    // - PG1:  FMC_A11
    // - PG2:  FMC_A12    (used only by the 32MB ram, not by the 8MB one)
    // - PD14: FMC_D0
    // - PD15: FMC_D1
    // - PD0:  FMC_D2
    // - PD1:  FMC_D3
    // - PE7:  FMC_D4
    // - PE8:  FMC_D5
    // - PE9:  FMC_D6
    // - PE10: FMC_D7
    // - PE11: FMC_D8
    // - PE12: FMC_D9
    // - PE13: FMC_D10
    // - PE14: FMC_D11
    // - PE15: FMC_D12
    // - PD8:  FMC_D13
    // - PD9:  FMC_D14
    // - PD10: FMC_D15

    // - PG4:  FMC_BA0
    // - PG5:  FMC_BA1
    // - PF11: FMC_SDNRAS
    // - PG15: FMC_SDNCAS
    // - PC0:  FMC_SDNWE
    // - PE0:  FMC_NBL0
    // - PE1:  FMC_NBL1

    // All SDRAM GPIOs needs to be configured with alternate function 12 and
    // maximum speed

    // WARNING: The current configuration is for the 8MB ram

    // Alternate functions
    GPIOB->AFR[0] = 0x0cc00000;
    GPIOC->AFR[0] = 0x0000000c;
    GPIOD->AFR[0] = 0x000000cc;
    GPIOD->AFR[1] = 0xcc000ccc;
    GPIOE->AFR[0] = 0xc00000cc;
    GPIOE->AFR[1] = 0xcccccccc;
    GPIOF->AFR[0] = 0x00cccccc;
    GPIOF->AFR[1] = 0xccccc000;
    GPIOG->AFR[0] = 0x00cc0ccc;
    GPIOG->AFR[1] = 0xc000000c;

    // Mode
    GPIOB->MODER = 0x00002800;
    GPIOC->MODER = 0x00000002;
    GPIOD->MODER = 0xa02a000a;
    GPIOE->MODER = 0xaaaa800a;
    GPIOF->MODER = 0xaa800aaa;
    GPIOG->MODER = 0x80020a2a;

    // Speed (high speed for all, very high speed for SDRAM pins)
    GPIOB->OSPEEDR = 0x00003c00;
    GPIOC->OSPEEDR = 0x00000003;
    GPIOD->OSPEEDR = 0xf03f000f;
    GPIOE->OSPEEDR = 0xffffc00f;
    GPIOF->OSPEEDR = 0xffc00fff;
    GPIOG->OSPEEDR = 0xc0030f3f;

    // Since we'we un-configured PB3 and PB4 (by default they are SWO and NJRST)
    // finish the job and remove the default pull-up
    GPIOB->PUPDR = 0;

    // Enable the SDRAM controller clock
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
    RCC_SYNC();

    // The SDRAM is a AS4C16M16SA-6TIN
    // 16Mx16bit = 256Mb = 32MB
    // HCLK = 216MHz -> SDRAM clock = HCLK/2 = 108MHz

    // 1. Memory device features
    FMC_Bank5_6->SDCR[0] = 0                     // 0 delay after CAS latency
                           | FMC_SDCR1_RBURST    // Enable read bursts
                           | FMC_SDCR1_SDCLK_1;  // SDCLK = HCLK / 2
    FMC_Bank5_6->SDCR[1] = 0                     // Write accesses allowed
                           | FMC_SDCR2_CAS_1     // 2 cycles CAS latency
                           | FMC_SDCR2_NB        // 4 internal banks
                           | FMC_SDCR2_MWID_0    // 16 bit data bus
                           | FMC_SDCR2_NR_1      // 13 bit row address
                           | FMC_SDCR2_NC_0;     // 9 bit column address

    // 2. Memory device timings
    static_assert(cpuFrequency==216000000, "No SDRAM timings for this clock frequency");
    // SDRAM timings. One clock cycle is 9.26ns
    FMC_Bank5_6->SDTR[0] =
        (2 - 1) << FMC_SDTR1_TRP_Pos     // 2 cycles TRP  (18.52ns > 18ns)
        | (7 - 1) << FMC_SDTR1_TRC_Pos;  // 7 cycles TRC  (64.82ns > 60ns)
    FMC_Bank5_6->SDTR[1] =
        (2 - 1) << FMC_SDTR1_TRCD_Pos     // 2 cycles TRCD (18.52ns > 18ns)
        | (2 - 1) << FMC_SDTR1_TWR_Pos    // 2 cycles TWR  (min 2cc > 12ns)
        | (5 - 1) << FMC_SDTR1_TRAS_Pos   // 5 cycles TRAS (46.3ns  > 42ns)
        | (7 - 1) << FMC_SDTR1_TXSR_Pos   // 7 cycles TXSR (74.08ns > 61.5ns)
        | (2 - 1) << FMC_SDTR1_TMRD_Pos;  // 2 cycles TMRD (min 2cc > 12ns)

    // 3. Enable the bank 2 clock
    FMC_Bank5_6->SDCMR =
        0b001 << FMC_SDCMR_MODE_Pos  // Clock Configuration Enable
        | FMC_SDCMR_CTB2;            // Bank 2
    sdramCommandWait();

    // 4. Wait during command execution
    delayUs(100);

    // 5. Issue a "Precharge All" command
    FMC_Bank5_6->SDCMR = 0b010 << FMC_SDCMR_MODE_Pos  // Precharge all
                         | FMC_SDCMR_CTB2;            // Bank 2
    sdramCommandWait();

    // 6. Issue Auto-Refresh commands
    FMC_Bank5_6->SDCMR = 0b011 << FMC_SDCMR_MODE_Pos       // Auto-Refresh
                         | FMC_SDCMR_CTB2                  // Bank 2
                         | (8 - 1) << FMC_SDCMR_NRFS_Pos;  // 8 Auto-Refresh
    sdramCommandWait();

    // 7. Issue a Load Mode Register command
    FMC_Bank5_6->SDCMR =
        0b100 << FMC_SDCMR_MODE_Pos          // Load mode register
        | FMC_SDCMR_CTB2                     // Bank 2
        | 0 << FMC_SDCMR_MRD_Pos             // Burst length = 1
        | (0b010 << 4) << FMC_SDCMR_MRD_Pos  // CAS = 2 clocks,
        | (1 << 9) << FMC_SDCMR_MRD_Pos;     // Single bit write burst mode
    sdramCommandWait();

    // 8. Program the refresh rate (4K / 32ms)
    // 64ms / 8192 = 7.8125us
    // 7.8125us * 133MHz = 1039 - 20 = 1019
    static_assert(cpuFrequency==216000000, "No SDRAM refresh timings for this clock frequency");
    FMC_Bank5_6->SDRTR = 1019 << FMC_SDRTR_COUNT_Pos;

}

void IRQbspInit()
{
    // Enable USART1 pins port
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    userLed1::mode(Mode::OUTPUT);
    userLed2::mode(Mode::OUTPUT);
    userLed3::mode(Mode::OUTPUT);
    userLed4::mode(Mode::OUTPUT);

    ledOn();
    delayMs(100);
    ledOff();
    
    IRQsetDefaultConsole(
        STM32SerialBase::get<defaultSerialTxPin,defaultSerialRxPin,
        defaultSerialRtsPin,defaultSerialCtsPin>(
            defaultSerial,defaultSerialSpeed,
            defaultSerialFlowctrl,defaultSerialDma));
}

void bspInit2()
{
#ifdef WITH_FILESYSTEM
    basicFilesystemSetup(SDIODriver::instance());
#endif  // WITH_FILESYSTEM
}

//
// Shutdown and reboot
//

void shutdown()
{
    ioctl(STDOUT_FILENO, IOCTL_SYNC, 0);

#ifdef WITH_FILESYSTEM
    ledOn();
    delayMs(100);
    ledOff();
    delayMs(500);
    ledOn();
    delayMs(500);
    ledOff();
    FilesystemManager::instance().umountAll();
#endif  // WITH_FILESYSTEM

    FastGlobalIrqLock::lock();
    for (;;)
        ;
}

void reboot()
{
    ioctl(STDOUT_FILENO, IOCTL_SYNC, 0);

#ifdef WITH_FILESYSTEM
    ledOn();
    delayMs(100);
    ledOff();
    delayMs(500);
    ledOn();
    delayMs(100);
    ledOff();

    FilesystemManager::instance().umountAll();
#endif  // WITH_FILESYSTEM

    FastGlobalIrqLock::lock();
    IRQsystemReboot();
}

}  // namespace miosix
