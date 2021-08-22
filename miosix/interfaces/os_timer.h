/***************************************************************************
 *   Copyright (C) 2015-2021 by Terraneo Federico, Sasan Golchin           *
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

#pragma once

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file os_timer.h
 * This file contains the interface through which the OS accesses the underlying
 * hardware timer, that is used to:
 * - measure time durations
 * - set interrupts used both preemption and to handle sleeping threads wakeup
 *
 * Please note that all functions in this interface should provide the kernel
 * with time information in nanoseconds. In the platform-specific implementation
 * it is highly recommended to use the TimeConversion class to convert the
 * underlying hardware timer ticks to nanoseconds.
 */

namespace miosix {

// The os timer platform-specific implementation shall provide these two
// functions, although they are not declared here, but in kernel.h, as
// these are the only two function that are meant to be called also from
// application code. For comments about the intended behavior, see kernel.h
//long long getTime();
//long long IRQgetTime();
    
namespace internal {

/**
 * \internal
 * Initialize and start the os timer.
 * It is used by the kernel, and should not be used by end users.
 */
void IRQosTimerInit();

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
void IRQosTimerSetInterrupt(long long ns);

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
void IRQosTimerSetTime(long long ns);

/**
 * \internal
 * It is used by the kernel, and should not be used by end users.
 * \return the timer frequency in Hz.
 * If a prescaler is used, it should be taken into account, the returned
 * value should be equal to the frequency at which the timer increments in
 * an observable way through IRQgetCurrentTime()
 */
unsigned int osTimerGetFrequency();

} //namespace internal

} //namespace miosix

/**
 * \}
 */
