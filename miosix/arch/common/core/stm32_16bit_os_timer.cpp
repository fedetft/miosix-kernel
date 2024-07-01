/***************************************************************************
 *   Copyright (C) 2021 by Terraneo Federico                               *
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

#ifndef WITH_RTC_AS_OS_TIMER

namespace miosix {

#if defined(STM32F030x8)

class STM32Timer14HW
{
public:
    static inline TIM_TypeDef *get() { return TIM14; }
    static inline IRQn_Type getIRQn() { return TIM14_IRQn; }
    static inline unsigned int IRQgetClock()
    {
        unsigned int timerInputFreq=SystemCoreClock;
        if(RCC->CFGR & RCC_CFGR_PPRE_2) timerInputFreq/=1<<((RCC->CFGR>>RCC_CFGR_PPRE_Pos) & 0x3);
        return timerInputFreq;
    }
    static inline void IRQenable()
    {
        RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
        RCC_SYNC();
        DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM14_STOP; //Stop while debugging
    }
};
#define IRQ_HANDLER_NAME TIM14_IRQHandler
#define TIMER_HW_CLASS STM32Timer14HW

#elif defined(_ARCH_CORTEXM0PLUS_STM32L0)

class STM32Timer22HW
{
public:
    static inline TIM_TypeDef *get() { return TIM22; }
    static inline IRQn_Type getIRQn() { return TIM22_IRQn; }
    static inline unsigned int IRQgetClock()
    {
        unsigned int timerInputFreq=SystemCoreClock;
        if(RCC->CFGR & RCC_CFGR_PPRE2_2) timerInputFreq/=1<<((RCC->CFGR>>RCC_CFGR_PPRE2_Pos) & 0x3);
        return timerInputFreq;
    }
    static inline void IRQenable()
    {
        RCC->APB2ENR |= RCC_APB2ENR_TIM22EN;
        RCC_SYNC();
        DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM22_STOP; //Stop while debugging
    }
};
#define IRQ_HANDLER_NAME TIM22_IRQHandler
#define TIMER_HW_CLASS STM32Timer22HW

#else

class STM32Timer4HW
{
public:
    static inline TIM_TypeDef *get() { return TIM4; }
    static inline IRQn_Type getIRQn() { return TIM4_IRQn; }
    static inline unsigned int IRQgetClock()
    {
        unsigned int timerInputFreq=SystemCoreClock;
        if(RCC->CFGR & RCC_CFGR_PPRE1_2) timerInputFreq/=1<<((RCC->CFGR>>RCC_CFGR_PPRE1_Pos) & 0x3);
        return timerInputFreq;
    }
    static inline void IRQenable()
    {
        RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
        RCC_SYNC();
        #ifndef _ARCH_CORTEXM3_STM32L1
        DBGMCU->CR |= DBGMCU_CR_DBG_TIM4_STOP; //Stop while debugging
        #else
        DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM4_STOP; //Stop while debugging
        #endif
    }
};
#define IRQ_HANDLER_NAME TIM4_IRQHandler
#define TIMER_HW_CLASS STM32Timer4HW

#endif

template<class T>
class STM32Timer : public TimerAdapter<STM32Timer<T>, 16>
{
public:
    static inline unsigned int IRQgetTimerCounter() { return T::get()->CNT; }
    static inline void IRQsetTimerCounter(unsigned int v) { T::get()->CNT=v; }

    static inline unsigned int IRQgetTimerMatchReg() { return T::get()->CCR1; }
    static inline void IRQsetTimerMatchReg(unsigned int v) { T::get()->CCR1=v; }

    static inline bool IRQgetOverflowFlag() { return T::get()->SR & TIM_SR_UIF; }
    static inline void IRQclearOverflowFlag() { T::get()->SR = ~TIM_SR_UIF; }
    
    static inline bool IRQgetMatchFlag() { return T::get()->SR & TIM_SR_CC1IF; }
    static inline void IRQclearMatchFlag() { T::get()->SR = ~TIM_SR_CC1IF; }
    
    static inline void IRQforcePendingIrq() { NVIC_SetPendingIRQ(T::getIRQn()); }

    static inline void IRQstopTimer() { T::get()->CR1 &= ~TIM_CR1_CEN; }
    static inline void IRQstartTimer() { T::get()->CR1 |= TIM_CR1_CEN; }
    
    /*
     * These microcontrollers unfortunately do not have a 32bit timer.
     * 16bit timers overflow too frequently if clocked at the maximum speed,
     * and this is bad both because it nullifies the gains of a tickless kernel
     * and because if interrupts are disabled for an entire timer period the
     * OS loses the knowledge of time. For this reason, we set the timer clock
     * to a lower value using the prescaler.
     */
    static const int timerFrequency=1000000; //1MHz
    
    static unsigned int IRQTimerFrequency()
    {
        return timerFrequency;
    }
    
    static void IRQinitTimer()
    {
        T::IRQenable();
        // Setup TIM4 base configuration
        // Mode: Up-counter
        // Interrupts: counter overflow, Compare/Capture on channel 1
        T::get()->CR1=TIM_CR1_URS;
        T::get()->DIER=TIM_DIER_UIE | TIM_DIER_CC1IE;
        NVIC_SetPriority(T::getIRQn(),3); //High priority for TIM4 (Max=0, min=15)
        NVIC_EnableIRQ(T::getIRQn());
        // Configure channel 1 as:
        // Output channel (CC1S=0)
        // No preload(OC1PE=0), hence TIM4_CCR1 can be written at anytime
        // No effect on the output signal on match (OC1M = 0)
        T::get()->CCMR1 = 0;
        T::get()->CCR1 = 0;
        // TIM4 Operation Frequency Configuration: Max Freq. and longest period
        
        // The global variable SystemCoreClock from ARM's CMSIS allows to know
        // the CPU frequency.
        // However, the timer frequency may be a submultiple of the CPU frequency,
        // due to the bus at whch the periheral is connected being slower. The
        // RCC->CFGR register tells us how slower the APB bus is running.
        // This formula takes into account that if the APB clock is divided by a
        // factor of two or greater, the timer is clocked at twice the bus
        // interface. After this, the freq variable contains the frequency in Hz
        // at which the timer prescaler is clocked.
        // The field in the RCC registers where we need to look at depends on
        // the specific timer, so this computation is defined in the hardware
        // abstraction class.
        unsigned int timerInputFreq=T::IRQgetClock();
        
        //Handle the case when the prescribed timer frequency is not achievable.
        //For now, we just enter an infinite loop so if someone selects an
        //impossible frequency it won't go unnoticed during testing
        if(timerInputFreq % timerFrequency)
        {
            IRQbootlog("Frequency error\r\n");
            for(;;) ;
        }
        
        T::get()->PSC = (timerInputFreq/timerFrequency)-1;
        T::get()->ARR = 0xFFFF;
        T::get()->EGR = TIM_EGR_UG; //To enforce the timer to apply PSC
    }
};

static STM32Timer<TIMER_HW_CLASS> timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);
} //namespace miosix

void __attribute__((naked)) IRQ_HANDLER_NAME()
{
    saveContext();
    asm volatile ("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((used)) osTimerImpl()
{
    miosix::timer.IRQhandler();
}

#endif //#ifndef WITH_RTC_AS_OS_TIMER
