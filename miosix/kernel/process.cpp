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
#include "process.h"

using namespace std;

#ifdef WITH_PROCESSES

namespace miosix {

//
// class Process
//

Process *Process::create(const ElfProgram& program)
{
    //Loading the process outside the PKlock as relocation takes time
    auto_ptr<Process> proc(new Process(program));
    {
        PauseKernelLock dLock;
        pid_t pid=PKgetNewPid();
        processes[pid]=proc.get();
        Thread *thr=Thread::PKcreateUserspace(Process::start,0,
            Thread::JOINABLE,pid);
        if(thr==0)
        {
            processes.erase(pid);
            throw runtime_error("Thread creation failed");
        }
        //Cannot throw bad_alloc due to the reserve in Process's constructor.
        //This ensures we will never be in the uncomfortable situation where a
        //thread has already been created but there's no memory to list it
        //among the threads of a process
        proc->threads.push_back(thr);
    }
    #ifdef SCHED_TYPE_EDF
    //The new thread might have a closer deadline
    if(isKernelRunning()) Thread::yield();
    #endif //SCHED_TYPE_EDF
    return proc.release(); //Do not delete the pointer
}

Process::Process(const ElfProgram& program) : program(program)
{
    //This is required so that bad_alloc can never be thrown when the first
    //thread of the process will be stored in this vector
    threads.reserve(1);
    //Done here so if not enough memory the new process is not even created
    image.load(program);
}

void *Process::start(void *argv)
{
    Thread *thr=Thread::getCurrentThread();
    map<pid_t,Process*>::iterator it=processes.find(thr->pid);
    if(it==processes.end()) errorHandler(UNEXPECTED);
    Process *proc=it->second;
    Thread::setupUserspaceContext(
        proc->program.getEntryPoint(),proc->image.getProcessBasePointer(),
        proc->image.getProcessBasePointer()+proc->image.getProcessImageSize());
    bool running=true;
    do {
        miosix_private::SyscallParameters sp=Thread::switchToUserspace();
        if(sp.isValid())
        {
            switch(sp.getSyscallId())
            {
                case 1:
                    iprintf("Exit %d\n",sp.getFirstParameter());
                    running=false;
                    break;
                case 2:
                    //FIXME: check that the pointer belongs to the process
                    sp.setReturnValue(write(sp.getFirstParameter(),
                        reinterpret_cast<const char*>(sp.getSecondParameter()),
                        sp.getThirdParameter()));
                    break;
                default:
                    iprintf("Unexpected invalid syscall\n");
                    running=false;
                    break;
            }
        }
        if(Thread::testTerminate())
        {
            running=false;
        }
    } while(running);
    //TODO: handle process termination
    return 0;
}

pid_t Process::PKgetNewPid()
{
    for(;;pidCounter++)
    {
        if(pidCounter<0) pidCounter=1;
        if(pidCounter==0) continue; //Zero is not a valid pid
        map<pid_t,Process*>::iterator it=processes.find(pidCounter);
        if(it!=processes.end()) continue; //Pid number already used
        return pidCounter++;
    }
}

map<pid_t,Process*> Process::processes;
pid_t Process::pidCounter=1;
    
} //namespace miosix

#endif //WITH_PROCESSES
