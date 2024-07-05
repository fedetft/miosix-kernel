/***************************************************************************
 *   Copyright (C) 2024 by Terraneo Federico                               *
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

#include "interfaces/deep_sleep.h"
#include "interfaces/arch_registers.h"
#include "config/miosix_settings.h"

#ifdef WITH_DEEP_SLEEP

#ifndef WITH_RTC_AS_OS_TIMER
#error For stm32f1 target, WITH_DEEP_SLEEP requires WITH_RTC_AS_OS_TIMER
#endif //WITH_RTC_AS_OS_TIMER

using namespace std;

namespace miosix {

void IRQdeepSleepInit()
{
    EXTI->RTSR |= EXTI_RTSR_TR17;
    EXTI->IMR  |= EXTI_IMR_MR17; //enable wakeup interrupt
}
  
bool IRQdeepSleep(long long int abstime)
{
    /*
     * NOTE: The simplest way to support deep sleep is to use the RTC as the
     * OS timer. By doing so, there is no need to set the RTC wakeup time
     * whenever we enter deep sleep, as the scheduler already does, and there
     * is also no need to resynchronize the OS timer to the RTC because the
     * OS timer doesn't stop counting while we are in deep sleep.
     * The only disadvantage on this platform is that the OS timer resolution is
     * rather coarse, 1/16384Hz is ~61us, but this is good enough for many use
     * cases.
     *
     * This is why this code ignores the wakeup absolute time parameter, and the
     * only task is to write the proper sequence to enter the deep sleep state
     * instead of the sleep state.
     */
    return IRQdeepSleep();
}

bool IRQdeepSleep()
{
    #ifdef RUN_WITH_HSI
    /*
     * NOTE: The RTC causes two separate IRQs, the RTC IRQ, and the RTC_Alarm
     * IRQ. This second interrupt is only caused by the alarm condition, not by
     * e.g. a counter overflow, and is the only interrupt that can wake the MCU
     * from deep sleep (stop mode). The RTC interrupt doesn't.
     * We don't need an IRQ handler for the RTC_Alarm, as all interrupt code is
     * already handled by the RTC IRQ, but we need to enable it here just to
     * wake from deep sleep. Since when the RTC IRQ fires, the RTC_Alarm becomes
     * pending, we almost certain arrive here with the RTC_Alarm already pending
     * which would prevent deep sleep. Clearing this pending IRQ does not cause
     * a race condition if the RTC event already happened, as in that case also
     * the RTC IRQ is pending, and while this IRQ can't wake us from deep sleep,
     * it does prevent us from entring deep sleep.
     */
    EXTI->PR = EXTI_PR_PR17;
    NVIC_ClearPendingIRQ(RTC_Alarm_IRQn);
    NVIC_EnableIRQ(RTC_Alarm_IRQn);

    //Ensure the RTC alarm register has been written
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ;

    PWR->CR |= PWR_CR_LPDS;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    PWR->CR &= ~PWR_CR_LPDS;

    NVIC_DisableIRQ(RTC_Alarm_IRQn);

    //Wait RSF
    RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_RSF;
    while((RTC->CRL & RTC_CRL_RSF)==0) ;
    return true;

    #else //RUN_WITH_HSI
    #error TODO not yet implemented
    #endif //RUN_WITH_HSI
}

} //namespace miosix

#endif //WITH_DEEP_SLEEP
