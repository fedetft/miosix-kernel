/***************************************************************************
 *   Copyright (C) 2012-2024 by Terraneo Federico                          *
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

#pragma once

#include <vector>
#include <map>
#include <list>
#include <set>
#include <sys/types.h>
#include "kernel.h"
#include "sync.h"
#include "elf_program.h"
#include "config/miosix_settings.h"
#include "filesystem/file_access.h"

#ifdef WITH_PROCESSES

namespace miosix {

//Forware decl
class Process;
class ArgsBlock;

/**
 * This class contains the fields that are in common between the kernel and
 * processes
 */
class ProcessBase
{
public:
    /**
     * Constructor
     * A ProcessBase is constructed with a default-constructed file descriptor
     * table. The kernel itself (which is process 0 and is the only ProcessBase
     * that is not also a Process derived class) is constructed this way.
     */
    ProcessBase() {}

    /**
     * Constructor
     * Constrcts a ProcessBase so that it inherits the file descriptor table
     * from the parent process.
     * \param fdt file descriptor table of the parent process
     */
    ProcessBase(const FileDescriptorTable& fdt) : fileTable(fdt) {}
    
    /**
     * \return the process' pid 
     */
    pid_t getPid() const { return pid; }
    
    /**
     * \return the process file descriptor table
     */
    FileDescriptorTable& getFileTable() { return fileTable; }
    
protected:
    pid_t pid=0;  ///<The pid of this process
    pid_t ppid=0; ///<The parent pid of this process
    std::list<Process *> childs;   ///<Living child processes are stored here
    std::list<Process *> zombies;  ///<Dead child processes are stored here
    FileDescriptorTable fileTable; ///<The file descriptor table
    
private:
    ProcessBase(const ProcessBase&)=delete;
    ProcessBase& operator= (const ProcessBase&)=delete;
    
    friend class Process;
};

/**
 * Process class, allows to create and handle processes
 */
class Process : public ProcessBase
{
public:
    /**
     * Create a new process
     * \param program Program that the process will execute
     * \param args program arguments and environment variables
     * \return the pid of the newly created process
     * \throws std::exception or a subclass in case of errors, including
     * not emough memory to spawn the process
     */
    static pid_t create(ElfProgram&& program, ArgsBlock&& args);

    /**
     * Create a new process from a file in the filesystem
     * \param path file path of the program to spawn
     * \param argv program arguments
     * \param envp program environment variables
     * \param narg number of arguments
     * \param nenv number of environment variables
     * \return the pid of the newly created process
     * \throws std::exception or a subclass in case of errors, including
     * not emough memory to spawn the process
     */
    static pid_t spawn(const char *path, const char* const* argv,
            const char* const* envp, int narg, int nenv);

    /**
     * Create a new process from a file in the filesystem
     * \param path file path of the program to spawn
     * \param argv program arguments
     * \param envp program environment variables
     * \return the pid of the newly created process
     * \throws std::exception or a subclass in case of errors, including
     * not emough memory to spawn the process
     */
    static pid_t spawn(const char *path, const char* const* argv=nullptr,
                       const char* const* envp=nullptr);
    
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
     * \return the pid of the terminated process, or an error code
     */
    static pid_t wait(int *exit) { return waitpid(-1,exit,0); }
    
    /**
     * Wait for a specific child process to terminate
     * \param pid pid of the process, or -1 to wait for any child process
     * \param exit the process exit code will be returned here, if the pointer
     * is not null
     * \param options only 0 and WNOHANG are supported
     * \return the pid of the terminated process, or an error code. In
     * case WNOHANG  is specified and the specified process has not terminated,
     * 0 is returned
     */
    static pid_t waitpid(pid_t pid, int *exit, int options);
    
    /**
     * Destructor
     */
    ~Process();
    
private:
    
    /**
     * Constructor
     * \param fdt file descriptor table of the parent process
     * \param program program that will be executed by the process
     * \param args program arguments and environment variables
     */
    Process(const FileDescriptorTable& fdt, ElfProgram&& program,
            ArgsBlock&& args);

    /**
     * Load the a program into memory to be run as a process
     * \param program program that will be executed by the process
     * \param args program arguments and environment variables
     */
    void load(ElfProgram&& program, ArgsBlock&& args);
    
    /**
     * Contains the process' main loop. 
     * \param argv the process pointer is passed here
     * \return null
     */
    static void *start(void *argv);

    enum SvcResult
    {
        Resume=0,   ///< Process can switch to userspace and resume operation
        Exit=1,     ///< Process exited
        Execve=2,   ///< Process can resume, but the program has been switched
        Segfault=3  ///< Unrecoverable error occurred
    };
    
    /**
     * Handle a supervisor call
     * \param sp syscall parameters
     * \return true if the process can continue running, false if it has
     * terminated
     */
    SvcResult handleSvc(miosix_private::SyscallParameters sp);
    
    /**
     * \return an unique pid that is not zero and is not already in use in the
     * system, used to assign a pid to a new process.<br>
     */
    static pid_t getNewPid();
    
