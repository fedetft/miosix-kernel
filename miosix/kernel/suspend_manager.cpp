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
#include "elf_program.h"
#include "process.h"
#include <string.h>
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
    
    ProcessStatus* currentBase=
            processesBackupBase+numSerializedProcesses*sizeof(ProcessStatus);
    numSerializedProcesses++;
    //FIXME remember to store this information in the backup RAM
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
//FIXME to be removed???
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

/*
 *The following functions is used to sort the list of resuming time
 */
bool compareResumeTime(syscallResumeTime first, syscallResumeTime second )
{
    if(first.resumeTime<second.resumeTime)
        return true;
    else
        return false;
}

/*
 *The following thread create the threds of processes at the time they myust be
 * resumed
 */
void SuspendManager::wakeupDaemon(void*)
{
    //the thread must be awaken few seconds before the resume of the next thread
    //in the list, from this comes the next attribute deltaResume
    const int deltaResume=10;
    list<syscallResumeTime>::iterator it;
    map<pid_t,Process*>:: iterator findProc;
    while(1)
    {
        for(it=syscallReturnTime.begin();it!=syscallReturnTime.end();it++)
        {
            if(it->resumeTime<=time(NULL))
            {
                findProc=Process::processes.find(it->pid);
                //check if the process is already alive...it could happen that
                //the main thread has already been spawned and is also terminated
                //so other threads waiting to be resumed must be not be created.
                //In any case, at the end of the cycle, the process must be 
                //erased from the syscallReturnTime list
                if(findProc!=Process::processes.end())
                    Process::create(it->status,it->threadNum);
                syscallReturnTime.erase(it);
                
            }
                
        }//end for
        if(!syscallReturnTime.empty())
                sleep(syscallReturnTime.begin()->resumeTime
                        -time(NULL)-deltaResume);
        else
            break; //the wakeup daemon terminates if no more threads should
                   //be awaken
    }
}

int SuspendManager::resume()
{
    ProcessStatus* proc=getProcessesBackupAreaBase();
    
    //in the following block the processes map and 
    //the list syscallReturnTime are populated
    {
        syscallResumeTime retTime;

        for(int i=0;i<=numSerializedProcesses;i++)
        {   
            
            Process::resume(ElfProgram(proc->programBase,proc->programSize),proc);
            for(int i=0;i<proc->numThreads;i++)
            {
                retTime.pid=proc->pid;
                retTime.threadNum=i;
                retTime.resumeTime=proc->interruptionPoints[i].absSyscallTime;
                retTime.status=proc;
                syscallReturnTime.push_back(retTime);
            }
            proc++;
        }//end for
        syscallReturnTime.sort(compareResumeTime);
    }//end of the block of code that populates the list of resuming time

    Thread::create(wakeupDaemon,2048);
}

}//namespace miosix

#endif //WITH_PROCESSES