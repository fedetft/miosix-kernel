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

#include "kernel/kernel.h"
#include "interfaces/delays.h"
#include "interfaces/os_timer.h"
#include "interfaces/arch_registers.h"

namespace miosix {

namespace internal {

static TimeConversion tc(48000000);
static uint64_t lastAlarmTicks;

/**
 * \internal
 * Get raw tick count from the timer.
 * \returns the current tick count.
 */
static inline uint64_t IRQgetTicks() noexcept
{
    //Latching reads: order matters!
    uint32_t lo = timer_hw->timelr;
    uint32_t hi = timer_hw->timehr;
    return (uint64_t)lo + ((uint64_t)hi << 32);
}

/**
 * \internal
 * Initialize and start the os timer.
 * It is used by the kernel, and should not be used by end users.
 */
void IRQosTimerInit()
{
    //Bring timer out of reset
    resets_hw->reset = resets_hw->reset & ~RESETS_RESET_TIMER_BITS;
    while (~resets_hw->reset_done & RESETS_RESET_TIMER_BITS) {}
    //Enable timer interrupt
    //Timer IRQ saves context: its priority must be 3 (see portability.cpp)
    NVIC_SetPriority(TIMER_IRQ_0_IRQn, 3);
    NVIC_EnableIRQ(TIMER_IRQ_0_IRQn);
    timer_hw->inte = TIMER_INTE_ALARM_0_BITS;
    //Toggle debug sleep mode. Works around a bug where the timer does not
    //start counting if it was reset while it was paused due to debug mode.
    timer_hw->dbgpause = 0;
    delayUs(1);
    timer_hw->dbgpause = 3;
}

/**
 * \internal
 * Set the next interrupt.
 * It is used by the kernel, and should not be used by end users.
 * Can be called with interrupts disabled or within an interrupt.
 * The hardware timer handles only one outstading interrupt request at a
 * time, so a new call before the interrupt expires cancels the previous one.
 * \param ns the absolute time when the interrupt will be fired, in nanoseconds.
 * When the interrupt fires, it shall call the
 * \code
 * void IRQtimerInterrupt(long long currentTime);
 * \endcode
 * function defined in kernel/scheduler/timer_interrupt.h
 */
void IRQosTimerSetInterrupt(long long ns) noexcept
{
    uint64_t lastAlarmTicks = (uint64_t)tc.ns2tick(ns);
    //Writing to the ALARM register also enables the timer
    timer_hw->alarm[0] = (uint32_t)lastAlarmTicks;
    if(IRQgetTicks() >= lastAlarmTicks) NVIC_SetPendingIRQ(TIMER_IRQ_0_IRQn);
}

/**
 * \internal
 * Handles the timer interrupt, checking if the alarm period is indeed
 * elapsed and calling the kernel if so.
 */
void IRQtimerInterruptHandler()
{
    //Clear the timer IRQ (register is write clear (WC)!)
    timer_hw->intr = 1;
    uint64_t t = IRQgetTicks();
    //Check the full 64 bits. If the alarm deadline has passed, call the kernel.
    //Otherwise rearm the timer and try again.
    if (t >= lastAlarmTicks) miosix::IRQtimerInterrupt(tc.tick2ns(t));
    else timer_hw->armed = 1;
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
    uint64_t newTicks = (uint64_t)tc.ns2tick(ns);
    timer_hw->pause = 1;
    timer_hw->timelw = (uint32_t)newTicks;
    timer_hw->timehw = (uint32_t)(newTicks >> 32);
    //Check if the time is advancing past the last alarm deadline set
    if ((timer_hw->armed & 1) && lastAlarmTicks >= newTicks)
    {
        timer_hw->armed = 0;
        NVIC_SetPendingIRQ(TIMER_IRQ_0_IRQn);
    }
    timer_hw->pause = 0;
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

} // namespace internal

long long getTime() noexcept
{
    FastInterruptDisableLock dLock;
    return IRQgetTime();
}

long long IRQgetTime() noexcept
{
    long long ticks = (long long)internal::IRQgetTicks();
    return internal::tc.tick2ns(ticks);
}

} // namespace miosix

void __attribute__((naked)) TIMER_IRQ_0_Handler()
{
    saveContext();
    asm volatile("bl %a0"::"i"(miosix::internal::IRQtimerInterruptHandler):);
    restoreContext();
}
