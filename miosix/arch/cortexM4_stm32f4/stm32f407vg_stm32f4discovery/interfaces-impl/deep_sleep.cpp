/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014 by Terraneo Federico                   *
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
#include "drivers/rtc.h"

namespace miosix {

// enabling programmable wakeup timer for RTC
// as described in the refernence manual of
// STM32F407VG (the EXTI lines should be already
// configured)
void IRQdeepSleep(long long int abstime)
{
    unsigned long long int value = (abstime % 1000000); // part less than 1 second  

    RTC->CR &= ~RTC_CR_WUTE;
    while( (RTC->ISR & RTC_ISR_WUTWF ) == 0 );
    RTC->CR |= (RTC_CR_WUCKSEL_0 | RTC_CR_WUCKSEL_1);
    RTC->CR &= ~(RTC_CR_WUCKSEL_2) ; // select RTC/2 clock for wakeup
    RTC->WUTR = value & RTC_WUTR_WUT; 
    RTC->CR |=  RTC_CR_WUTE;

    __WFE();
}

// setup of the RTC registers
// and configure it to allow RTC Wakeup IRQ
void IRQdeepSleepInit()
{
    Rtc* rtc = Rtc::instance();
    rtc->setWakeupInterrupt();
}

} //namespace miosix
