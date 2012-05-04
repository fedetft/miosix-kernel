/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico  and Luigi Rucco  *
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
#ifndef BACKUP_SRAM_H
#define	BACKUP_SRAM_H

namespace miosix {

inline unsigned int *getBackupSramBase();
inline int getBackupSramSize();
void initializeBackupSram();
void initializeRTC();
/**
 * \return size of the backup SRAM for the allocator status
 *  
 */
inline int getAllocatorSramAreaSize();

/**
 * \return size of the backup SRAM for the backup allocator status
 *  
 */
inline int getBackupAllocatorSramAreaSize();


/**
 * \return size of the backup SRAM for the Processes status serialization
 *  
 */
inline int getProcessesSramAreaSize();


/**
 * \return size of the backup SRAM for network topology information
 *  
 */
inline int getRoutingTableSramAreaSize();

/**
 * \return size of the backup SRAM for the queue of the smart drivers
 *  
 */
inline int getSmartDriversQueueSramAreaSize();


} //namespace miosix

// This contains the macros and the implementation of inline functions
#include "interfaces-impl/suspend_support_impl.h"

#endif	/* BACKUP_SRAM_H */

