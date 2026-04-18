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

#include "board_settings.h"
#include "interfaces/arch_registers.h"

extern "C" void SystemInit();

namespace miosix {

void IRQsetupClockTree()
{
    static_assert(hseFrequency==8000000,"Unsupported HSE oscillator frequency");
    static_assert(cpuFrequency==32000000,"Unsupported target SYSCLK");

    // Select MSI as system clock, which is already the default after reset.
    // Unless it is reconfigured, it runs at 2MHz.
    RCC->CFGR = (RCC->CFGR & (uint32_t)~RCC_CFGR_SW) | RCC_CFGR_SW_MSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_MSI) {}

    // Disable PLL
    RCC->CR &= (uint32_t)~RCC_CR_PLLON;
    while(RCC->CR & RCC_CR_PLLRDY) {}

    // Switch to voltage range 1
    // On voltage ranges 2 and 3 you can't change system clock frequency in
    // larger steps than 4x!
    PWR->CR = (PWR->CR & ~PWR_CR_VOS_Msk) | (1 << PWR_CR_VOS_Pos);
    while(PWR->CSR & PWR_CSR_VOSF) {}

    // Set flash latency to 1 wait state
    FLASH->ACR |= FLASH_ACR_LATENCY;
    while((FLASH->ACR & FLASH_ACR_LATENCY) == 0) ;

    // Enable HSE
    if(oscillatorType==OscillatorType::HSEBYP) RCC->CR |= RCC_CR_HSEBYP;
    RCC->CR |= RCC_CR_HSEON;
    while((RCC->CR & RCC_CR_HSERDY) == 0) ;

    // Set PLL multiplier to 12 (HSI is 8MHz, gives 96MHz) and divider by 3
    // 96MHz is needed for the 48MHz output to work correctly (it is hardcoded
    // to divide by 2)
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PLLMUL | RCC_CFGR_PLLDIV))
                | (RCC_CFGR_PLLMUL12 | RCC_CFGR_PLLDIV3);
    // Set PLL source
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PLLSRC)) | RCC_CFGR_PLLSRC_HSE;
    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {}

    // Set PLL as system clock
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
}

void IRQmemoryAndClockInit()
{
    // For L0 microcontrollers SystemInit is basically empty,
    // but we call it anyway for good measure.
    SystemInit();
    IRQsetupClockTree();
}

} // namespace miosix
