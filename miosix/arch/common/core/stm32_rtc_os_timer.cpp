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
#include "interfaces/deep_sleep.h"
#include "kernel/logging.h"

#ifdef WITH_RTC_AS_OS_TIMER

// Defined in system_stm32f1xx.c TODO replace with miosix-specific PLL code
extern "C" void SetSysClock();

// NOTE: using the RTC as OS timer is currently only supported on STM32F1, the
// stm32f2/f4 RTC design makes this impossible. TODO: check stm32l4 RTC

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

// Setting quirkAdvance to 1 because datasheet specifies "The write operation
// only executes when the CNF bit is cleared; it takes at least three RTCCLK
// cycles to complete". Considering prescaler is 2, it takes "at least" 1.5
// ticks to write to the match register.
class STM32F1RTC_Timer : public TimerAdapter<STM32F1RTC_Timer, 32, 1>
{
public:
    static inline unsigned int IRQgetTimerCounter()
    {
        // Workaround for hardware bug! The pending bit is asserted earlier than
        // the observable counter rollover, actually in the middle of the timer
        // counting 0xffffffff. This is actually documented behavior, for some
        // reason the overflow flag is asserted before the overflow occurs!
        // Solution: if reading 0xffffffff wait till next cycle as we can't
        // predict what the pending bit would be
        for(;;)
        {
            unsigned int h1=RTC->CNTH;
            unsigned int l1=RTC->CNTL;
            unsigned int h2=RTC->CNTH;
            auto result = h1==h2 ? (h1<<16) | l1 : (h2<<16) | RTC->CNTL;
            if(result==0xffffffff) continue;
            return result;
        }
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
        // If two context switches occur with a short delay in between, the 3
        // RTCCLK tick wait till RTOFF is set means we could be wasting here up
        // to an unreasonable 91us. This is what limits the number of context
        // switch/second when using the RTC as ostimer, an why you shouldn't use
        // this driver unless you really need it (e.g, using deep sleep),
        // and with already slow CPU speeds. At least, by optimistically not
        // waiting for RTOFF in the ScopedCnf destructor, this time penalty is
        // only incurred for two back-to-back context switches, not for all
        // context switches. We did what we could, but sadly there's no way
        // around bad hardware design.
        ScopedCnf cnf;
        RTC->ALRL=v;
        RTC->ALRH=v>>16;
    }

    static inline bool IRQgetOverflowFlag() { return RTC->CRL & RTC_CRL_OWF; }
    static inline void IRQclearOverflowFlag()
    {
        // Workaround for hardware bug! If the RTC overflow interrupt is delayed
        // by an interrupt disable lock. its interrupt flag can become "sticky"
        // and not clear for a certain amount of time. Of course, if this flag
        // isn't cleared, the interrupt remains pending and keeps occurring,
        // thus the upperTimeTick gets incrementd more than once, causing a
        // massive forward clock jump of ~72 hours per each extra increment!
        // To replicate the bug reliably, remove the for loop in
        // IRQgetTimerCounter(), uncomment the test() function and call it from
        // main. This was tested on an stm3210e-eval running @ 72MHz from flash:
        // Test failed fail=0x1ffffffff lastgood=0xffffffff
        // Test failed fail=0x2900000193 lastgood=0x1ffffffff
        // The 0x29 should have been 0x1, but the IRQ fired 41 times in a row.
        // The workaround is to keep clearing the flag till it's really cleared.
        // When the bug occurs, the do/while loop iterates ~209 times, tested by
        // adding a counter variable...
        do {
            RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_OWF;
        } while(RTC->CRL & RTC_CRL_OWF);
    }

    static inline bool IRQgetMatchFlag() { return RTC->CRL & RTC_CRL_ALRF; }
    static inline void IRQclearMatchFlag()
    {
        // Defensive programming. The do/while is added just in case the ALRF
        // flag suffers from the same "sticky" hardware bug as the OWF flag,
        // even though the bug was not observed yet.
        do {
            RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_ALRF;
        } while(RTC->CRL & RTC_CRL_ALRF);
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
            RCC_SYNC();
            PWR->CR |= PWR_CR_DBP;
            RCC->BDCR=RCC_BDCR_RTCEN       //RTC enabled
                    | RCC_BDCR_LSEON       //External 32KHz oscillator enabled
                    | RCC_BDCR_RTCSEL_0;   //Select LSE as clock source for RTC
            RCC_SYNC();
        }
        while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //Wait for LSE to start

