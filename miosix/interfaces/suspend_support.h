/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico and Luigi Rucco   *
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
#ifndef SUSPEND_SUPPORT_H
#define	SUSPEND_SUPPORT_H

#ifdef WITH_HIBERNATION

namespace miosix {

/**
 * \return a pointer to the beginning of the backup SRAM area, that preserves
 * its content through hibernation
 */
inline unsigned int *getBackupSramBase();

/**
 * \return the size of the backup SRAM
 */
inline int getBackupSramSize();

/**
 * Needs to be called from IRQbspInit() if hibernation support is enabled.
 * None of the functions in this haeder can be called prior to this, as they
 * might access uninitialized resources
 */
void IRQinitializeSuspendSupport();

/**
 * Enter hibernation mode for the specified number of seconds
 * \param seconds after how much time the system will reboot
 */
void doSuspend(unsigned int seconds);

/**
 * \return true if this is the first boot, i.e. a boot caused by a reset
 * assertion or power on. If returns false, this is a boot caused by an RTC
 * wakeup, and in this case swapped processes need to be reloaded.
 */
bool firstBoot();

/**
 * \return size of the backup SRAM for the allocator status
 *  
 */
int getAllocatorSramAreaSize();

/**
 * \return size of the backup SRAM for the backup allocator status
 *  this includes four pointers, i.e. the base pointers to the four 
 * region of the backup memory
 */
inline int getBackupAllocatorSramAreaSize();

/**
 * \return size of the backup SRAM for the Processes status serialization 
 */
inline int getProcessesSramAreaSize();

/**
 * \return size of the backup SRAM for network topology information 
 */
inline int getRoutingTableSramAreaSize();

/**
 * \return size of the backup SRAM for the queue of the smart drivers
 */
inline int getSmartDriversQueueSramAreaSize();

} //namespace miosix

// This contains the macros and the implementation of inline functions
#include "interfaces-impl/suspend_support_impl.h"

#endif  //WITH_HIBERNATION
#endif	//SUSPEND_SUPPORT_H
