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

#include "miosix.h"
#include "interfaces/deep_sleep.h"

#ifdef WITH_DEEP_SLEEP

#ifndef WITH_RTC_AS_OS_TIMER
#error For atsam4l target, WITH_DEEP_SLEEP requires WITH_RTC_AS_OS_TIMER
#endif //WITH_RTC_AS_OS_TIMER

using namespace std;

namespace miosix {

void IRQdeepSleepInit() {}
  
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
     *
     * This is why this code ignores the wakeup absolute time parameter, and the
     * only task is to write the proper sequence to enter the deep sleep state
     * instead of the sleep state.
     */
    return IRQdeepSleep();
}

bool IRQdeepSleep()
{
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; //Select wait mode
    __WFI();
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; //Unselect wait mode
    //The only core clock option we support is the internal RC oscillator and
    //the atsam microcontroller preserve its configuration across deep sleep
    //so there's no need to restore clocks upn wakeup
    return true;
}

} //namespace miosix

#endif //WITH_DEEP_SLEEP
