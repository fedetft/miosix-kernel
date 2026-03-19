/***************************************************************************
 *   Copyright (C) 2014 by Terraneo Federico                               *
 *   Copyright (C) 2017 by Federico Amedeo Izzo                            *
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
#include <cmath>
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
#include "drivers/serial/serial.h"
#include "drivers/sdmmc/sd_stm32f2_f4_f7.h"
#include "board_settings.h"
// #include "kernel/IRQDisplayPrint.h"

namespace miosix {

//
// Initialization
//

template <int div, int t_rcd_ns, int t_rp_ns, int t_wr_ns,
    int t_rc_ns, int t_ras_ns, int t_xsr_ns, int t_mrd_ns>
static constexpr unsigned long sdramSDTR() noexcept
{
    constexpr float hclk_mhz = cpuFrequency/1000000;
    constexpr float t_ns = 1000.0f / (hclk_mhz / (float)div);
    constexpr int sdtr_trcd = std::ceil((float)t_rcd_ns / t_ns) - 1;
    static_assert(0 <= sdtr_trcd && sdtr_trcd <= 15);
    constexpr int sdtr_trp  = std::ceil((float)t_rp_ns  / t_ns) - 1;
    static_assert(0 <= sdtr_trp && sdtr_trp <= 15);
    constexpr int sdtr_twr  = std::ceil((float)t_wr_ns  / t_ns) - 1;
    static_assert(0 <= sdtr_twr && sdtr_twr <= 15);
    constexpr int sdtr_trc  = std::ceil((float)t_rc_ns  / t_ns) - 1;
    static_assert(0 <= sdtr_trc && sdtr_trc <= 15);
    constexpr int sdtr_tras = std::ceil((float)t_ras_ns / t_ns) - 1;
    static_assert(0 <= sdtr_tras && sdtr_tras <= 15);
    constexpr int sdtr_txsr = std::ceil((float)t_xsr_ns / t_ns) - 1;
    static_assert(0 <= sdtr_txsr && sdtr_txsr <= 15);
    constexpr int sdtr_tmrd = std::ceil((float)t_mrd_ns / t_ns) - 1;
    static_assert(0 <= sdtr_tmrd && sdtr_tmrd <= 15);
    return sdtr_trcd << FMC_SDTR1_TRCD_Pos
        | sdtr_trp  << FMC_SDTR1_TRP_Pos  
        | sdtr_twr  << FMC_SDTR1_TWR_Pos  
        | sdtr_trc  << FMC_SDTR1_TRC_Pos  
        | sdtr_tras << FMC_SDTR1_TRAS_Pos 
        | sdtr_txsr << FMC_SDTR1_TXSR_Pos 
        | sdtr_tmrd << FMC_SDTR1_TMRD_Pos;
}

template <int div, int n_rows, int t_refresh_ms>
constexpr unsigned long sdramSDRTR() noexcept
{
    constexpr float hclk_mhz = cpuFrequency/1000000;
    constexpr float t_us = 1.0f / (hclk_mhz / (float)div);
    constexpr float t_refresh_us = (float)t_refresh_ms * 1000.0f;
    constexpr float t_refreshPerRow_us = t_refresh_us / (float)n_rows;
    constexpr int sdrtr_count = (std::floor(t_refreshPerRow_us / t_us)-20)-1;
    static_assert(41 <= sdrtr_count && sdrtr_count <= 0x1FFF);
    return sdrtr_count << FMC_SDRTR_COUNT_Pos;
}

/**
 * The example code from ST checks for the busy flag after each command.
 * Interestingly I couldn't find any mention of this in the datasheet.
 */
static void sdramCommandWait()
{
    for(int i=0;i<0xffff;i++)
        if((FMC_Bank5_6->SDSR & FMC_SDSR_BUSY)==0) return;
}

