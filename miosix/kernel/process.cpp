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
#include "process.h"

using namespace std;

#ifdef WITH_PROCESSES

namespace miosix {

//
// class Process
//

Process *Process::create(const ElfProgram& program)
{
    //Loading the process outside the PKlock as relocation take time
    Process *proc=new Process;
    proc->image.load(program);
    {
        PauseKernelLock dLock;
        pid_t pid=PKgetNewPid();
        try {
            processes[pid]=proc;
        } catch(...) {
            delete proc;
            throw;
        }
        Thread *thr=Thread::PKcreate(Process::start,
            SYSTEM_MODE_PROCESS_STACK_SIZE,MAIN_PRIORITY,0,Thread::JOINABLE,pid,
            program.getEntryPoint(),proc->image.getProcessBasePointer());
        if(thr==0) throw runtime_error("Thread creation failed");
        //Cannot throw bad_alloc due to the reserve in Process's constructor.
        //This ensures we will never be in the uncomfortable situation where a
        //thread has already been created but there's no memory to list it
        //among the threads of a process
        proc->threads.push_back(thr);
    }
}

Process::Process()
{
    //This is required so that bad_alloc can never be thrown when the first
    //thread of the process will be stored in this vector
    threads.reserve(1);
}

void *Process::start(void *argv)
{
    //TODO
}

pid_t Process::PKgetNewPid()
{
    for(;;pidCounter++)
    {
        if(pidCounter==0) continue; //Zero is not a valid pid
        map<pid_t,Process *>::iterator it=processes.find(pidCounter);
        if(it!=processes.end()) continue; //Pid number already used
        return pidCounter++;
    }
}

map<pid_t,Process *> Process::processes;
pid_t Process::pidCounter=1;
    
} //namespace miosix

#endif //WITH_PROCESSES
