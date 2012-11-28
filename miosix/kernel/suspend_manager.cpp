/***************************************************************************
 *   Copyright (C) 2012 by Luigi Rucco and Terraneo Federico               *
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
#include "smart_sensing.h"
#include "process_pool.h"
#include <cstring>
#include <cstdio>
#include <stdexcept>

#ifdef WITH_HIBERNATION

using namespace std;

namespace miosix {

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
bool compareResumeTime(SyscallResumeTime first, SyscallResumeTime second )
{
    return first.resumeTime<second.resumeTime;
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
    SyscallResumeTime newSuspThread;
    newSuspThread.status=NULL;
    newSuspThread.pid=proc->pid;
    newSuspThread.resumeTime=resumeTime+getTick()/1000;
    newSuspThread.threadNum=threadID;
    newSuspThread.intPointID=intPointID;
    newSuspThread.fileID=fileID; 

    {
        Lock<Mutex> l(suspMutex);
        proc->numActiveThreads--;
        //FIXME: what if the system does not go in hibernation for a long time
        //because the threshold is never met? this thing continues to push data
        //in this list, causing a memory leak that will crash the whole OS!!
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

void SuspendManager::wakeUpProcess(pid_t processId){
    map<pid_t,Process*>::iterator findProc;
    Lock<Mutex> l(suspMutex);
    for(list<SyscallResumeTime>::iterator i=syscallReturnTime.begin();i!=syscallReturnTime.end();i++){
        if(i->pid==processId){
            findProc=Process::processes.find(processId);
            if(findProc!=Process::processes.end()){
                Process::create(i->status,i->threadNum,true);
            }
            syscallReturnTime.erase(i);
            return;
        }
    }
}


/*
 *The following thread create the threds of processes at the time they must be
 * resumed
 */
void SuspendManager::wakeupDaemon(void*)
{
    map<pid_t,Process*>::iterator findProc;
    Lock<Mutex> l(suspMutex);
    while(syscallReturnTime.empty()==false)
    {
        SyscallResumeTime ret=syscallReturnTime.front();        
        if(ret.resumeTime<=getTick()/1000)
        {
            findProc=Process::processes.find(ret.pid);
            //check if the process is already alive...it could happen that
            //the main thread has already been spawned and is also terminated
            //so other threads waiting to be resumed must be not be created.
            //In any case, at the end of the cycle, the process must be 
            //erased from the syscallReturnTime list
            if(findProc!=Process::processes.end())
                Process::create(ret.status,ret.threadNum);
            syscallReturnTime.pop_front();
        } else {            
            long long resumeTime=ret.resumeTime;
            
            {    
                Unlock<Mutex> u(l);
                int currentTime=resumeTime-getTick()/1000;                 
                if(currentTime>0){
                        sleep(currentTime);
                }
            }
        }
    }
}

/*
 *The following thread function wait for all the processes to be suspended and
 * then decide whether to hibernate or not..
 */
void SuspendManager::hibernateDaemon(void*)
{
    Lock<Mutex>l(suspMutex);
    for(;;)
    { 
        hibernWaiting.wait(l);
        syscallReturnTime.sort(compareResumeTime);
        list<SyscallResumeTime>::iterator it;
        it=syscallReturnTime.begin();
        //NOTE: the following if, as well as the upper and lower bunds,
        //will be replaced by the policy, once refined 
        if((it->resumeTime-getTick()/1000)<=hibernationThreshold) continue;
        ProcessStatus* proc=getProcessesBackupAreaBase();
        iprintf("Swapping %d processes\n",suspendedProcesses.size());
        list<Process*>::iterator findProc;
        for(findProc=suspendedProcesses.begin();
                findProc!=suspendedProcesses.end();findProc++)
        {
            (*findProc)->serialize(proc);

            if((*findProc)->toBeSwappedOut)
            {
                //TODO: optimize
                Mram& mram=Mram::instance();
                mram.exitSleepMode();
                //Copy the process image from RAM to MRAM
                mram.write(reinterpret_cast<unsigned int>(
                        (*findProc)->image.getProcessBasePointer())-
                        ProcessPool::instance().getBaseAddress(),
                        (*findProc)->image.getProcessBasePointer(),
                        (*findProc)->image.getProcessImageSize());
                mram.enterSleepMode();
                //Now serialize the state of the SRAM allocator
                ProcessPool::instance().serialize(getBackupSramBase());

                //Now serialize the state of the backup SRAM allocator
                (*(getBackupSramBase()+ 
                        getAllocatorSramAreaSize()/sizeof(int)))=
                        suspendedProcesses.size();
            }
            proc++;
        }
        
//        iprintf("Backup SRAM\n");
//        memDump((char*)getBackupSramBase(),getBackupSramSize());
//        Mram& mram=Mram::instance();
//        mram.exitSleepMode();
//        char *buf=new char[131072];
//        mram.read(0,buf,32768);
//        mram.read(32768,buf+32768,32768);
//        mram.read(2*32768,buf+2*32768,32768);
//        mram.read(3*32768,buf+3*32768,32768);
//        mram.enterSleepMode();
//        iprintf("MRAM\n");
//        memDump(buf,131072);
//        delete[] buf;
        
        SMART_SENSING::getSmartSensingInstance()->onSuspend(syscallReturnTime.begin()->resumeTime);
        
        
    }
}

    void SuspendManager::suspend(unsigned long long resumeTime) {
        long long prev = getTick();
        int sleepTime = resumeTime - prev / 1000;
        //iprintf("about to suspend, tick=%lld\n", prev);
        getBackupSramBase()[1021] = prev & 0xffffffff; //FIXME: hack
        getBackupSramBase()[1022] = prev >> 32;
        getBackupSramBase()[1023] = sleepTime;
        doSuspend(sleepTime);
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
        SyscallResumeTime retTime;
        int numProc=*(getBackupSramBase()+(getAllocatorSramAreaSize()/sizeof(int)));
        iprintf("Reloading %d processes\n",numProc);
        for(int i=0;i<numProc;i++)
        {   
            iprintf("reload base=%p size=%d\n",proc->programBase,proc->programSize);
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
        }
        syscallReturnTime.sort(compareResumeTime);
    }

    Thread::create(wakeupDaemon,2048);
    return proc->pid;
}

void SuspendManager::startHibernationDaemon()
{
    Thread::create(hibernateDaemon,2048);
}

std::list<SyscallResumeTime> SuspendManager::syscallReturnTime;
Mutex SuspendManager::suspMutex(Mutex::RECURSIVE);
ConditionVariable SuspendManager::hibernWaiting;
std::list<Process *> SuspendManager::suspendedProcesses;

}//namespace miosix

#endif //WITH_HIBERNATION
