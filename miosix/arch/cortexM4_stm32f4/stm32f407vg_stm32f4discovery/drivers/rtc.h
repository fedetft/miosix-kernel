/***************************************************************************
 *   Copyright (C) 2017 by Terraneo Federico                               *
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

/***********************************************************************
* rtc.h Part of the Miosix Embedded OS.
* Rtc support for the board that initialize correctly the RTC peripheral,
* and exposes its functionalities.
************************************************************************/

#ifndef RTC_H
#define RTC_H

#include <kernel/timeconversion.h>
#include <kernel/kernel.h>

namespace miosix {

/**
 * \brief Class implementing the functionalities 
 *        of the RTC peripherla of the board
 *
 * All the wait and deepSleep functions cannot be called concurrently by
 * multiple threads, so there is a single instance of the class that is share * among all the threads
 */
class Rtc
{
public:
    /**
     * \return an instance of this class
     */
    static Rtc* instance();


    /**
     * Function used to setup the wakeup interrupt for the 
     * RTC and the associated NVIC IRQ and EXTI lines.
     * It also store the correct value for the clock used by RTC
     * and the wakeup clock period
     */
    void setWakeupInterrupt();

    /**
     * Set wakeup timer for wakeup interrupt according to the
     * procedure described in the reference manual of the
     * STM32F407VG
     */
    void setWakeupTimer(unsigned short int);
    /**
     * \return the Sub second value of the Time register of the RTC
     *         as milliseconds.
     *
     * The value is converted according to the current clock used to
     * synchronize the RTC. 
     */
    unsigned short int getSSR();
    unsigned long long int getDate();
    unsigned long long int getTime();

private:
    Rtc();
    Rtc(const Rtc&);
    Rtc& operator= (const Rtc&);
    unsigned int clock_freq = 0; // Hz set according to the selected clock
    unsigned int wkp_clock_period = 0; // how much ms often the wut counter is decreased
    unsigned short int prescaler_s = 0; // Need to know the prescaler factor 
    
    long int remaining_wakeups = 0; ///< keep track of remaining wakeups after 1 second
    /*
     * not preemptable function that read SSR value of the RTC Time register
     */
    unsigned short int IRQgetSSR(FastInterruptDisableLock&);
    /*
     * not preemptable function that compute the time of the RTC Time register
     */
    unsigned long long int IRQgetTime(FastInterruptDisableLock&);
    /*
     * not preemptable function that compute the date of the RTC calendar value
     */
    unsigned long long int IRQgetDate(FastInterruptDisableLock&);

    friend void IRQdeepSleep(long long int value);
};

} //namespace miosix

#endif //RTC_H
