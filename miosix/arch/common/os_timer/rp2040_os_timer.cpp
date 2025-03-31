/***************************************************************************
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
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

#include "config/miosix_settings.h"
#include "kernel/thread.h"
#include "interfaces/arch_registers.h"
#include "interfaces/delays.h"
#include "interfaces/interrupts.h"
#include "interfaces_private/os_timer.h"

namespace miosix {

static TimeConversion tc(48000000);
#ifdef WITH_SMP
static long long lastAlarmTicks[2]={0};
#else
static long long lastAlarmTicks[1]={0};
#endif

/**
 * \internal
 * Get raw tick count from the timer.
 * \returns the current tick count.
 */
static inline long long IRQgetTicks() noexcept
{
    //Timer has latching registers that however break when multiple cores read
    //at the same time, so don't use them
    unsigned int h1=timer_hw->timerawh;
    unsigned int l1=timer_hw->timerawl;
    unsigned int h2=timer_hw->timerawh;
    if(h1==h2)
        return static_cast<long long>(h1)<<32 | static_cast<long long>(l1);
    else
        return static_cast<long long>(h2)<<32 | static_cast<long long>(timer_hw->timerawl);
}

/**
 * \internal
 * Handles the timer interrupt, checking if the alarm period is indeed
 * elapsed and calling the kernel if so.
 */
template<unsigned char AlarmId>
static void IRQtimerInterruptHandler()
{
    FastGlobalLockFromIrq irq;
    timer_hw->intr=1<<AlarmId; //Bit is write-clear
    auto tnow=IRQgetTicks(), twake=lastAlarmTicks[AlarmId];
    //Check the full 64 bits. If the alarm deadline has passed, call the kernel.
    //Otherwise rearm the timer. Rearming the timer is also important to prevent
    //a race condition that occurs when IRQosTimerSetInterrupt is called right
    //as the previously set alarm is about to trigger. In this case the previous
    //timer interrupt clears the armed flag thus the next interrupt set with
    //IRQosTimerSetInterrupt would not occur unless rearmed.
    if(twake<=tnow) IRQwakeThreads(tc.tick2ns(tnow));
    else timer_hw->alarm[AlarmId]=static_cast<unsigned int>(twake & 0xffffffff);
}

long long getTime() noexcept
{
    FastGlobalIrqLock dLock;
    return tc.tick2ns(IRQgetTicks());
}

long long IRQgetTime() noexcept
{
    return tc.tick2ns(IRQgetTicks());
}

/**
 * \internal
 * Initialize and start the os timer.
 * It is used by the kernel, and should not be used by end users.
 */
void IRQosTimerInit()
{
    //Bring timer out of reset
    resets_hw->reset&= ~RESETS_RESET_TIMER_BITS;
    while((resets_hw->reset_done & RESETS_RESET_TIMER_BITS)==0) ;
    //Enable timer interrupt generation
    #ifdef WITH_SMP
    timer_hw->inte=TIMER_INTE_ALARM_0_BITS|TIMER_INTE_ALARM_1_BITS;
    #else
    timer_hw->inte=TIMER_INTE_ALARM_0_BITS;
    IRQregisterIrq(TIMER_IRQ_0_IRQn,IRQtimerInterruptHandler<0>);
    #endif
    //Toggle debug sleep mode. Works around a bug where the timer does not
    //start counting if it was reset while it was paused due to debug mode.
    timer_hw->dbgpause=0;
    delayUs(1);
    timer_hw->dbgpause=3;
}

#ifdef WITH_SMP
/**
 * \internal
 * Initialize the OS timer for a given core during SMP setup.
 * This function is used by the kernel, and should not be used by end users, and
 * is called by SMP setup code.
 * On non-SMP platforms it is not called.
 */
void IRQosTimerInitSMP()
{
    if(getCurrentCoreId()==0)
    {
        IRQregisterIrq(TIMER_IRQ_0_IRQn,IRQtimerInterruptHandler<0>);
    } else {
        IRQregisterIrq(TIMER_IRQ_1_IRQn,IRQtimerInterruptHandler<1>);
    }
}
#endif

/**
 * \internal
 * Set the next interrupt on the current core.
 * It is used by the kernel, and should not be used by end users.
 * Can be called with interrupts disabled or within an interrupt.
 * The hardware timer handles only one outstading interrupt request at a
 * time, so a new call before the interrupt expires cancels the previous one.
 * \param ns the absolute time when the interrupt will be fired, in nanoseconds.
 * When the interrupt fires, it shall call the
 * \code
 * bool IRQwakeThreads(long long currentTime);
 * \endcode
 * function defined in thread.cpp
 */
void IRQosTimerSetInterrupt(long long ns) noexcept
{
    unsigned char core=getCurrentCoreId();
    auto twake=tc.ns2tick(ns);
    lastAlarmTicks[core]=twake;
    //Writing to the ALARM register also enables the timer
    timer_hw->alarm[core]=static_cast<unsigned int>(twake & 0xffffffff);
    if(twake<=IRQgetTicks())
    {
        if(core==0) NVIC_SetPendingIRQ(TIMER_IRQ_0_IRQn);
        else NVIC_SetPendingIRQ(TIMER_IRQ_1_IRQn);
    }
}

/**
 * \internal
 * Set the current system time.
 * It is used by the kernel, and should not be used by end users.
 * Used to adjust the time for example if the system clock was stopped due to
 * entering deep sleep.
 * Can be called with interrupts disabled or within an interrupt.
 * \param ns value to set the hardware timer to. Note that the timer can
 * only be set to a higher value, never to a lower one, as the OS timer
 * needs to be monotonic.
 * If an interrupt has been set with IRQsetNextInterrupt, it needs to be
 * moved accordingly or fired immediately if the timer advance causes it
 * to be in the past.
 */
void IRQosTimerSetTime(long long ns) noexcept
{
    auto newTicks=tc.ns2tick(ns);
    timer_hw->pause=1;
    timer_hw->timelw=static_cast<unsigned int>(newTicks & 0xffffffff);
    timer_hw->timehw=static_cast<unsigned int>(newTicks>>32);
    timer_hw->pause=0;
    //Force a timer interrupt for all alarms currently configured. The timer
    //interrupt handler will check if the alarm is true or not.
    //With SMP enabled this may trigger an IRQ to another core, and this is why
    //we are not simply setting the IRQ as pending in the NVIC.
    //TODO: test me please!
    timer_hw->intf=timer_hw->armed & 0x3;
}

/**
 * \internal
 * It is used by the kernel, and should not be used by end users.
 * \return the timer frequency in Hz.
 * If a prescaler is used, it should be taken into account, the returned
 * value should be equal to the frequency at which the timer increments in
 * an observable way through IRQgetCurrentTime()
 */
unsigned int osTimerGetFrequency()
{
    return 48000000;
}

} // namespace miosix