    ElfProgram program; ///<The program that is running inside the process
    ProcessImage image; ///<The RAM image of a process
    miosix_private::FaultData fault; ///< Contains information about faults
    MPUConfiguration mpu; ///<Memory protection data
    int argc;   ///< Process argument count
    void *argvSp; ///< Ptr to argument array within ProcessImage and initial sp
    void *envp; ///< Pointer to the environment array within the ProcessImage
    
    std::vector<Thread *> threads; ///<Threads that belong to the process
    
    ///Contains the count of active wait calls which specifically requested
    ///to wait on this process
    int waitCount;
    ///Active wait calls which specifically requested to wait on this process
    ///wait on this condition variable
    ConditionVariable waiting;
    bool zombie; ///< True for terminated not yet joined processes
    short int exitCode; ///< Contains the exit code
    
    //Needs access to fault,mpu
    friend class Thread;
    //Needs access to mpu
    friend class PriorityScheduler;
    //Needs access to mpu
    friend class ControlScheduler;
    //Needs access to mpu
    friend class EDFScheduler;
};

/**
 * Class used to create a copy of the argv and envp data during calls such as
 * execve
 */
class ArgsBlock
{
public:
    /**
     * Default constructor, yields an invalid object
     */
    ArgsBlock() {}

    /**
     * Constructor
     * \param argv argv array
     * \param envp env array
     * \param narg number of elements of argv array
     * \param nenv number of elements of envp array
     */
    ArgsBlock(const char* const* argv, const char* const* envp, int narg, int nenv);

    /**
     * \return true if the object is valid
     */
    bool valid() const { return block!=nullptr; }

    /**
     * \return the pointer to the arg block data
     */
    const char *data() const { return block; }

    /**
     * \return the arg block size
     */
    unsigned int size() const { return blockSize; }

    /**
     * \return the number of arguments
     */
    int getNumberOfArguments() const { return narg; }

    /**
     * \return the position in the arg block of the envp array
     * block()+getArgIndex() is the envp array
     */
    unsigned int getEnvIndex() const { return envArrayIndex; }

    /**
     * Copy the args block to a target destination, performing a fixup of the
     * pointers so that the target args block contains the argv and envp array
     * whose pointers point to the target args block, not the source one.
     * \param target pointer to a memory area whose size is at least size()
     * bytes where the args block will be copied and relocated
     */
    void relocateTo(char *target);

    /**
     * Destructor
     */
    ~ArgsBlock();

    ArgsBlock(const ArgsBlock&)=delete;
    ArgsBlock& operator=(const ArgsBlock&)=delete;

private:
    char *block=nullptr;          ///< Buffer where args/env are packed
    unsigned int blockSize=0;     ///< Size of buffer
    unsigned int envArrayIndex=0; ///< Offset into buffer of env array
    int narg=0;                   ///< Number of args
};

/**
 * List of Miosix syscalls
 */
enum class Syscall
{
    // Yield. Can be called both by kernel threads and process threads both in
    // userspace and kernelspace mode. It causes the scheduler to switch to
    // another thread. It is the only SVC that is available also when processes
    // are disabled in miosix_config.h. No parameters, no return value.
    YIELD=0,
    // Back to userspace. It is used by process threads running in kernelspace
    // mode to return to userspace mode after completing an SVC. If called by a
    // process thread already in userspace mode it does nothing. Use of this
    // SVC by kernel threads is forbidden. No parameters, no return value.
    USERSPACE=1,

    // All other syscalls. Use of these SVC by kernel threads is forbidden.
    // Kernel should just call the functions with the corresponding name
    // (kercalls) in stdlib_integration

    // File/directory syscalls
    OPEN      = 2,
    CLOSE     = 3,
    READ      = 4,
    WRITE     = 5,
    LSEEK     = 6,
    STAT      = 7,
    LSTAT     = 8,
    FSTAT     = 9,
    FCNTL     = 10,
    IOCTL     = 11,
    ISATTY    = 12,
    GETCWD    = 13,
    CHDIR     = 14,
    GETDENTS  = 15,
    MKDIR     = 16,
    RMDIR     = 17,
    LINK      = 18,
    UNLINK    = 19,
    SYMLINK   = 20,
    READLINK  = 21,
    TRUNCATE  = 22,
    FTRUNCATE = 23,
    RENAME    = 24,
    CHMOD     = 25,
    FCHMOD    = 26,
    CHOWN     = 27,
    FCHOWN    = 28,
    LCHOWN    = 29,
    DUP       = 30,
    DUP2      = 31,
    PIPE      = 32,
    ACCESS    = 33,
    //From 34 to 37 reserved for future use

    // Time syscalls
    GETTIME   = 38,
    SETTIME   = 39,
    NANOSLEEP = 40,
    GETRES    = 41,
    ADJTIME   = 42,

    // Process syscalls
    EXIT      = 43,
    EXECVE    = 44,
    SPAWN     = 45,
    KILL      = 46,
    WAITPID   = 47,
    GETPID    = 48,
    GETPPID   = 49,
    GETUID    = 50,
    GETGID    = 51,
    GETEUID   = 52,
    GETEGID   = 53,
    SETUID    = 54,
    SETGID    = 55,

    // Filesystem syscalls
    MOUNT     = 56,
    UMOUNT    = 57,
    MKFS      = 58, //Moving filesystem creation code to kernel
};

} //namespace miosix

#endif //WITH_PROCESSES
