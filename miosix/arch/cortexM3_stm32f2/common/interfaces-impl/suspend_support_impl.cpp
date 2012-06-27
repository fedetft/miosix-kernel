/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico  and Luigi Rucco  *
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

#include "interfaces/suspend_support.h"
#include "kernel/process_pool.h"
#include "miosix.h"
#include "interfaces/arch_registers.h"

#ifdef WITH_HIBERNATION

namespace miosix {
    
void IRQinitializeSuspendSupport()
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; //This is to to enable access to PWR...
    PWR-> CR |= PWR_CR_DBP; //This is to enable access to the BRE bit below...
    PWR-> CSR |= PWR_CSR_BRE; //This is to enable the SRAM in standby mode...
    while((PWR->CSR & PWR_CSR_BRR)==0) ; //Wait till the change becomes effective
    RCC->AHB1ENR |= RCC_AHB1ENR_BKPSRAMEN; //This is to enable SW access...
    
    PWR->CR |= PWR_CR_CWUF;
    
    RCC->BDCR |= RCC_BDCR_LSEON;
    while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //wait
    RCC->BDCR |= RCC_BDCR_RTCSEL_0 | RCC_BDCR_RTCEN;
    RTC->WPR=0xca;
    RTC->WPR=0x53;
    
    RTC->CR |= RTC_CR_WUTE | RTC_CR_WUTIE;
}

void doSuspend(unsigned int seconds)
{
    if(seconds==0) return;
    if(seconds>31) seconds=31;
    {
        InterruptDisableLock dLock;
        while(Console::IRQtxComplete()==false) ;

        RCC->CSR |= RCC_CSR_RMVF; //This makes firstBoot() return false next time
        
        EXTI->PR = 1<<22; //The EXTI_PR_PR22 constant does not exist??
        EXTI->IMR |= 1<<22; //The EXTI_IMR_MR22 constant does not exist??
        EXTI->RTSR |= 1<<22; //The EXTI_RTSR_TR22 constant does not exist??
        
        RTC->CR &= ~RTC_CR_WUTE;
        while((RTC->ISR & RTC_ISR_WUTWF)==0) ; //Wait
        RTC->WUTR=seconds*2048;
        RTC->CR |= RTC_CR_WUTE;
        RTC->ISR= ~RTC_ISR_WUTF;
       
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
        PWR->CR |= PWR_CR_PDDS;
        for(;;) __WFI();
    }
}

bool firstBoot()
{
    static const unsigned int firstBootMask=
        RCC_CSR_LPWRRSTF
      | RCC_CSR_WWDGRSTF
      | RCC_CSR_WDGRSTF
      | RCC_CSR_SFTRSTF
      | RCC_CSR_PORRSTF
      | RCC_CSR_PADRSTF
      | RCC_CSR_BORRSTF;
    return (RCC->CSR & firstBootMask) ? true : false;
}

int getAllocatorSramAreaSize()
{
    return ProcessPool::instance().getSerializableSize();
}

} //namespace miosix

#endif //WITH_HIBERNATION