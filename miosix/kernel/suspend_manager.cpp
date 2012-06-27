/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Luigi Rucco and  Terraneo Federico  *
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
#include "interfaces/suspend_support.h"
#include "elf_program.h"
#include "process.h"
#include "mram_driver/mram.h"
#include "process_pool.h"
#include <string.h>
#include <stdexcept>

#ifdef WITH_PROCESSES

using namespace std;

namespace miosix{

SuspendManager::SuspendManager() 
{
}

SuspendManager::~SuspendManager() 
{

}

ProcessStatus* SuspendManager::getProcessesBackupAreaBase()
{
    return reinterpret_cast<struct ProcessStatus*>(
                            reinterpret_cast<unsigned int>(getBackupSramBase())+
                            getAllocatorSramAreaSize()+ 
                            getBackupAllocatorSramAreaSize());
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
 *The following function is called each time a thread enter in a system call
 * in order to update the status of the thread, and eventually of the process, 
 * from 'suspended=false' to 'suspended=true', possibly. Moreover, if all the
 * processes are suspended, this function signal the hibernation thread (through
 * the condition variable), which will decide whether to hibernate or not the
 * system.
 */
void SuspendManager::enterInterruptionPoint(Process* proc, int threadID,
        long long resumeTime, int intPointID, int fileID)
{
    syscallResumeTime newSuspThread;
    newSuspThread.status=NULL;
    newSuspThread.pid=proc->pid;
    newSuspThread.resumeTime=resumeTime;
    newSuspThread.threadNum=threadID;
    newSuspThread.intPointID=intPointID;
    newSuspThread.fileID=fileID; 
    
    {
        Lock<Mutex> l(suspMutex);
        proc->numActiveThreads--;
        syscallReturnTime.push_back(newSuspThread);
        if(proc->numActiveThreads==0)
        {
            proc->suspended=true;
            suspendedProcesses.push_back(proc);
        }
        if(suspendedProcesses.size()==Process::processes.size())
            hibernWaiting.broadcast();
    }
    
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
    Lock<Mutex> l(suspMutex);
    while(1)
    {

            
        for(it=syscallReturnTime.begin();it!=syscallReturnTime.end();it++)
        {
            if(it->resumeTime<=getTick()/1000)
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
        {
            long long resumeTime=syscallReturnTime.begin()->resumeTime;
            {    
                Unlock<Mutex> u(l);
                sleep(resumeTime-getTick()/1000-deltaResume);
            }
        }
        else
            break; //the wakeup daemon terminates if no more threads should
                   //be awaken
    }
}

/*
 *The following thread function wait for all the processes to be suspended and
 * then decide whether to hibernate or not..
 */
void SuspendManager::hibernateDaemon(void*)
{
    while(1)
    {   
        Lock<Mutex>l(suspMutex);
        hibernWaiting.wait(l);
        syscallReturnTime.sort(compareResumeTime);
        list<syscallResumeTime>::iterator it;
        it=syscallReturnTime.begin();
        //NOTE: the following if, as well as the upper and lower bunds,
        //will be replaced by the policy, once refined 
        if(it->resumeTime<=lowerResumeBound)
            continue;
        else if(it->resumeTime>=upperResumeBound)
        {
            ProcessStatus* proc=getProcessesBackupAreaBase();
            list<Process*>:: iterator findProc;
            for(findProc=suspendedProcesses.begin();
                    findProc!=suspendedProcesses.end();findProc++)
            {
                (*findProc)->serialize(proc);
                
                if((*findProc)->toBeSwappedOut)
                {
                    //FIXME: check if true with Fede
                    Mram::instance().exitSleepMode();
                    //reload the image from MRAM to the  main RAM
                    Mram::instance().write(
                    reinterpret_cast<unsigned int>(
                            (*findProc)->image.getProcessBasePointer()),
                            (*findProc)->image.getProcessBasePointer(),
                            (*findProc)->image.getProcessImageSize());
                    //FIXME: check if true with Fede
                    Mram::instance().enterSleepMode();
                    //Now serialize the state of the SRAM allocator
                    ProcessPool::instance().serialize(getBackupSramBase());
                            
                    //Now serialize the state of the backup SRAM allocator
                    (*(getBackupSramBase()+ 
                            getAllocatorSramAreaSize()/sizeof(int)))=
                            suspendedProcesses.size();
                }
                proc++;
            }
            
        }
     
            
        
    }
}

int SuspendManager::resume()
{
    ProcessStatus* proc=getProcessesBackupAreaBase();
    
    ProcessPool::instance().resume(getBackupSramBase(),
            getProcessesBackupAreaBase(),
            *(getBackupSramBase()+(getAllocatorSramAreaSize()/sizeof(int))));
    //in the following block the processes map and 
    //the list syscallReturnTime are populated
    {
        Lock<Mutex>l(SuspendManager::suspMutex);
        syscallResumeTime retTime;
        for(int i=0;i<=*(getBackupSramBase()+(getAllocatorSramAreaSize()/sizeof(int)));i++)
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
    return proc->pid;
}

std::list<syscallResumeTime> SuspendManager::syscallReturnTime;
Mutex SuspendManager::suspMutex;
ConditionVariable SuspendManager::hibernWaiting;
std::list<Process *> SuspendManager::suspendedProcesses;

}//namespace miosix

#endif //WITH_PROCESSES