void configureSdram()
{
    /*   SDRAM pins assignment
    PC0 -> FMC_SDNWE
    PD0  -> FMC_D2   | PE0  -> FMC_NBL0 | PF0  -> FMC_A0 | PG0 -> FMC_A10
    PD1  -> FMC_D3   | PE1  -> FMC_NBL1 | PF1  -> FMC_A1 | PG1 -> FMC_A11
    PD8  -> FMC_D13  | PE7  -> FMC_D4   | PF2  -> FMC_A2 | PG4 -> FMC_BA0
    PD9  -> FMC_D14  | PE8  -> FMC_D5   | PF3  -> FMC_A3 | PG5 -> FMC_BA1
    PD10 -> FMC_D15  | PE9  -> FMC_D6   | PF4  -> FMC_A4 | PG8 -> FC_SDCLK
    PD14 -> FMC_D0   | PE10 -> FMC_D7   | PF5  -> FMC_A5 | PG15 -> FMC_NCAS
    PD15 -> FMC_D1   | PE11 -> FMC_D8   | PF11 -> FC_NRAS
                     | PE12 -> FMC_D9   | PF12 -> FMC_A6
                     | PE13 -> FMC_D10  | PF13 -> FMC_A7
                     | PE14 -> FMC_D11  | PF14 -> FMC_A8
                     | PE15 -> FMC_D12  | PF15 -> FMC_A9
    PH2 -> FMC_SDCKE0| PI4 -> FMC_NBL2  |
    PH3 -> FMC_SDNE0 | PI5 -> FMC_NBL3  |

    // 32-bits Mode: D31-D16
    PH8 -> FMC_D16   | PI0 -> FMC_D24
    PH9 -> FMC_D17   | PI1 -> FMC_D25
    PH10 -> FMC_D18  | PI2 -> FMC_D26
    PH11 -> FMC_D19  | PI3 -> FMC_D27
    PH12 -> FMC_D20  | PI6 -> FMC_D28
    PH13 -> FMC_D21  | PI7 -> FMC_D29
    PH14 -> FMC_D22  | PI9 -> FMC_D30
    PH15 -> FMC_D23  | PI10 -> FMC_D31 */

    //Enable all gpios, passing clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN |
                    RCC_AHB1ENR_GPIOIEN | RCC_AHB1ENR_GPIOJEN |
                    RCC_AHB1ENR_GPIOKEN;
    RCC_SYNC();

    //First, configure SDRAM GPIOs, memory controller uses AF12
    GPIOC->AFR[0]=0x0000000c;
    GPIOD->AFR[0]=0x000000cc;
    GPIOD->AFR[1]=0xcc000ccc;
    GPIOE->AFR[0]=0xc00000cc;
    GPIOE->AFR[1]=0xcccccccc;
    GPIOF->AFR[0]=0x00cccccc;
    GPIOF->AFR[1]=0xccccc000;
    GPIOG->AFR[0]=0x00cc00cc;
    GPIOG->AFR[1]=0xc000000c;
    GPIOH->AFR[0]=0x0000cc00;
    GPIOH->AFR[1]=0xcccccccc;
    GPIOI->AFR[0]=0xcccccccc;
    GPIOI->AFR[1]=0x00000cc0;

    GPIOC->MODER=0x00000002;
    GPIOD->MODER=0xa02a000a;
    GPIOE->MODER=0xaaaa800a;
    GPIOF->MODER=0xaa800aaa;
    GPIOG->MODER=0x80020a0a;
    GPIOH->MODER=0xaaaa00a0;
    GPIOI->MODER=0x0028aaaa;

    /* GPIO speed register
    00: Low speed
    01: Medium speed
    10: High speed (50MHz)
    11: Very high speed (100MHz) */

    //Default to 50MHz speed for all GPIOs...
    GPIOA->OSPEEDR=0xaaaaaaaa;
    GPIOB->OSPEEDR=0xaaaaaaaa;
    GPIOC->OSPEEDR=0xaaaaaaaa | 0x00000003; //...but 100MHz for the SDRAM pins
    GPIOD->OSPEEDR=0xaaaaaaaa | 0xf03f000f;
    GPIOE->OSPEEDR=0xaaaaaaaa | 0xffffc00f;
    GPIOF->OSPEEDR=0xaaaaaaaa | 0xffc00fff;
    GPIOG->OSPEEDR=0xaaaaaaaa | 0xc0030f0f;
    GPIOH->OSPEEDR=0xaaaaaaaa | 0xffff00f0;
    GPIOI->OSPEEDR=0xaaaaaaaa | 0x003cffff;
    GPIOJ->OSPEEDR=0xaaaaaaaa;
    GPIOK->OSPEEDR=0xaaaaaaaa;

    //Second, actually start the SDRAM controller
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
    RCC_SYNC();

    //SDRAM is a MT48LC4M32B2B5 -6A speed grade, connected to bank 1 (0xc0000000)
    FMC_Bank5_6->SDCR[0]=FMC_SDCR1_SDCLK_1// SDRAM runs @ half CPU frequency
                       | FMC_SDCR1_RBURST // Enable read burst
                       | 0                //  0 delay between reads after CAS
                       | 0                //  8 bit column address
                       | FMC_SDCR1_NR_0   // 12 bit row address
                       | FMC_SDCR1_MWID_1 // 32 bit data bus
                       | FMC_SDCR1_NB     //  4 banks
                       | FMC_SDCR1_CAS_0  //  3 cycle CAS latency
                       | FMC_SDCR1_CAS_1;

    FMC_Bank5_6->SDTR[0]=sdramSDTR<2,18,18,20,60,42,67,20>();

    FMC_Bank5_6->SDCMR=  FMC_SDCMR_CTB1   // Enable bank 1
                       | 1;               // MODE=001 clock enabled
    sdramCommandWait();

    //ST and SDRAM datasheet agree a 100us delay is required here.
    delayUs(100);

    FMC_Bank5_6->SDCMR=  FMC_SDCMR_CTB1   // Enable bank 1
                       | 2;               // MODE=010 precharge all command
    sdramCommandWait();

    FMC_Bank5_6->SDCMR=  (8-1)<<5         // NRFS=8 SDRAM datasheet says
                                          // "at least two AUTO REFRESH cycles"
                       | FMC_SDCMR_CTB1   // Enable bank 1
                       | 3;               // MODE=011 auto refresh
    sdramCommandWait();

    FMC_Bank5_6->SDCMR=0x230<<9           // MRD=0x230:CAS latency=3 burst len=1
                       | FMC_SDCMR_CTB1   // Enable bank 1
                       | 4;               // MODE=100 load mode register
    sdramCommandWait();

    FMC_Bank5_6->SDRTR=sdramSDRTR<2,4096,64>();
}

