/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

#include <stdexcept>
#include <memory>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>

#include "sync.h"
#include "process_pool.h"
#include "process.h"
#include "SystemMap.h"

using namespace std;

#ifdef WITH_PROCESSES

/*
 * List of implemented supervisor calls
 * ------------------------------------
 * 
 * 0 : Yield. Can be called both by kernel threads and process threads both in
 *     userspace and kernelspace mode. It causes the scheduler to switch to
 *     another thread. It is the only SVC that is available also when processes
 *     are disabled in miosix_config.h. No parameters, no return value.
 * 1 : Back to userspace. It is used by process threads running in kernelspace
 *     mode to return to userspace mode after completing a supervisor call.
 *     If called by a process thread already in userspace mode it does nothing.
 *     Use of this SVC is by kernel threads is forbidden. No parameters, no
 *     return value.
 * 2 : Exit. Terminates the current process. One parameter, the exit code.
 *     Never returns. Use of this SVC is by kernel threads is forbidden.
 * 3 : Write. Writes to stdout or a file. Three parameters, file descriptor,
 *     pointer to data to be written, size of data. Returns the number of
 *     written data or -1 on error. Use of this SVC is by kernel threads is
 *     forbidden.
 * 4 : Read. Reads from stdin or a file. Three parameters, file descriptor,
 *     pointer to data buffer, size of buffer. Returns the number of
 *     read data or -1 on error. Use of this SVC is by kernel threads is
 *     forbidden.
 * 5 : Usleep. One parameter, number of microseconds to sleep. Returns 0 on
 *     success, -1 on failure. Use of this SVC is by kernel threads isforbidden.
 */

namespace miosix {

/**
 * This class contains information on all the processes in the system
 */
class Processes
{
public:
    /**
     * \return the instance of this class (singleton)
     */
    static Processes& instance();
    
