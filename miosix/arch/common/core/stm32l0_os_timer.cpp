/***************************************************************************
 *   Copyright (C) 2021 by Federico Terraneo                               *
 *   Copyright (C) 2023 by Daniele Cattaneo                                *
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

namespace miosix {

class STM32Timer22 : public TimerAdapter<STM32Timer22, 16>
{
public:
    static inline unsigned int IRQgetTimerCounter() { return TIM22->CNT; }
    static inline void IRQsetTimerCounter(unsigned int v) { TIM22->CNT=v; }

    static inline unsigned int IRQgetTimerMatchReg() { return TIM22->CCR1; }
    static inline void IRQsetTimerMatchReg(unsigned int v) { TIM22->CCR1=v; }

    static inline bool IRQgetOverflowFlag() { return TIM22->SR & TIM_SR_UIF; }
    static inline void IRQclearOverflowFlag() { TIM22->SR = ~TIM_SR_UIF; }
    
    static inline bool IRQgetMatchFlag() { return TIM22->SR & TIM_SR_CC1IF; }
    static inline void IRQclearMatchFlag() { TIM22->SR = ~TIM_SR_CC1IF; }
    
    static inline void IRQforcePendingIrq() { NVIC_SetPendingIRQ(TIM22_IRQn); }

    static inline void IRQstopTimer() { TIM22->CR1 &= ~TIM_CR1_CEN; }
    static inline void IRQstartTimer() { TIM22->CR1 |= TIM_CR1_CEN; }
    
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
        RCC->APB2ENR |= RCC_APB2ENR_TIM22EN;
        RCC_SYNC();
        DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM22_STOP; //Stop while debugging
        // Setup TIM22 base configuration
        // Mode: Up-counter
        // Interrupts: counter overflow, Compare/Capture on channel 1
        TIM22->CR1=TIM_CR1_URS;
        TIM22->DIER=TIM_DIER_UIE | TIM_DIER_CC1IE;
        NVIC_SetPriority(TIM22_IRQn,3); //High priority for TIM22 (Max=0, min=15)
        NVIC_EnableIRQ(TIM22_IRQn);
        // Configure channel 1 as:
        // Output channel (CC1S=0)
        // No preload(OC1PE=0), hence TIM4_CCR1 can be written at anytime
        // No effect on the output signal on match (OC1M = 0)
        TIM22->CCMR1 = 0;
        TIM22->CCR1 = 0;
        // TIM22 Operation Frequency Configuration: Max Freq. and longest period
        
        // The global variable SystemCoreClock from ARM's CMSIS allows to know
        // the CPU frequency.
        unsigned int timerInputFreq=SystemCoreClock;

        // The timer frequency may however be a submultiple of the CPU frequency,
        // due to the bus at whch the periheral is connected being slower. The
        // RCC->CFGR register tells us how slower the APB1 bus is running.
        // This formula takes into account that if the APB1 clock is divided by a
        // factor of two or greater, the timer is clocked at twice the bus
        // interface. After this, the freq variable contains the frequency in Hz
        // at which the timer prescaler is clocked.
        if(RCC->CFGR & RCC_CFGR_PPRE1_2) timerInputFreq/=1<<((RCC->CFGR>>8) & 0x3);
        
        //Handle the case when the prescribed timer frequency is not achievable.
        //For now, we just enter an infinite loop so if someone selects an
        //impossible frequency it won't go unnoticed during testing
        if(timerInputFreq % timerFrequency)
        {
            IRQbootlog("Frequency error\r\n");
            for(;;) ;
        }
        
        TIM22->PSC = (timerInputFreq/timerFrequency)-1;
        TIM22->ARR = 0xFFFF;
        TIM22->EGR = TIM_EGR_UG; //To enforce the timer to apply PSC
    }
};

static STM32Timer22 timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);
} //namespace miosix

void __attribute__((naked)) TIM22_IRQHandler()
{
    saveContext();
    asm volatile ("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((used)) osTimerImpl()
{
    miosix::timer.IRQhandler();
}
