/***************************************************************************
 *   Copyright (C) 2017 by Terraneo Federico                               *
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

#include "rtc.h"
#include <miosix.h>
#include <sys/ioctl.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/kernel.h>

using namespace miosix;


namespace miosix {

//
// class Rtc
//

Rtc* Rtc::instance()
{
    static Rtc* singleton = nullptr;
    if ( singleton == nullptr) {
	singleton = new Rtc();
    }
    singleton->clock_freq = 32768; 
    return singleton;

}

unsigned short int Rtc::getSSR() 
{
 // //Function takes ~170 clock cycles ~60 cycles IRQgetTick, ~96 cycles tick2ns
    long long ssr;
    {
     FastInterruptDisableLock dlock;
     ssr = IRQgetSSR(dlock);
    }
}

unsigned short int Rtc::IRQgetSSR(FastInterruptDisableLock& dlock) 
{
 unsigned short int subseconds = 0x000 | (RTC->TSSSR & RTC_TSSSR_SS);
 return subseconds;
}

// Return day time as microseconds
unsigned long long int Rtc::IRQgetTime(FastInterruptDisableLock& dlock)
{
 unsigned short int hour_tens = 0x0000 | (RTC->TSTR & RTC_TSTR_HT);
 unsigned short int hour_units = 0x0000 | (RTC->TSTR & RTC_TSTR_HU);
 unsigned short int minute_tens = 0x0000 | (RTC->TSTR & RTC_TSTR_MNT);
 unsigned short int minute_units = 0x0000 | (RTC->TSTR & RTC_TSTR_MNU);
 unsigned short int second_tens = 0x0000 | (RTC->TSTR & RTC_TSTR_ST);
 unsigned short int second_units = 0x0000 | (RTC->TSTR & RTC_TSTR_SU);
 unsigned short int subseconds = 0x000 | (RTC->TSSSR & RTC_TSSSR_SS);
 unsigned long long int time_microsecs = ( hour_tens * 10 + hour_units) * 3600 * 1000000 \
					 + (minute_tens * 10 + minute_units) * 60 * 1000000 \
					 + ( second_tens * 10 + second_units ) * 1000000 \
					 + ( 256 - subseconds ) * 1000 / (clock_freq / 129) ;
 if ( (RTC->TSTR & RTC_TSTR_PM) == 0 ) 
 	return time_microsecs;
 else
	 return time_microsecs + 12 * 3600 * 1000000;
}

// \todo
unsigned long long int Rtc::IRQgetDate(FastInterruptDisableLock& dlock) 
{
	unsigned long long int date_secs;
 	// \todo
	return date_secs;
}

void Rtc::setWakeupInterrupt() 
{
	EXTI->IMR |= (1<<22);
	EXTI->RTSR |= (1<<22);
	EXTI->FTSR &= ~(1<<22);
	NVIC_SetPriority(RTC_WKUP_IRQn,10);
	NVIC_EnableIRQ(RTC_WKUP_IRQn);
}

Rtc::Rtc() 
{
	{
		FastInterruptDisableLock dLock;
		RCC->APB1ENR |= RCC_APB1ENR_PWREN;
		PWR->CR |= PWR_CR_DBP;
		RCC->BDCR=RCC_BDCR_RTCEN       //RTC enabled
			| RCC_BDCR_LSEON       //External 32KHz oscillator enabled
			| RCC_BDCR_RTCSEL_0;   //Select LSE as clock source for RTC
	}
	while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //Wait for LSE to start    

	{ 
		FastInterruptDisableLock dLock;
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;
	}
}

} //namespace miosix
