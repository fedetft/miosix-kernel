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

#include "interfaces/arch_registers.h"
#include "interfaces/gpio.h"
#include "board_settings.h"
#include "drivers/mpu/cortexMx_mpu.h"

uint32_t SystemCoreClock=miosix::cpuFrequency;

namespace miosix {

/*
 * Restore clocks to default configuration. On L1 chips, this means the SYSCLK
 * originates from the MSI RC oscillator.
 */
void IRQresetClockTree()
{
    // Ensure MSI is turned on
    RCC->CR |= RCC_CR_MSION;
    // Select MSI oscillator and disable all prescalers (/1 for everything)
    RCC->CFGR &= ~(RCC_CFGR_SW
        | RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2
        | RCC_CFGR_MCOSEL | RCC_CFGR_MCOPRE);
    // Reset PLL configuration
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL | RCC_CFGR_PLLDIV);
    // Disable high speed internal (HSI) and external (HSE) oscillator,
    // clock security system (CSS) and PLL
    RCC->CR &= ~(RCC_CR_HSION | RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);
    while(RCC->CR & RCC_CR_PLLRDY) {}
    // Now that HSE is disabled we can also disable HSE bypass safely
    RCC->CR &= ~RCC_CR_HSEBYP;
    // Disable all clock interrupts
    RCC->CIR = 0;
}

void IRQsetupClockTree()
{
    static_assert(hseFrequency==8000000,"Unsupported HSE frequency");
    static_assert(oscillatorType==OscillatorType::HSE
                  || oscillatorType==OscillatorType::HSEBYP,
                  "Unsupported oscillator type");

    // Reset clock tree to initial state
    IRQresetClockTree();

    // Enable HSE
    unsigned long bypass=0;
    if(oscillatorType==OscillatorType::HSEBYP) bypass=RCC_CR_HSEBYP;
    RCC->CR |= RCC_CR_HSEON | bypass;
    while((RCC->CR & RCC_CR_HSERDY) == 0) {}

    // Configure flash: 64 bit access, prefetch, 1 wait state
    FLASH->ACR |= FLASH_ACR_ACC64;
    FLASH->ACR |= FLASH_ACR_PRFTEN;
    FLASH->ACR |= FLASH_ACR_LATENCY;
    // Configure regulator to 1.8V
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    RCC_SYNC();
    PWR->CR = PWR_CR_VOS_0;
    while((PWR->CSR & PWR_CSR_VOSF) != 0) ;

    unsigned long mul,div;
    static_assert(cpuFrequency==32000000
                ||cpuFrequency==24000000, "unsupported sysclk");
    if(cpuFrequency==32000000)
    {
        mul=RCC_CFGR_PLLMUL12;
        div=RCC_CFGR_PLLDIV3;
    } else if(cpuFrequency==24000000) {
        mul=RCC_CFGR_PLLMUL12;
        div=RCC_CFGR_PLLDIV4;
    }
    // Configure PLL
    RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE | mul | div);
    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) ;

    // Set PLL as system clock
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) ;
}

void IRQmemoryAndClockInit()
{
    IRQsetupClockTree();
    
    // Architecture has MPU, enable kernel-level W^X protection
    IRQconfigureMPU();
}

} // namespace miosix
