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

namespace miosix {

#if defined(_ARCH_CORTEXM0_STM32F0) || defined(_ARCH_CORTEXM4_STM32F3)

class STM32Timer2HW
{
public:
    static inline TIM_TypeDef *get() { return TIM2; }
    static inline IRQn_Type getIRQn() { return TIM2_IRQn; }
    static inline int IRQgetClock()
    {
        unsigned int result=SystemCoreClock;
        #if _ARCH_CORTEXM0_STM32F0
        if(RCC->CFGR & RCC_CFGR_PPRE_2) result/=1<<((RCC->CFGR>>8) & 0x3);
        #else
        if(RCC->CFGR & RCC_CFGR_PPRE1_2) result/=1<<((RCC->CFGR>>8) & 0x3);
        #endif
        return result;
    }
    static inline void IRQenable()
    {
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
        RCC_SYNC();
        DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM2_STOP; //Stop while debugging
    }
};
#define IRQ_HANDLER_NAME TIM2_IRQHandler
#define TIMER_HW_CLASS STM32Timer2HW

#else

class STM32Timer5HW
{
public:
    static inline TIM_TypeDef *get() { return TIM5; }
    static inline IRQn_Type getIRQn() { return TIM5_IRQn; }
    static inline int IRQgetClock()
    {
        unsigned int result=SystemCoreClock;
        #if defined(_ARCH_CORTEXM7_STM32H7)
        #ifndef STM32H753xx
        // In stm32h723/h755 MCUs there isn't any prescaler 2x, for this reason when the
        // prescaler is enabled we will have to divide it for 2^PPRE and not for 2^(PPRE-1)
        if(RCC->D2CFGR & RCC_D2CFGR_D2PPRE1_2) result/=(2<<((RCC->D2CFGR>>4) & 0x3));
        #else
        if(RCC->D2CFGR & RCC_D2CFGR_D2PPRE1_2) result/=(1<<((RCC->D2CFGR>>4) & 0x3));
        #endif
        #else
        if(RCC->CFGR & RCC_CFGR_PPRE1_2) result/=1<<((RCC->CFGR>>10) & 0x3);
        #endif
        return result;
    }
    static inline void IRQenable()
    {
        #if defined(_ARCH_CORTEXM7_STM32H7)
        RCC->APB1LENR |= RCC_APB1LENR_TIM5EN;
        RCC_SYNC();
        DBGMCU->APB1LFZ1 |= DBGMCU_APB1LFZ1_DBG_TIM5; //Stop while debugging
        #elif defined(_ARCH_CORTEXM4_STM32L4)
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
        RCC_SYNC();
        DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_TIM5_STOP; //Stop while debugging
        #else
        RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
        RCC_SYNC();
        DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM5_STOP; //Stop while debugging
        #endif
    }
};
#define IRQ_HANDLER_NAME TIM5_IRQHandler
#define TIMER_HW_CLASS STM32Timer5HW

#endif

template<class T>
class STM32Timer : public TimerAdapter<STM32Timer<T>, 32>
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
    
    static unsigned int IRQTimerFrequency()
    {
        // The global variable SystemCoreClock from ARM's CMSIS allows to know
        // the CPU frequency.
        // However, the timer frequency may be a submultiple of the CPU frequency,
        // due to the bus at whch the periheral is connected being slower. The
        // RCC configuration registers tells us how slower the APB1 bus is running.
        // This formula takes into account that if the APB1 clock is divided by a
        // factor of two or greater, the timer is clocked at twice the bus
        // interface. After this, the freq variable contains the frequency in Hz
        // at which the timer prescaler is clocked.
        // The field in the RCC registers where we need to look at depends on
        // the specific timer, so this computation is defined in the hardware
        // abstraction class.
        return T::IRQgetClock();
    }
    
    static void IRQinitTimer()
    {
        T::IRQenable();
        // Setup TIM5 base configuration
        // Mode: Up-counter
        // Interrupts: counter overflow, Compare/Capture on channel 1
        T::get()->CR1=TIM_CR1_URS;
        T::get()->DIER=TIM_DIER_UIE | TIM_DIER_CC1IE;
        NVIC_SetPriority(T::getIRQn(),3); //High priority for TIM5 (Max=0, min=15)
        NVIC_EnableIRQ(T::getIRQn());
        // Configure channel 1 as:
        // Output channel (CC1S=0)
        // No preload(OC1PE=0), hence TIM5_CCR1 can be written at anytime
        // No effect on the output signal on match (OC1M = 0)
        T::get()->CCMR1 = 0;
        T::get()->CCR1 = 0;
        // TIM5 Operation Frequency Configuration: Max Freq. and longest period
        T::get()->PSC = 0;
        T::get()->ARR = 0xFFFFFFFF;
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
