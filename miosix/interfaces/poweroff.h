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

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file poweroff.h
 * This file must provide these functions:
 *
 * shutdown(), for system shutdown. This function is called in case main()
 * returns, and is available to be called by user code.
 *
 * reboot(), a function that can be called to reboot the system under normal
 * (non error) conditions. It should sync and unmount the filesystem, and
 * perform a reboot. This function is available for user code.
 *
 * IRQsystemReboot(), a low-level function to reboot the system in case of an
 * unrecoverablle error. You should probably not use it unless really needed
 * as it does not cleanly unmount the filesystem and could thus lead to data
 * loss.
 */

namespace miosix {

/**
 * This function should flush the default console with
 * \code
 * ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
 * \endcode
 * close all files and unmount all filesystems by caling
 * \code
 * FilesystemManager::instance().umountAll()
 * \endcode
 * and finally shut down the system, usually by putting the procesor in a deep
 * sleep state until some board specific condition occurs (which could even
 * be a processor reset or powercycle).
 *
 * This function does not return.
 *
 * For architecture where it does not make sense to perform a shutdown, or for
 * safety reasons it is not advisable to enter a state where execution stops,
 * it is suggested to implement this function by performing a reboot instead.
 */
[[noreturn]] void shutdown();

/**
 * This function should flush the default console with
 * \code
 * ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
 * \endcode
 * close all files and unmount all filesystems by caling
 * \code
 * FilesystemManager::instance().umountAll()
 * \endcode
 * and finally reboot the system.
 */
[[noreturn]] void reboot();

/**
 * Used after an unrecoverable error condition to restart the system, even from
 * within an interrupt routine.
 * WARNING: this function does not close files nor unmount filesystems, so using
 * it could lead to data corruption.
 */
[[noreturn]] void IRQsystemReboot();

} //namespace miosix

/**
 * \}
 */
