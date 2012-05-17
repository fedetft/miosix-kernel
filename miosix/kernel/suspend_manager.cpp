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

#include "suspend_manager.h"
#include <stdexcept>

#ifdef WITH_PROCESSES

using namespace std;

namespace miosix{

SuspendManager::SuspendManager() 
{
    numSerializedProcesses=0;
}

SuspendManager::~SuspendManager() 
{

}

ProcessStatus* SuspendManager:: getProcessesBackupAreaPtr()
{
    ProcessStatus* processesBackupBase= 
                                reinterpret_cast<struct ProcessStatus*>(
                                getProcessesBackupAreaBase());
    
    ProcessStatus* currentBase=processesBackupBase+numSerializedProcesses;
    numSerializedProcesses++;
    return currentBase;
}

void SuspendManager::setInvalidBitToSerializedProcess(int pid)
{
    int visited=0;
    struct ProcessStatus* processesBackupBase= 
                                reinterpret_cast<struct ProcessStatus*>(
                                getProcessesBackupAreaBase());
    
    while(processesBackupBase->pid!=pid && visited <= numSerializedProcesses)
    {
        processesBackupBase++;
        visited++;
    }
        
    
    if(processesBackupBase->pid==pid && visited <= numSerializedProcesses)
    {
        processesBackupBase->status|=1<<1;
    }else{
       throw runtime_error("Error: no serialized process found in backup SRAM");
    }
        
    
    
}

int SuspendManager::findFirstInvalidInSerializedProcess()
{

    int visited=0;
    struct ProcessStatus* processesBackupBase= 
                                reinterpret_cast<struct ProcessStatus*>(
                                getProcessesBackupAreaBase());
    
    while((processesBackupBase->status &((1<<1)==0) && 
          visited <= numSerializedProcesses))
    {
        processesBackupBase++;
        visited++;
    }
    
     if((processesBackupBase->status & ((1<<1)!=0)) &&
        visited <= numSerializedProcesses)
     {
         return visited;
    }else{
         return -1;
    }
    
}

int SuspendManager::resume()
{
    
    return 0;
}

}//namespace miosix

#endif //WITH_PROCESSES