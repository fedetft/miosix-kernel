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

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <kernel/timeconversion.h>
#include <kernel/kernel.h>
#include <kernel/sync.h>

namespace miosix {

  /**
   * \brief used to initialize power management
   * class at boot phase
   **/
  void IRQpowerManagementInit();
 
  /**
   * \brief Class implementing the functionalities 
   *        of the RTC peripherla of the board
   *
   * All the wait and deepSleep functions cannot be called concurrently by
   * multiple threads, so there is a single instance of the class that is share * among all the threads
   */
  class PowerManagement
  {

public:
    /**
     * \return an instance of the power management class (singleton) 
     */
    static PowerManagement& instance();
    
    /**
     * \return true if the USB cable is connected 
     */
    bool isUsbConnected() const;
    
    /**
     * \return true if the battery is currently charging. For this to happen,
     * charging must be allowed, the USB cable must be connected, and the
     * battery must not be fully charged. 
     */
    bool isCharging();
    
    /** 
     * \return the battery charge status as a number in the 0..100 range.
     * The reading takes ~5ms 
     */
    int getBatteryStatus();
    
    /**
     * \return the battery voltage, in millivolts. So 3700 means 3.7V. 
     * The reading takes ~5ms 
     */
    int getBatteryVoltage();
    
    /**
     * Possible values for the core frequency, to save power
     */
    enum CoreFrequency
    {
        FREQ_120MHz=120, ///< Default value is 120MHz
        FREQ_26MHz=26    ///< 26MHz, to save power
    };
    
    /**
     * Allows to change the core frequency to reduce power consumption.
     * Miosix by default boots at the highest frequency (120MHz).
     * According to the datasheet, the microcontroller current consumption is
     * this: (the difference between minimum and maximum depend on the number
     * of peripherals that are emabled)
     *        | Run mode    | Sleep mode  |
     *        | min  | max  | min  | max  |
     * 120MHz | 21mA | 49mA |  8mA | 38mA |
     * 26MHz  |  5mA | 11mA |  3mA |  8mA |
     * 
     * For greater power savings consider entering deep sleep as well.
     * 
     * Note that changing the frequency takes a significant amount of time, and
     * that if you are designing a multithreaded application, you have to make
     * sure all your threads are in an interruptible point. For example, if
     * you call this function while a thread is transfering data through I2C,
     * it may cause problems.
     */
    void setCoreFrequency(CoreFrequency cf);
    
    /**
     * \return the current core frequency 
     */
    CoreFrequency getCoreFrequency() const { return coreFreq; }
    
    /**
     * Enters deep sleep. ST calls this mode "Stop". It completely turns off the
     * microcontroller and its peripherals, minus the RTC. Power gating is not
     * applied, so the CPU registers, RAM and peripheral contents are preserved.
     * The current consumption goes down to 300uA, and with the 110mAh battery
     * in the sony watch, it would last 366 hours in this state. The packaging
     * of the watch says that the standby time is 330 hours, so this is clearly
     * how they did it.
     * 
     * There are a few words of warning for using this mode, though
     * - Entering/exiting deep sleep may take a significant time, so the sleep
     *   time may not be precise down to the last millisecond.
     * - You have to turn off all other stuff external to the microcontroller
     *   that draw power to actually reduce the consumption to 300uA. If you
     *   leave the display ON, or the accelerometer, the consumption will be
     *   higher. The Driver for the battery voltage measurement and light sensor
     *   already turn off the enable pin so don't worry.
     * - If you are designing a multithreaded application, you have to make
     *   sure all your threads are in an interruptible point. For example, if
     *   you call this function while a thread is transfering data through I2C,
     *   it may cause problems.
     * - Also, the BSP needs to be optimized for low power, and this is a TODO.
     *   Even leaving a single GPIO floating can significantly increase the
     *   power consumption. Normally, I would do that using a multimeter to
     *   measure current and an oscilloscope to probe around, but I don't want
     *   to open my watch, and there is no schematic, which makes things harder.
     *   For this reason, I can't guarantee that the current will be as low as
     *   300uA.
     * - Also, for now I've implemented only timed wakeup. What will be
     *   interesting is to also have event wakeup. For example, wake up on
     *   accelerometer tapping detected, or on RTC alarms that can be set at a
     *   given date and time.
     * 
     * \param ms number of nanoseconds to stay in deep sleep. Maximum is 30s
     */
    void IRQgoDeepSleepFor(unsigned long long ns);
    
    /**
     * Allows to configure if we should exit the deep sleep earlier than the
     * timeout if the button is pressed (default is false)
     * \param wake if true, pushing the button will make goDeepSleep() return
     * before its timeout
     */
    void setWakeOnButton(bool wake) { wakeOnButton=wake; }
    
    /**
     * \return true if wake on button is set 
     */
    bool getWakeOnButton() const { return wakeOnButton; }
    
    /**
     * Locking the power management allows to access hardware operation without
     * the risk of a frequency change in the middle, or entering deep sleep.
     * Since the power management exposes the lock() and unlock() member
     * functions (i.e, the lockable concept), it can be treated as a mutex:
     * \code
     * {
     *     Lock<PowerManagement> l(PowerManagement::instance());
     *     //Do something without the risk of being interrupted by a frequency
     *     //change
     * }
     * Note that you should eventually release the mutex, or calls to
     * setCoreFrequency() and goDeepSleep() will never return. Also, if
     * you are going to lock both the power management and the i2c mutex, make
     * sure to always lock ithe i2c mutex <b>after</b> the power management, or
     * a deadlock may occur.
     * 
     * \endcode
     */
    void lock() { powerManagementMutex.lock(); }
    
    /**
     * Unlock the power management, allowing frequency changes and entering deep
     * sleep again
     */
    void unlock() { powerManagementMutex.unlock(); }

    PowerManagement(const PowerManagement&) = delete;
    PowerManagement& operator=(const PowerManagement&) = delete;
    
private:
    
    /**
     * Constructor
     */
    PowerManagement();
    
    /**
     * Reconfigure the system clock after the microcontroller has been in deep
     * sleep. Can only be called with interrupts disabled.
     */
    void IRQsetSystemClock();
    
    /**
     * Set clock prescalers based on clock frequency.
     *  Can only be called with interrupts disabled.
     */
    
    /**
     *  Set core frequency. Can only be called with interrupts disabled.
     */
    void IRQsetCoreFreq();

    /** 
     * common procedure to enter the Stop Mode once the RTC Wakeup Timer counter
     * is set
     **/
    void IRQenterStopMode();
    bool chargingAllowed;
    bool wakeOnButton;
    CoreFrequency coreFreq;
    FastMutex powerManagementMutex;
//    std::list<std::function<void (bool)> > notifier;
  };
}

#endif //POWER_H
