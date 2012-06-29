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

#include <stdio.h>
#include "interfaces/suspend_support.h"
#include "kernel/process_pool.h"
#include "miosix.h"
#include "interfaces/arch_registers.h"

#ifdef WITH_HIBERNATION

namespace miosix {
    
void IRQinitializeSuspendSupport()
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_DBP;
    PWR->CSR |= PWR_CSR_BRE;
    while((PWR->CSR & PWR_CSR_BRR)==0) ;
    RCC->AHB1ENR |= RCC_AHB1ENR_BKPSRAMEN;

    RCC->BDCR |= RCC_BDCR_LSEON;
    while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //wait
    RCC->BDCR |= RCC_BDCR_RTCSEL_0 | RCC_BDCR_RTCEN;
    RTC->WPR=0xca;
    RTC->WPR=0x53;
}

void doSuspend(unsigned int seconds)
{
    if(seconds==0) return;
    if(seconds>31) seconds=31;
  
    while(Console::txComplete()==false) ;

    {
        FastInterruptDisableLock dLock;
 
        RCC->CSR |= RCC_CSR_RMVF; //This makes firstBoot() return false next time
        RTC->CR &= ~RTC_CR_WUTE;
        while((RTC->ISR & RTC_ISR_WUTWF)==0) ; //Wait
        RTC->WUTR=seconds*2048; //10s
        RTC->CR |= RTC_CR_WUTE | RTC_CR_WUTIE;
    
        RTC->ISR= ~RTC_ISR_WUTF;
        EXTI->EMR |= 1<<22;
        EXTI->RTSR |= 1<<22;

        PWR->CR |= PWR_CR_PDDS | PWR_CR_CWUF;
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
        //Using WFE instead of WFI because if while we are with interrupts
        //disabled an interrupt (such as the tick interrupt) occurs, it
        //remains pending and the WFI becomes a nop, and the device never goes
        //in sleep mode. WFE events are latched in a separate pending register
        //so interrupts do not interfere with them
        __WFE();
        //Should never reach here
        NVIC_SystemReset();
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