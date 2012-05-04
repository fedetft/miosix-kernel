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

#ifndef SUSPENDMANAGER_H
#define	SUSPENDMANAGER_H
#include "interfaces/portability.h"
#include "interfaces/suspend_support.h"

namespace miosix{

///This struct is used to serialize the interrupion point status for processes
///and threads spawned by that process
struct InterruptionPointStatus{
    int interruptionPointID;
    int status[8];
}__attribute__((packed));
    
    
///This struct is used to serialize the process status in the backup SRAM 
struct ProcessStatus 
{
    int pid;
    int ppid;
    short int status;
    short int numThreads;
    int exitCode;
    unsigned int* processImageBase;
    int processImageSize;
    unsigned int* programBase;
    int programSize;
    unsigned int MPUregValues[miosix_private::MPUConfiguration::numRegisters];
    int fileDescriptors[MAX_OPEN_FILES];
    struct InterruptionPointStatus InterruptionPoints[1+MAX_THREADS_PER_PROCESS];
} __attribute__((packed));




class SuspendManager 
{
public:
    SuspendManager();
    virtual ~SuspendManager();
    
    /**
     * /return the base address of the process status backup area 
     */
    ProcessStatus* getProcessesBackupAreaBase()
    {
        return    reinterpret_cast<struct ProcessStatus*>(
                                reinterpret_cast<unsigned int>(backupSramBase)+
                                getAllocatorSramAreaSize()+ 
                                getBackupAllocatorSramAreaSize());
    }
    
    /**
     * /return the ptr to allocate the next process status in the backup area 
     */
    ProcessStatus* getProcessesBackupAreaPtr();
    
    /**
     * Reset to zero the number of serialized processes
     */
    void resetNumSerializedProcesses()
    {
        numSerializedProcesses=0;
    }
    
    /**
     * Set the dirty bit for a process that must be serialized again
     */
    void setInvalidBitToSerializedProcess(int pid);
    
    /**
     * Find the the serialized processes with invalid bit set,
     * which must be serialzied again. Return -1 if no dirty processes status
     * are found
     */
    int findFirstInvalidInSerializedProcess();

private:
    unsigned int* backupSramBase;
    int numSerializedProcesses;
};

}//namespace miosix
#endif	/* SUSPENDMANAGER_H */

