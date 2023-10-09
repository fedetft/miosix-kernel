/***************************************************************************
 *   Copyright (C) 2023 by Terraneo Federico                               *
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

class EFM32Timer2 : public TimerAdapter<EFM32Timer2, 16>
{
public:
    static inline unsigned int IRQgetTimerCounter() { return TIMER2->CNT; }
    static inline void IRQsetTimerCounter(unsigned int v) { TIMER2->CNT=v; }

    static inline unsigned int IRQgetTimerMatchReg()
    {
        //Quirk: timer matches one count after the CCV value is reached.
        //The cast is necessary as if CCV is 0xffff the irq will trigger at 0x0
        return static_cast<unsigned short>(TIMER2->CC[0].CCV+1);
    }

    static inline void IRQsetTimerMatchReg(unsigned int v)
    {
        //Quirk: timer matches one count after the CCV value is reached.
        //The cast is necessary as if v is 0x0 the timer must be set to 0xffff
        TIMER2->CC[0].CCV=static_cast<unsigned short>(v-1);
    }

    static inline bool IRQgetOverflowFlag() { return TIMER2->IF & TIMER_IF_OF; }
    static inline void IRQclearOverflowFlag() { TIMER2->IFC=TIMER_IFC_OF; }

    static inline bool IRQgetMatchFlag() { return TIMER2->IF & TIMER_IF_CC0; }
    static inline void IRQclearMatchFlag() { TIMER2->IFC=TIMER_IFC_CC0; }

    static inline void IRQforcePendingIrq() { NVIC_SetPendingIRQ(TIMER2_IRQn); }

    static inline void IRQstopTimer() { TIMER2->CMD=TIMER_CMD_STOP; }
    static inline void IRQstartTimer() { TIMER2->CMD=TIMER_CMD_START; }

    /*
     * These microcontrollers unfortunately do not have a 32bit timer.
     * 16bit timers overflow too frequently if clocked at the maximum speed,
     * and this is bad both because it nullifies the gains of a tickless kernel
     * and because if interrupts are disabled for an entire timer period the
     * OS loses the knowledge of time. For this reason, we set the timer clock
     * to a lower value using the prescaler.
     * However, on't top of that these MCUs have a prescaler that can only
     * divide by powers of two. For now, we fix the prescaler value of 16
     * which seems about the right tradeoff with the clocks EFM32 use and get
     * whatever timer clock comes out.
     */

    //Warning: just changing this line won't change the prescaler.
    //Modify the actual prescaler setting in TIMER2->CTRL
    static const int prescaler=16;

    static unsigned int IRQTimerFrequency()
    {
        unsigned int result=SystemCoreClock;
        //EFM32 has separate prescalers for core and peripherals, so we start
        //from HFCORECLK, work our way up to HFCLK and then down to HFPERCLK
        result*=1<<(CMU->HFCORECLKDIV & _CMU_HFCORECLKDIV_HFCORECLKDIV_MASK);
        result/=1<<(CMU->HFPERCLKDIV & _CMU_HFPERCLKDIV_HFPERCLKDIV_MASK);
        return result/prescaler;
    }

    static void IRQinitTimer()
    {
        CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_TIMER2;

        // MODE=0     Up-counter
        // DEBUGRUN=0 Stop while debugging
        // PRESC      divide by 16
        TIMER2->CTRL=TIMER_CTRL_PRESC_DIV16;

        TIMER2->CNT=0;
        TIMER2->TOP=0xffff;
        TIMER2->IFC=0xffffffff;
        TIMER2->IEN=TIMER_IEN_CC0 | TIMER_IEN_OF;
        TIMER2->ROUTE=0;

        TIMER2->CC[0].CTRL=TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
        TIMER2->CC[0].CCV=0xffff;

        NVIC_SetPriority(TIMER2_IRQn,3); //High priority (Max=0, min=15)
        NVIC_EnableIRQ(TIMER2_IRQn);
    }
};

static EFM32Timer2 timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);
} //namespace miosix

void __attribute__((naked)) TIMER2_IRQHandler()
{
    saveContext();
    asm volatile ("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((used)) osTimerImpl()
{
    miosix::timer.IRQhandler();
}
