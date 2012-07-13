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
#include <list>
#include <sys/types.h>
#include "kernel.h"
#include "sync.h"
#include "elf_program.h"
#include "config/miosix_settings.h"
#include "suspend_manager.h"

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
     * \return the pid of the newly created process
     * \throws std::exception or a subclass in case of errors, including
     * not emough memory to spawn the process
     */
    static pid_t create(const ElfProgram& program);
    
    /**
     * Overloaded version that recreate a process after hibernation
     * \param status is the serialized status of the process
     * \param threadID is the number of the thread to be resumed
     * \return the same pid of the original process
     * \throws std::exception or a subclass in case of errors, including
     * not emough memory to spawn the process
     */
    static pid_t create(ProcessStatus* status, int threadId);
    
    
    /**
     * Recreate a process after the hibernation, exactly as it was before
     * \param program Program that the process will execute 
     * \param ptr to the serialized status of the process in the backup RAM
     * \return the pid of the newly created process
     * \throws std::exception or a subclass in case of errors, including
     * not emough memory to spawn the process
     */
    static pid_t resume(const ElfProgram& program, ProcessStatus* status);
    
    /**
     * Given a process, returns the pid of its parent.
     * \param proc the pid of a process
     * \return the pid of the parent process, or zero if the process was created
     * by the kernel directly, or -1 if proc is not a valid process
     */
    static pid_t getppid(pid_t proc);
    
    /**
     * Wait for child process termination
     * \param exit the process exit code will be returned here, if the pointer
     * is not null
     * \return the pid of the terminated process, or -1 in case of errors
     */
    static pid_t wait(int *exit) { return waitpid(-1,exit,0); }
    
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
    static pid_t waitpid(pid_t pid, int *exit, int options);
    
    /**
     * Save the state of the allocator 
     * \param ptr pointer to a memory area of type ProcessStatus
     * \param interruptionId id of the syscal that caused the interruption
     * \param fileID file opened by the syscall
     * \param sleepTime time to sleep for the syscall, actual or estimed,
     * the latter case accounts for all the syscalls that are not sleep()
     * \param sampleBuf pointer to the area of memore where the results of
     * the syscall will be copied. Usefuls in case of smart drivers 
     */
    void serialize(ProcessStatus* ptr);
    
    
    
    /**
     * Destructor
     */
    ~Process();
    
private:
    Process(const Process&);
    Process& operator= (const Process&);
    
    /**
     * Constructor
     * \param program program that will be executed by the process
     * \param resuming true if resuming from hibernation
     */
    Process(const ElfProgram& program, bool resuming=false);
    

    
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
    
    ElfProgram* program; ///<The program that is running inside the process
    #ifdef __CODE_IN_XRAM
    /// When __CODE_IN_XRAM is defined, the programs are loaded in the process
    /// pool so the memory is aligned and the MPU works
    unsigned int *loadedProgram;
    #endif //__CODE_IN_XRAM
    ProcessImage image; ///<The RAM image of a process
    miosix_private::FaultData fault; ///< Contains information about faults
    miosix_private::MPUConfiguration mpu; ///<Memory protection data
    std::vector<Thread *> threads; ///<Threads that belong to the process
    std::list<Process *> childs;   ///<Living child processes are stored here
    std::list<Process *> zombies;  ///<Dead child processes are stored here
    pid_t pid;  ///<The pid of this process
    pid_t ppid; ///<The parent pid of this process
    ///number of the threads active, i.e. not suspended, in the process
    int numActiveThreads; 
    ///Contains the count of active wait calls which specifically requested
    ///to wait on this process
    int waitCount;
    ///Active wait calls which specifically requested to wait on this process
    ///wait on this condition variable
    ConditionVariable waiting;
    bool zombie; ///< True for terminated not yet joined processes
    bool suspended;
    bool toBeSwappedOut;///This is set true only if a processs need to be
                        ///swapped out when hibernating
    short int exitCode; ///< Contains the exit code
    int fileTable[MAX_OPEN_FILES];///table of files opened by the process
    
    ///Maps the pid to the Process instance. Includes zombie processes
    static std::map<pid_t,Process *> processes;
    static std::list<Process *> kernelChilds;
    static std::list<Process *> kernelZombies;
    ///Used to assign a new pid to a process
    static pid_t pidCounter;
    ///Uset to guard access to processes and pidCounter
    static Mutex procMutex;
    ///Used to wait on process termination
    static ConditionVariable genericWaiting;
    //Used to take account of the ELF size opportunely rounded to suit for MPU 
    unsigned int roundedSize;
    //FIXME: the following two must be implemented in future (smart drivers)
    ///The following buffer has to be initialized in the constructor to store samples
    void* sampleBuf;
    ///the following will store the size of a sample to be retrieved from sensors
    ///it should be initialized in the constructor too
    int sizeOfSample;
    
    
    
    //Needs access to fault,mpu
    friend class Thread;
    //Needs access to mpu
    friend class PriorityScheduler;
    //Needs access to mpu
    friend class ControlScheduler;
    //Needs access to mpu
    friend class EDFScheduler;
    //Nedds access interruption point and resume methods, to handle hibernation
    friend class SuspendManager;
};

} //namespace miosix

#endif //WITH_PROCESSES

#endif //PROCESS_H
