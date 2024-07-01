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

#include "kernel/kernel.h"
#include "interfaces/os_timer.h"
#include "interfaces/arch_registers.h"
#include "kernel/logging.h"

#ifdef WITH_RTC_AS_OS_TIMER

//NOTE: using the RTC as OS timer is currently only supported on STM32F1, the
//stm32f2/f4 RTC design makes this impossible. TODO: check stm32l4 RTC

namespace {

// class ScopedCnf, RAII to allow writing to the RTC alarm and prescaler.
// NOTE: datasheet says that after CNF bit has been cleared, no write is allowed
// to *any* of the RTC registers, not just PRL,CNT,ALR until RTOFF goes to 1.
// We do not wait until RTOFF is 1 in the destructor for performance reasons,
// so the rest of the code must be careful.
class ScopedCnf
{
public:
    ScopedCnf()
    {
        while((RTC->CRL & RTC_CRL_RTOFF)==0) ;
        RTC->CRL=0b11111;
    }

    ~ScopedCnf()
    {
        RTC->CRL=0b01111;
    }
};

}

namespace miosix {

//Setting quirkAdvance to 1 because datasheet specifies "The write operation
//only executes when the CNF bit is cleared; it takes at least three RTCCLK
//cycles to complete". Considering prescaler is 2, it takes "at least" 1.5
//ticks to write to the match register.
//This three RTCCLK tick wait till RTOFF is set increases the minimum time
//between two context switches to an unreasonable 91us, but sadly there's no way
//around bad hardware design
class STM32F1RTC_Timer : public TimerAdapter<STM32F1RTC_Timer, 32, 1>
{
public:
    static inline unsigned int IRQgetTimerCounter()
    {
        unsigned int h1=RTC->CNTH;
        unsigned int l1=RTC->CNTL;
        unsigned int h2=RTC->CNTH;
        if(h1==h2) return (h1<<16) | l1;
        return (h2<<16) | RTC->CNTL;
    }
    static inline void IRQsetTimerCounter(unsigned int v)
    {
        ScopedCnf cnf;
        RTC->CNTL=v;
        RTC->CNTH=v>>16;
    }

    static inline unsigned int IRQgetTimerMatchReg()
    {
        return RTC->ALRH<<16 | RTC->ALRL;
    }
    static inline void IRQsetTimerMatchReg(unsigned int v)
    {
        ScopedCnf cnf;
        RTC->ALRL=v;
        RTC->ALRH=v>>16;
    }

    static inline bool IRQgetOverflowFlag() { return RTC->CRL & RTC_CRL_OWF; }
    static inline void IRQclearOverflowFlag()
    {
        RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_OWF;
    }

    static inline bool IRQgetMatchFlag() { return RTC->CRL & RTC_CRL_ALRF; }
    static inline void IRQclearMatchFlag()
    {
        RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_ALRF;
    }

    static inline void IRQforcePendingIrq() { NVIC_SetPendingIRQ(RTC_IRQn); }

    static inline void IRQstopTimer()
    {
        //Unsupported
    }
    static inline void IRQstartTimer()
    {
        //Unsupported
    }

    static unsigned int IRQTimerFrequency() { return 16384; }

    static void IRQinitTimer()
    {
        {
            FastInterruptDisableLock dLock;
            RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
            PWR->CR |= PWR_CR_DBP;
            RCC->BDCR=RCC_BDCR_RTCEN       //RTC enabled
                    | RCC_BDCR_LSEON       //External 32KHz oscillator enabled
                    | RCC_BDCR_RTCSEL_0;   //Select LSE as clock source for RTC
            RCC_SYNC();
            #ifdef RTC_CLKOUT_ENABLE
            BKP->RTCCR=BKP_RTCCR_CCO;      //Output RTC clock/64 on pin
            #endif
        }
        while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //Wait for LSE to start

        //Interrupt on overflow or match
        RTC->CRH=RTC_CRH_OWIE | RTC_CRH_ALRIE;

        //Initialize prescaler, conter and match register
        {
            ScopedCnf cnf;
            RTC->PRLH=0; RTC->PRLL=1; //Minimum allowed prescaler
            RTC->CNTH=0; RTC->CNTL=0;
            RTC->ALRH=0xffff; RTC->ALRL=0xffff;
        }
        //High priority for RTC (Max=0, min=15)
        NVIC_SetPriority(RTC_IRQn,3);
        NVIC_EnableIRQ(RTC_IRQn);
    }
};

static STM32F1RTC_Timer timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);
} //namespace miosix

void __attribute__((naked)) RTC_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((used)) osTimerImpl()
{
    miosix::timer.IRQhandler();
}

#endif //WITH_RTC_AS_OS_TIMER
