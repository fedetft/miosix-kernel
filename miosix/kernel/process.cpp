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
#include "sync.h"
#include "process.h"

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

//
// class Process
//

Process *Process::create(const ElfProgram& program)
{
    auto_ptr<Process> proc(new Process(program));
    {   
        Lock<Mutex> l(procMutex);
        proc->pid=getNewPid();
        if(Thread::getCurrentThread()->proc!=0)
            proc->ppid=Thread::getCurrentThread()->proc->pid;
        else proc->ppid=0;
        processes[proc->pid]=proc.get();
    }
    Thread *thr=Thread::createUserspace(Process::start,0,Thread::DEFAULT,
        proc.get());
    if(thr==0)
    {
        Lock<Mutex> l(procMutex);
        processes.erase(proc->pid);
        throw runtime_error("Thread creation failed");
    }
    //Cannot throw bad_alloc due to the reserve in Process's constructor.
    //This ensures we will never be in the uncomfortable situation where a
    //thread has already been created but there's no memory to list it
    //among the threads of a process
    proc->threads.push_back(thr);
    thr->wakeup(); //Actually start the thread, now that everything is set up
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
    Process *proc=thr->proc;
    if(proc==0) errorHandler(UNEXPECTED);
    Thread::setupUserspaceContext(
        proc->program.getEntryPoint(),proc->image.getProcessBasePointer(),
        proc->image.getProcessBasePointer()+proc->image.getProcessImageSize());
    bool running=true;
    do {
        miosix_private::SyscallParameters sp=Thread::switchToUserspace();
        if(proc->fault.faultHappened())
        {
            running=false;
            #ifdef WITH_ERRLOG
            iprintf("Process %d terminated due to a fault\n"
                    "* Code base address was 0x%x\n"
                    "* Data base address was %p\n",proc->pid,
                    proc->program.getElfBase(),
                    proc->image.getProcessBasePointer());
            proc->fault.print();
            #endif //WITH_ERRLOG
        } else {
            switch(sp.getSyscallId())
            {
                case 2:
                    running=false;
                    iprintf("Exit %d\n",sp.getFirstParameter()); //FIXME: remove
                    break;
                case 3:
                    //FIXME: check that the pointer belongs to the process
                    sp.setReturnValue(write(sp.getFirstParameter(),
                        reinterpret_cast<const char*>(sp.getSecondParameter()),
                        sp.getThirdParameter()));
                    break;
                case 4:
                    //FIXME: check that the pointer belongs to the process
                    sp.setReturnValue(read(sp.getFirstParameter(),
                        reinterpret_cast<char*>(sp.getSecondParameter()),
                        sp.getThirdParameter()));
                    break;
                case 5: 
                    sp.setReturnValue(usleep(sp.getFirstParameter()));
                    break;
                default:
                    running=false;
                    #ifdef WITH_ERRLOG
                    iprintf("Unexpected syscall number %d\n",sp.getSyscallId());
                    #endif //WITH_ERRLOG
                    break;
            }
        }
        if(Thread::testTerminate()) running=false;
    } while(running);
    //TODO: handle process termination
    return 0;
}

pid_t Process::getNewPid()
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
Mutex Process::procMutex;
    
} //namespace miosix

#endif //WITH_PROCESSES