    ///Used to assign a new pid to a process
    pid_t pidCounter;
    ///Maps the pid to the Process instance. Includes zombie processes
    map<pid_t,ProcessBase *> processes;
    ///Uset to guard access to processes and pidCounter
    Mutex procMutex;
    ///Used to wait on process termination
    ConditionVariable genericWaiting;
    
private:
    Processes()
    {
        ProcessBase *kernel=Thread::getCurrentThread()->getProcess();
        assert(kernel->getPid()==0);
        processes[0]=kernel;
    }
    Processes(const Processes&);
    Processes& operator=(const Processes&);
};

Processes& Processes::instance()
{
    static Processes singleton;
    return singleton;
}

//
// class Process
//

pid_t Process::create(const ElfProgram& program)
{
    Processes& p=Processes::instance();
    ProcessBase *parent=Thread::getCurrentThread()->proc;
    auto_ptr<Process> proc(new Process(program));
    {   
        Lock<Mutex> l(p.procMutex);
        proc->pid=getNewPid();
        proc->ppid=parent->pid;
        parent->childs.push_back(proc.get());
        p.processes[proc->pid]=proc.get();
    }
    Thread *thr;
    thr=Thread::createUserspace(Process::start,0,Thread::DEFAULT,proc.get());
    if(thr==0)
    {
        Lock<Mutex> l(p.procMutex);
        p.processes.erase(proc->pid);
        parent->childs.remove(proc.get());
        throw runtime_error("Thread creation failed");
    }
    //Cannot throw bad_alloc due to the reserve in Process's constructor.
    //This ensures we will never be in the uncomfortable situation where a
    //thread has already been created but there's no memory to list it
    //among the threads of a process
    proc->threads.push_back(thr);
    thr->wakeup(); //Actually start the thread, now that everything is set up
    pid_t result=proc->pid;
    proc.release(); //Do not delete the pointer
    return result;
}

pid_t Process::getppid(pid_t proc)
{
    Processes& p=Processes::instance();
    Lock<Mutex> l(p.procMutex);
    map<pid_t,ProcessBase *>::iterator it=p.processes.find(proc);
    if(it==p.processes.end()) return -1;
    return it->second->ppid;
}

pid_t Process::waitpid(pid_t pid, int* exit, int options)
{
    Processes& p=Processes::instance();
    Lock<Mutex> l(p.procMutex);
    ProcessBase *self=Thread::getCurrentThread()->proc;
    if(pid<=0)
    {
        //Wait for a generic child process
        if(self->zombies.empty() && (options & WNOHANG)) return 0;
        while(self->zombies.empty())
        {
            if(self->childs.empty()) return -1;
            p.genericWaiting.wait(l);
        }
        Process *joined=self->zombies.front();
        self->zombies.pop_front();
        p.processes.erase(joined->pid);
        if(joined->waitCount!=0) errorHandler(UNEXPECTED);
        if(exit!=0) *exit=joined->exitCode;
        pid_t result=joined->pid;
        delete joined;
        return result;
    } else {
        //Wait on a specific child process
        map<pid_t,ProcessBase *>::iterator it=p.processes.find(pid);
        if(it==p.processes.end() || it->second->ppid!=self->pid
                || pid==self->pid) return -1;
        //Since the case when pid==0 has been singled out, this cast is safe
        Process *joined=static_cast<Process*>(it->second);
        if(joined->zombie==false)
        {
            //Process hasn't terminated yet
            if(options & WNOHANG) return 0;
            joined->waitCount++;
            joined->waiting.wait(l);
            joined->waitCount--;
            if(joined->waitCount<0 || joined->zombie==false)
                errorHandler(UNEXPECTED);
        }
        pid_t result=-1;
        if(joined->waitCount==0)
        {
            result=joined->pid;
            if(exit!=0) *exit=joined->exitCode;
            self->zombies.remove(joined);
            p.processes.erase(joined->pid);
            delete joined;
        }
        return result;
    }
}

Process::~Process()
{
    #ifdef __CODE_IN_XRAM
    ProcessPool::instance().deallocate(loadedProgram);
    #endif //__CODE_IN_XRAM
	
	//close alla opened files
	for(set<int>::iterator it = mFiles.begin(); it != mFiles.end(); ++it)
		close(*it);
}

Process::Process(const ElfProgram& program) : program(program), waitCount(0),
        zombie(false)
{
    //This is required so that bad_alloc can never be thrown when the first
    //thread of the process will be stored in this vector
    threads.reserve(1);
    //Done here so if not enough memory the new process is not even created
    image.load(program);
    unsigned int elfSize=program.getElfSize();
    unsigned int roundedSize=elfSize;
    //Allocatable blocks must be greater than ProcessPool::blockSize, and must
    //be a power of two due to MPU limitations
    if(elfSize<ProcessPool::blockSize) roundedSize=ProcessPool::blockSize;
    else if(elfSize & (elfSize-1)) roundedSize=1<<ffs(elfSize);
    #ifndef __CODE_IN_XRAM
    //FIXME -- begin
    //Till a flash file system that ensures proper alignment of the programs
    //loaded in flash is implemented, make the whole flash visible as a big MPU
    //region
    extern unsigned char _end asm("_end");
    unsigned int flashEnd=reinterpret_cast<unsigned int>(&_end);
    if(flashEnd & (flashEnd-1)) flashEnd=1<<ffs(flashEnd);
    mpu=miosix_private::MPUConfiguration(0,flashEnd,
            image.getProcessBasePointer(),image.getProcessImageSize());
//    mpu=miosix_private::MPUConfiguration(program.getElfBase(),roundedSize,
//            image.getProcessBasePointer(),image.getProcessImageSize());
    //FIXME -- end
    #else //__CODE_IN_XRAM
    loadedProgram=ProcessPool::instance().allocate(roundedSize);
    memcpy(loadedProgram,reinterpret_cast<char*>(program.getElfBase()),elfSize);
    mpu=miosix_private::MPUConfiguration(loadedProgram,roundedSize,
            image.getProcessBasePointer(),image.getProcessImageSize());
    #endif //__CODE_IN_XRAM
}

void *Process::start(void *argv)
{
    //This function is never called with a kernel thread, so the cast is safe
    Process *proc=static_cast<Process*>(Thread::getCurrentThread()->proc);
    if(proc==0) errorHandler(UNEXPECTED);
    unsigned int entry=proc->program.getEntryPoint();
    #ifdef __CODE_IN_XRAM
    entry=entry-proc->program.getElfBase()+
        reinterpret_cast<unsigned int>(proc->loadedProgram);
    #endif //__CODE_IN_XRAM
    Thread::setupUserspaceContext(entry,proc->image.getProcessBasePointer(),
        proc->image.getProcessImageSize());
    bool running=true;
    do {
        miosix_private::SyscallParameters sp=Thread::switchToUserspace();
        if(proc->fault.faultHappened())
        {
            running=false;
            proc->exitCode=SIGSEGV; //Segfault
            #ifdef WITH_ERRLOG
            iprintf("Process %d terminated due to a fault\n"
                    "* Code base address was 0x%x\n"
                    "* Data base address was %p\n",proc->pid,
                    #ifndef __CODE_IN_XRAM
                    proc->program.getElfBase(),
                    #else //__CODE_IN_XRAM
                    reinterpret_cast<unsigned int>(proc->loadedProgram),
                    #endif //__CODE_IN_XRAM
                    proc->image.getProcessBasePointer());
            proc->mpu.dumpConfiguration();
            proc->fault.print();
            #endif //WITH_ERRLOG
        } else {
            switch(sp.getSyscallId())
            {
                case 2:
                    running=false;
                    proc->exitCode=(sp.getFirstParameter() & 0xff)<<8;
                    break;
                case 3:
                    //FIXME: check that the pointer belongs to the process
					
					if((sp.getFirstParameter() >= 0 && sp.getFirstParameter() < 3) ||		//stdio
						proc->mFiles.find(sp.getFirstParameter()) != proc->mFiles.end()){	//only on my files
						
						//check if pointer and pointer + size is within the mpu data region
						if(!proc->mpu.within(sp.getSecondParameter()) ||
						   !proc->mpu.within(sp.getSecondParameter() + sp.getThirdParameter())){
							running = false;
							proc->exitCode = SIGSYS;
							break;
						}
						
						sp.setReturnValue(write(sp.getFirstParameter(),
												reinterpret_cast<const char*>(sp.getSecondParameter()),
												sp.getThirdParameter()));
					} else 
						sp.setReturnValue(-1);
					
                    break;
                case 4:					
					if((sp.getFirstParameter() >= 0 && sp.getFirstParameter() < 3) ||		//stdio
						proc->mFiles.find(sp.getFirstParameter())!= proc->mFiles.end()){	//only on my files
                    
						//check if pointer and pointer + size is within the mpu data region
						if(!proc->mpu.within(sp.getSecondParameter()) ||
						   !proc->mpu.within(sp.getSecondParameter() + sp.getThirdParameter())){
							running = false;
							proc->exitCode = SIGSYS;
							break;
						}
							
						sp.setReturnValue(read(sp.getFirstParameter(),
											   reinterpret_cast<char*>(sp.getSecondParameter()),
											   sp.getThirdParameter()));
					} else
						sp.setReturnValue(-1);
					
                    break;
                case 5:
					sp.setReturnValue(usleep(sp.getFirstParameter()));
                    break;
				#ifdef WITH_FILESYSTEM
				case 6:
					//open
				{
					unsigned int base = proc->mpu.getBaseDataAddress();
					unsigned int size = proc->mpu.getDataSize();
					
					unsigned int maxLen = min(base + size - sp.getFirstParameter(), (unsigned int)PATH_MAX);
					size_t filenameLen = strnlen(reinterpret_cast<const char*>(sp.getFirstParameter()), maxLen);
					
					if(!proc->mpu.within(sp.getFirstParameter()) ||
					   !proc->mpu.within(sp.getFirstParameter() + filenameLen)){
						
						proc->mpu.dumpConfiguration();	
						
						running = false;
						proc->exitCode = SIGSYS;
						break;
					}

					int fd = open(reinterpret_cast<const char*>(sp.getFirstParameter()),	//filename
								  sp.getSecondParameter(),									//flags
								  sp.getThirdParameter());									//permission, used?
					if (fd != -1)
						proc->mFiles.insert(fd);
						
					sp.setReturnValue(fd);
				}
                    break;
				case 7:
					//close
					if(proc->mFiles.find(sp.getFirstParameter()) != proc->mFiles.end()){
						int ret = close(sp.getFirstParameter());
						
						if(ret == 0)
							proc->mFiles.erase(sp.getFirstParameter());
						
						sp.setReturnValue(ret);
					} else 
						sp.setReturnValue(-1);
					break;
				case 8:
					//seek
					if(proc->mFiles.find(sp.getFirstParameter()) != proc->mFiles.end())
						sp.setReturnValue(lseek(sp.getFirstParameter(),
								                sp.getSecondParameter(),
					  						    sp.getThirdParameter()));
					else
						sp.setReturnValue(-1);
					
					break;
				#endif //WITH_FILESYSTEM
				case 9:
					//system
				{				
					std::pair<const unsigned int*, unsigned int> res = SystemMap::instance().getElfProgram(reinterpret_cast<const char*>(sp.getFirstParameter()));
							
					if(res.first == 0 || res.second == 0){
						iprintf("Program not found.\n");
						sp.setReturnValue(-1);
						break;
					}
								
					ElfProgram program(res.first, res.second);
					int ret = 0;
					
					pid_t child = Process::create(program);
					Process::waitpid(child, &ret, 0);
					
					sp.setReturnValue(WEXITSTATUS(ret));
				}
					break;
                default:
                    running=false;
                    proc->exitCode=SIGSYS; //Bad syscall
                    #ifdef WITH_ERRLOG
                    iprintf("Unexpected syscall number %d\n",sp.getSyscallId());
                    #endif //WITH_ERRLOG
                    break;
            }
        }
        if(Thread::testTerminate()) running=false;
    } while(running);
    {
        Processes& p=Processes::instance();
        Lock<Mutex> l(p.procMutex);
        proc->zombie=true;
        list<Process*>::iterator it;
        for(it=proc->childs.begin();it!=proc->childs.end();++it) (*it)->ppid=0;
        for(it=proc->zombies.begin();it!=proc->zombies.end();++it) (*it)->ppid=0;
        ProcessBase *kernel=p.processes[0];
        kernel->childs.splice(kernel->childs.begin(),proc->childs);
        kernel->zombies.splice(kernel->zombies.begin(),proc->zombies);
        
        map<pid_t,ProcessBase *>::iterator it2=p.processes.find(proc->ppid);
        if(it2==p.processes.end()) errorHandler(UNEXPECTED);
        it2->second->childs.remove(proc);
        if(proc->waitCount>0) proc->waiting.broadcast();
        else {
            it2->second->zombies.push_back(proc);
            p.genericWaiting.broadcast();
        }
    }
    return 0;
}

pid_t Process::getNewPid()
{
    Processes& p=Processes::instance();
    for(;;p.pidCounter++)
    {
        if(p.pidCounter<0) p.pidCounter=1;
        if(p.pidCounter==0) continue; //Zero is not a valid pid
        map<pid_t,ProcessBase*>::iterator it=p.processes.find(p.pidCounter);
        if(it!=p.processes.end()) continue; //Pid number already used
        return p.pidCounter++;
    }
}

} //namespace miosix

#endif //WITH_PROCESSES
