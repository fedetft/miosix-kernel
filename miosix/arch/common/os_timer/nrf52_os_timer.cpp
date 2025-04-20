/***************************************************************************
 *   Copyright (C) 2025 by Terraneo Federico                               *
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

#include "kernel/lock.h"
#include "interfaces/arch_registers.h"
#include "interfaces_private/os_timer.h"

namespace miosix {

/*
 * Is it possible that every timer must have its weirdness?
 * The nRF timer is so minimalistic that the timer counter isn't readable nor
 * writable by software, and there's no interrupt on overflow either.
 * Additionally, while the datasheet states that there are 6 capture/compare
 * channels per timer, if you read the fine print, TIMER0 to TIMER3 only have
 * the first 4, so we use them as follow:
 * - channel 3 to emulate reading the timer with capture events in software
 * - channel 2 to emulate the timer overflow interrupt
 * - channel 0 as the actual match value to implement the OS timer
 * Moreover, we use a software variable, offset, to emulate setting the timer
 * counter to an arbitrary value, since the timer counter can only be cleared.
 *
 * Finally, reading the timer registers seems to take quite a few clock cycles
 * as a test loop to read the timer as soon as it overflows
 * while(NRF_TIMER1->EVENTS_COMPARE[2]==0) ;
 * NRF_TIMER1->TASKS_CAPTURE[3]=1;
 * int value=NRF_TIMER1->CC[3];
 * results in value between 6 and 8, thus the act of reading TASKS_CAPTURE and
 * CC takes between 24 and 32 clock cycles...
 */

static unsigned int offset=0;

class NRFTimer1 : public TimerAdapter<NRFTimer1, 32>
{
public:
    static inline unsigned int IRQgetTimerCounter()
    {
        //Emulate reading by generating a capture event and adding offset
        NRF_TIMER1->TASKS_CAPTURE[3]=1;
        return NRF_TIMER1->CC[3]+offset;
    }
    static inline void IRQsetTimerCounter(unsigned int v)
    {
        //Emulate writing by clearing, setting offset and fixing overflow IRQ
        NRF_TIMER1->TASKS_CLEAR=1;
        NRF_TIMER1->CC[2]=-v; //Unsigned complement
        offset=v;
    }

    static inline unsigned int IRQgetTimerMatchReg()
    {
        return NRF_TIMER1->CC[0]+offset;
    }
    static inline void IRQsetTimerMatchReg(unsigned int v)
    {
        NRF_TIMER1->CC[0]=v-offset;
    }

    static inline bool IRQgetOverflowFlag()
    {
        return NRF_TIMER1->EVENTS_COMPARE[2];
    }
    static inline void IRQclearOverflowFlag()
    {
        NRF_TIMER1->EVENTS_COMPARE[2]=0;
    }

    static inline bool IRQgetMatchFlag()
    {
        return NRF_TIMER1->EVENTS_COMPARE[0];
    }
    static inline void IRQclearMatchFlag()
    {
        NRF_TIMER1->EVENTS_COMPARE[0]=0;
    }

    static inline void IRQforcePendingIrq() { NVIC_SetPendingIRQ(TIMER1_IRQn); }

    static inline void IRQstopTimer() { NRF_TIMER1->TASKS_STOP=1; }
    static inline void IRQstartTimer() { NRF_TIMER1->TASKS_START=1; }

    static unsigned int IRQTimerFrequency() { return 16000000; }

    void IRQinitTimer()
    {
        NRF_TIMER1->TASKS_STOP=1;
        NRF_TIMER1->TASKS_CLEAR=1;
        NRF_TIMER1->MODE=0;      //Timer mode
        NRF_TIMER1->BITMODE=3;   //32bit
        NRF_TIMER1->PRESCALER=0; //Maximum frequency
        NRF_TIMER1->CC[2]=0;     //Emulate overflow IRQ
        NRF_TIMER1->INTENSET=0b000101<<16; //Enable IRQ on channel 0 and 2
        IRQregisterIrq(TIMER1_IRQn,&TimerAdapter<NRFTimer1,32>::IRQhandler,
                       static_cast<TimerAdapter<NRFTimer1,32>*>(this));
    }
};

static NRFTimer1 timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLEMENTATION(timer);

} //namespace miosix
