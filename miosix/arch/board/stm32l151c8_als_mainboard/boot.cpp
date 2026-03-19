/***************************************************************************
 *   Copyright (C) 2012 by Federico Terraneo                               *
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
#include "interfaces/delays.h"
#include "interfaces/gpio.h"
#include "board_settings.h"
#include "drivers/mpu/cortexMx_mpu.h"

//We want to run the ALS_MAINBOARD at a lower clock to save power
uint32_t SystemCoreClock=miosix::cpuFrequency;

namespace miosix {

void IRQmemoryAndClockInit()
{
    static_assert(oscillatorType==OscillatorType::HSI,
                  "Unsupported oscillator type");
    static_assert(cpuFrequency==16000000,"Unsupported sysclk setting");
    FLASH->ACR |= FLASH_ACR_ACC64;
    FLASH->ACR |= FLASH_ACR_PRFTEN;
    
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    RCC_SYNC();
  
    /* Select the Voltage Range 1 (1.8 V) */
    PWR->CR = PWR_CR_VOS_0;
  
    /* Wait Until the Voltage Regulator is ready */
    while((PWR->CSR & PWR_CSR_VOSF) != RESET)
    {
    }
    
    //For low power reasons, this board runs off of the HSI 16MHz oscillator
    RCC->CR |= RCC_CR_HSION;
    //We should wait at least 6us for the HSI to stabilize. Therefore we wait
    //8us. However, the current clock is 2MHz (MSI) instead of 16, so ...
    delayUs(8*2/16);
    RCC->CFGR = 0x00000001; //Select HSI
    while((RCC->CFGR & 0xc)!=0x4) ;
    RCC->CR &= ~RCC_CR_MSION;
    /*!< Disable all interrupts */
    RCC->CIR = 0x00000000;
    
    // Architecture has MPU, enable kernel-level W^X protection
    IRQconfigureMPU();
}

} // namespace miosix
