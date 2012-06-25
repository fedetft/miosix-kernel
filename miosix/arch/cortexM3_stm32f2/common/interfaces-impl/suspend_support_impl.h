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

#ifndef SUSPEND_SUPPORT_IMPL_H
#define	SUSPEND_SUPPORT_IMPL_H

#ifdef WITH_PROCESSES

namespace miosix {

unsigned int* const backupSramBase = 
                                reinterpret_cast<unsigned int*>(0x40024000);
const int backupSramSize=4096;

int getAllocatorSramAreaSize();

inline unsigned int *getBackupSramBase()
{
    return backupSramBase;
}

inline int getBackupSramSize()
{
    return backupSramSize;
}

inline int getBackupAllocatorSramAreaSize()
{
    return sizeof(unsigned int*)*4 ;
}

inline int getProcessesSramAreaSize()
{
    return 1024;
}

inline int getRoutingTableSramAreaSize()
{
    return 1024;
}

inline int getSmartDriversQueueSramAreaSize()
{
    return getBackupSramSize()-
           getAllocatorSramAreaSize() -
           getProcessesSramAreaSize() -
           getSmartDriversQueueSramAreaSize();
}

} //namespace miosix

#endif  //WITH_PROCESSES
#endif	//SUSPEND_SUPPORT_IMPL_H
