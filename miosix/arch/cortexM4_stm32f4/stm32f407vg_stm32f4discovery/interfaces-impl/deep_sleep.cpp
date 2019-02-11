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
#include "interfaces/cstimer.h"
#include "interfaces/arch_registers.h"
#include "drivers/rtc.h"

namespace miosix {

void IRQdeepSleep(unsigned long long int abstime)
{
    Rtc& rtc = Rtc::instance();
    ContextSwitchTimer cstimer = ContextSwitchTimer::instance();
    unsigned long long int reltime = abstime - cstimer.IRQgetCurrentTime(); // as nanoseconds delay from now
    // if ( reltime < 0 )
    // {
    //   // unsigned integer overflow
    //   rtc.IRQenterWakeupStopModeFor(2000 * 1000000); // for now the maximum stop mode interval is 2 seconds
    // }
    if (reltime < 120000)
    {
        return; // too late for sleeping
    }
    else if (reltime > 3000 * 1000000)
    {
        rtc.IRQenterWakeupStopModeFor(3000 * 1000000); // overflow detected
    }
    else
    {
        rtc.IRQenterWakeupStopModeFor(reltime);
    }
    cstimer.IRQsetCurrentTime(abstime);
}

void IRQdeepSleepInit()
{
    //   SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    // PWR->CR |= PWR_CR_LPDS;
    Rtc& rtc = Rtc::instance();
    rtc.setWakeupInterrupt();
}

} //namespace miosix