        //Interrupt on overflow or match
        RTC->CRH=RTC_CRH_OWIE | RTC_CRH_ALRIE;

        //Initialize prescaler, counter and match register
        {
            ScopedCnf cnf;
            RTC->PRLH=0; RTC->PRLL=1; //Minimum allowed prescaler
            RTC->CNTH=0; RTC->CNTL=0;
            RTC->ALRH=0xffff; RTC->ALRL=0xffff;
        }
        //High priority for RTC (Max=0, min=15)
        NVIC_SetPriority(RTC_IRQn,3);
        NVIC_EnableIRQ(RTC_IRQn);

        // We can't stop the RTC during debugging, so debugging won't be easy.
        // Actually, we can't stop the RTC at all once we start it...
    }
};

static STM32F1RTC_Timer timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);

#ifdef WITH_DEEP_SLEEP

void IRQdeepSleepInit()
{
    EXTI->RTSR |= EXTI_RTSR_TR17;
    EXTI->IMR  |= EXTI_IMR_MR17; //enable wakeup interrupt
}

static bool IRQdeepSleepImpl(bool withTimeout)
{
    unsigned int lowerTickBefore=timer.IRQgetTimerCounter();
    #ifndef RUN_WITH_HSI
    // The HSI oscillator, even with PLL, starts so fast that it does not impact
    // lag from returining from application sleeps that resulted in deep sleep.
    // The HSE, however, takes up to 10 ticks to restart. In this case we want
    // to check whether the wakeup time is too close, and in that case not even
    // enter deep sleep. If the sleep is longer, we wakeup in advance and then
    // sleep the rest of the time. Tested with _tools/delay_test/os_timer_test.cpp
    const unsigned int minTicks=13;//TODO: increasing it further does not improve
    long long irqTick;
    if(withTimeout)
    {
        long long tick=timer.IRQgetTimeTickFromCounter(lowerTickBefore);
        irqTick=timer.IRQgetIrqTick();
        if(irqTick-tick<minTicks) return false; //Not enough time for deep sleep
        timer.IRQsetIrqTick(irqTick-minTicks+1);
    }
    #endif //RUN_WITH_HSI

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

    //Wait RSF part 1, until this is done RTC registers can't be read
    RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_RSF;

    //Try to save time by restarting the clock in the meantime
    SetSysClock();

    //Wait RSF part 2, until this is done RTC registers can't be read
    while((RTC->CRL & RTC_CRL_RSF)==0) ;

    // Workaround for hardware bug! The RTC overflow flag is not set if the
    // overflow occurs while we are in deep sleep. It is just lost. Of course,
    // we should also handle the case in which we didn't enter deep sleep
    // because of a pending interrupt, so we also check that OWF really isn't 1
    // TODO: last thing I'm not sure is if this works for very long sleeps
    // spanning multiple overflows, just don't sleep for more than 72 hours
    // to be sure
    unsigned int lowerTickAfter=timer.IRQgetTimerCounter();
    if(lowerTickAfter<lowerTickBefore && !(RTC->CRL & RTC_CRL_OWF))
        timer.IRQquirkIncrementUpperCounter();

    #ifndef RUN_WITH_HSI
    if(withTimeout)
    {
        //Restore the previous interrupt time and return false so we consume the
        //slack time in (non deep) sleep
        timer.IRQsetIrqTick(irqTick);
        return false;
    }
    #endif //RUN_WITH_HSI
    return true;
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
     */
    return IRQdeepSleepImpl(true);
}

bool IRQdeepSleep()
{
    return IRQdeepSleepImpl(false);
}

#endif //WITH_DEEP_SLEEP

/*
// Test code for checking the presence of the race condition. Call from main.
// Set RTC->CNTH=0xffff; RTC->CNTL=0; in timer init not to wait 72 hours till test end.
void test()
{
    FastInterruptDisableLock dLock;
    long long lastgood=0;
    for(;;)
    {
        auto current=timer.IRQgetTimeTick();
        if(current>0x180000000LL)
        {
            FastInterruptEnableLock eLock(dLock);
            iprintf("Test failed fail=0x%llx lastgood=0x%llx\n",current,lastgood);
        } else if(current==0x100001000LL) IRQerrorLog("Test end\r\n");
        lastgood=current;
    }
}*/
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
