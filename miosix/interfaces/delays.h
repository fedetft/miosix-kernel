/***************************************************************************
 *   Copyright (C) 2010-2024 by Terraneo Federico                          *
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

namespace miosix {

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file delays.h
 * This file contains two functions, delayMs() and delayUs() which implement
 * busy wait delays.
 *
 * NOTE: The kernel provides the possibility for a thread to sleep while leaving
 * the CPU free to use by to other threads, through the follwing functions:
 * \code
 * Thread::sleep();          //Milliseconds granularity, relative sleep time
 * Thread::nanoSleep();      //Nanosecond granularity, relative sleep time
 * Thread::nanoSleepUntil(); //Nanosecond granularity to an absolute time point
 * \endcode
 * and additionally the Miosix-specific functions mentioned above are wrapped
 * in POSIX-compliant versions (\code clock_nanosleep() \endcode) and C++
 * standard versions (\code this_thread::sleep_for() \endcode and \code
 * this_thread::sleep_until \endcode).
 *
 * The functions in this header are instead implemented using busy-wait, thus
 * they waste CPU cycles and are generally less precise as the thread can be
 * preempted, lengthening the sleep time. You should thus only use these
 * functions if you have a specific reason to do so.
 */

/**
 * Delay function. Accuracy depends on the underlying implementation which is
 * architecture specific.<br>
 * Delay time can be inaccurate if interrupts are enabled or the kernel is
 * running due to time spent in interrupts and due to preemption.<br>
 * It is implemented using busy wait, so can be safely used even when the
 * kernel is paused or interrupts are disabled.<br>
 * If the kernel is running it is *highly* recomended to use Thread::sleep()
 * since it gives CPU time to other threads and/or it puts the CPU in low power
 * mode.
 * \param mseconds milliseconds to wait
 */
void delayMs(unsigned int mseconds);

/**
 * Delay function. Accuracy depends on the underlying implementation which is
 * architecture specific.<br>
 * Delay time can be inaccurate if interrupts are enabled or the kernel is
 * running due to time spent in interrupts and due to preemption.<br>
 * It is implemented using busy wait, so can be safely used even when the
 * kernel is paused or interrupts are disabled.<br>
 * \param useconds microseconds to wait. Only values between 1 and 1000 are
 * allowed. For greater delays use Thread::sleep() or delayMs().
 */
void delayUs(unsigned int useconds);

/**
 * \}
 */

}