// static IRQDisplayPrint *irq_display;
void IRQbspInit()
{
    //If using SDRAM GPIOs are enabled by configureSdram(), else enable them here
    #ifndef __ENABLE_XRAM
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN |
                    RCC_AHB1ENR_GPIOIEN | RCC_AHB1ENR_GPIOJEN |
                    RCC_AHB1ENR_GPIOKEN;
    RCC_SYNC();
    #endif //__ENABLE_XRAM

    _led::mode(Mode::OUTPUT);
    ledOn();
    delayMs(100);
    ledOff();
    IRQsetDefaultConsole(
        STM32SerialBase::get<defaultSerialTxPin,defaultSerialRxPin,
        defaultSerialRtsPin,defaultSerialCtsPin>(
            defaultSerial,defaultSerialSpeed,
            defaultSerialFlowctrl,defaultSerialDma));
//     irq_display = new IRQDisplayPrint();
//     IRQsetDefaultConsole(intrusive_ref_ptr<Device>(irq_display));
}

// void* printIRQ(void *argv)
// {
// 	intrusive_ref_ptr<IRQDisplayPrint> irqq(irq_display);
// 	irqq.get()->printIRQ();
// 	return NULL;
// }

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    basicFilesystemSetup(SDIODriver::instance());
    #endif //WITH_FILESYSTEM
//     Thread::create(printIRQ, 2048, DEFAULT_PRIORITY, nullptr, Thread::DETACHED);
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
    for(;;) ;
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
