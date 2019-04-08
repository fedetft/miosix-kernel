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
#include "interfaces/portability.h"
#include "interfaces/arch_registers.h"
#include "drivers/rtc.h"
#include "drivers/power_manager.h"
#include "miosix.h"

namespace miosix {
  
void IRQdeepSleep(unsigned long long int abstime)
{
    using red = Gpio<GPIOD_BASE,10>;
    red::mode(Mode::OUTPUT);
    Rtc& rtc = Rtc::instance();
    PowerManagement& pm = PowerManagement::instance();
    ContextSwitchTimer cstimer = ContextSwitchTimer::instance();
    unsigned long long int reltime = abstime - cstimer.IRQgetCurrentTime();
    if (reltime < rtc.getMinimumDeepSleepPeriod())
    {
        // Too late for deep-sleep, use normal sleep
        miosix_private::sleepCpu();
        return; 
    }
    else
    {
        red::high();
        pm.IRQgoDeepSleepFor(reltime);
        red::low();
    }
    cstimer.IRQsetCurrentTime(abstime);
}

void IRQdeepSleepInit()
{
    PowerManagement& pm = PowerManagement::instance();
    Rtc& rtc = Rtc::instance();
    rtc.setWakeupInterrupt();
}

} //namespace miosix
