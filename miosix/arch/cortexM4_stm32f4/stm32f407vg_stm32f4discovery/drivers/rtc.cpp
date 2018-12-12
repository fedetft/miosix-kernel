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

using namespace miosix;

namespace {

//
// class ScopedCnf, RAII to allow writing to the RTC alarm and prescaler.
// NOTE: datasheet says that after CNF bit has been cleared, no write is allowed
// to *any* of the RTC registers, not just PRL,CNT,ALR until RTOFF goes to 1.
// We do not wait until RTOFF is 1 in the destructor for performance reasons,
// so the rest of the code must be careful.
//
class ScopedCnf
{
public:
    ScopedCnf()
    {
        while((RTC->CR & RTC_CR_RTOFF)==0) ;
        RTC->CRL=0b11111;
    }
    
    ~ScopedCnf()
    {
        RTC->CR=0b01111;
    }
};

long long swTime=0;      //64bit software extension of current time in ticks
long long irqTime=0;     //64bit software extension of scheduled irq time in ticks
Thread *waiting=nullptr; //waiting thread

/**
 * RTC interrupt
 */
void __attribute__((naked)) RTC_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z10RTCIrqImplv");
    restoreContext();
}

/**
 * RTC interrupt actual implementation
 */
void __attribute__((used)) RTCIrqImpl()
{
    unsigned int crl=RTC->CRL;
    if(crl & RTC_CRL_OWF)
    {
        RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_OWF;
        swTime+=1LL<<32;
    } else if(crl & RTC_CRL_ALRF) {
        RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_ALRF;
        if(waiting && IRQgetTick()>=irqTime)
        {
            waiting->IRQwakeup();
            if(waiting->IRQgetPriority()>
                Thread::IRQgetCurrentThread()->IRQgetPriority())
                    Scheduler::IRQfindNextThread();
            waiting=nullptr;
        }
    }
}

namespace miosix {

void absoluteDeepSleep(long long int ns_value)
{
    FastInterrupDisableLock dlock;
    const long long wakeupAdvance=3; //waking up takes time
    
    Rtc& rtc=Rtc::instance();
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);


    unsigned long long int value = (ns_value % 1000000); // part less than 1 second  

    EXTI->RTSR |= (1<<22);
    EXTI->EMR  |= (1<<22); //enable event for wakeup

    RTC->CR &= ~RTC_CR_WUTE;
    while( (RTC->ISR & RTC_ISR_WUTWF ) == 0 );
    RTC->CR |= (RTC_CR_WUCKSEL_0 | RTC_CR_WUCKSEL_1);
    RTC->CR &= ~(RTC_CR_WUCKSEL_2) ; // select RTC/2 clock for wakeupt
    RTC->WUTR = value & RTC_WUTR_WUT; 
    RTC->CR |=  RTC_CR_WUTE;
    while((RTC->CR & RTC_CR_RTOFF)==0) ;
    

}
//
// class Rtc
//

Rtc& Rtc::instance()
{
    static Rtc singleton;
    return singleton;
}

long long Rtc::getSSR() const
{
    //Function takes ~170 clock cycles ~60 cycles IRQgetTick, ~96 cycles tick2ns
    long long tick;
    {
        FastInterruptDisableLock dLock;
        tick=IRQgetTick();
    }
    //tick2ns is reentrant, so can be called with interrupt enabled
    return tc.tick2ns(tick);
}

long long Rtc::IRQSgetSSR() const
{
    return tc.tick2ns(IRQgetTick());
}

void Rtc::setValue(long long value)
{
    FastInterruptDisableLock dLock;
    swTime=tc.ns2tick(value)-IRQgetHwTick();
}

void Rtc::wait(long long value)
{
    FastInterruptDisableLock dLock;
    IRQabsoluteWaitTick(IRQgetTick()+tc.ns2tick(value),dLock);
}

bool Rtc::absoluteWait(long long value)
{
    FastInterruptDisableLock dLock;
    return IRQabsoluteWaitTick(tc.ns2tick(value),dLock);
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
	FastInterruptDisableLock dlock;
	EXTI->IMR |= (1<<22);
	EXTI->RTSR |= (1<<22);
	EXTI->FTSR &= ~(1<<22);
	NVIC_SetPriority(RTC_WKUP_IRQn,5);
	NVIC_EnableIRQ(RTC_WKUP_IRQn);
    }
}

} //namespace miosix
