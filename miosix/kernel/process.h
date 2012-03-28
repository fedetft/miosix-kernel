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

#ifndef PROCESS_H
#define	PROCESS_H

#include <vector>
#include <map>
#include <sys/types.h>
#include "kernel.h"
#include "elf_program.h"
#include "config/miosix_settings.h"

#ifdef WITH_PROCESSES

namespace miosix {

/**
 * Process class, allows to create and handle processes
 */
class Process
{
public:
    /**
     * Create a new process
     * \param program Program that the process will execute
     * \return a pointer to the newly created process
     * \throws std::exception or a subclass in case of errors, including
     * not emough memory to spawn the process
     */
    static Process *create(const ElfProgram& program);
    
    /**
     * \return the pid of the created process 
     */
    pid_t getpid() const { return pid; }
    
    /**
     * \return the pid of the parent process, or zero if the process was created
     * by the kernel directly 
     */
    pid_t getppid() const { return ppid; }
    
    /**
     * Wait for child process termination
     * \param exit the process exit code will be returned here, if the pointer
     * is not null
     * \return the pid of the terminated process, or -1 in case of errors
     */
    pid_t wait(int *exit);
    
    /**
     * Wait for a specific child process to terminate
     * \param pid pid of the process, or -1 to wait for any child process
     * \param exit the process exit code will be returned here, if the pointer
     * is not null
     * \param options only 0 and WNOHANG are supported
     * \return the pid of the terminated process, or -1 in case of errors. In
     * case WNOHANG  is specified and the specified process has not terminated,
     * 0 is returned
     */
    pid_t waitpid(pid_t pid, int *exit, int options);
    
private:
    Process(const Process&);
    Process& operator= (const Process&);
    
    /**
     * Constructor
     * \param program program that will be executed by the process
     */
    Process(const ElfProgram& program);
    
    /**
     * Contains the process' main loop. 
     * \param argv ignored parameter
     * \return null
     */
    static void *start(void *argv);
    
    /**
     * \return an unique pid that is not zero and is not already in use in the
     * system, used to assign a pid to a new process.<br>
     */
    static pid_t getNewPid();
    
    ElfProgram program; ///<The program that is running inside the process
    ProcessImage image; ///<The RAM image of a process
    miosix_private::FaultData fault; ///< Contains information about faults
    std::vector<Thread *> threads; ///<Threads that belong to the process
    
    pid_t pid;  ///<The pid of this process
    pid_t ppid; ///<The parent pid of this process
    ///This union is used to join threads. When the thread to join has not yet
    ///terminated and no other thread called join it contains (Thread *)NULL,
    ///when a thread calls join on this thread it contains the thread waiting
    ///for the join, and when the thread terminated it contains (void *)result
    union
    {
        Thread *waitingForJoin;///<Thread waiting to join this
        int exitCode;          ///<Process exit code
    } joinData;
    
    ///Maps the pid to the Process instance
    static std::map<pid_t,Process *> processes;
    ///Used to assign a new pid to a process
    static pid_t pidCounter;
    ///Uset to guard access to processes and pidCounter
    static Mutex procMutex;
    ///Used to wait on process termination
    static ConditionVariable waiting;
};

} //namespace miosix

#endif //WITH_PROCESSES

#endif //PROCESS_H
