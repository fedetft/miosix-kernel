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
    static_assert(oscillatorType==OscillatorType::HSI,"Unsupported clock config");
    static_assert(sysclkFrequency==32000000,"Unsupported clock config");
    RCC->CR |= RCC_CR_HSION;
    while((RCC->CR & RCC_CR_HSIRDY)==0) ;
    RCC->CR &= ~RCC_CR_PLLON;
    while(RCC->CR & RCC_CR_PLLRDY) ;
    RCC->CFGR &= ~RCC_CFGR_SW; //Selects HSI
    RCC->CFGR = RCC_CFGR_PLLMUL4 //4*8=32MHz
              | RCC_CFGR_PLLSRC_HSI_PREDIV;
    RCC->CR |= RCC_CR_PLLON;
    while((RCC->CR & RCC_CR_PLLRDY)==0) ;
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= 1; //1 wait state for freq > 24MHz
    RCC->CFGR |= RCC_CFGR_SW_PLL;
}

void IRQmemoryAndClockInit()
{
    // For F0 microcontrollers SystemInit is basically empty,
    // but we call it anyway for good measure.
    SystemInit();
    IRQsetupClockTree();
}

} // namespace miosix
