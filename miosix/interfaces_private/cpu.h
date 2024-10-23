/***************************************************************************
 *   Copyright (C) 2010-2024 by Terraneo Federico                          *
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

#include "config/miosix_settings.h"

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file cpu.h
 * This file is the interface from the Miosix kernel to the hardware.
 * It ccontains what is required to perform a context switch, disable
 * interrupts, set up the stack frame and registers of a newly created thread,
 * and contains iterrupt handlers for preemption and yield.
 *
 * Since some of the functions in this file must be inline for speed reasons,
 * and context switch code must be macros, at the end of this file the file
 * portability_impl.h is included.
 * This file should contain the implementation of those inline functions.
 */

namespace miosix {
  
/**
 * \internal
 * Used after an unrecoverable error condition to restart the system, even from
 * within an interrupt routine.
 */
void IRQsystemReboot();

/**
 * \internal
 * Called by miosix::start_kernel to handle the architecture-specific part of
 * initialization. It is used by the kernel, and should not be used by end users.
 *
 * This function should at minimum enable interrupts and perform the first
 * context switch.
 *
 * NOTE: the miosix::kernel_started flag is set to true before calling this
 * function, even though the kernel cannot be considered started until this
 * function completes, causing the first context switch.
 * For this reason this function must be written with great care not to call
 * code that uses InterruptDisableLock, such as general purpose driver classes
 * that would be ran either before or after start of the kernel.
 */
void IRQportableStartKernel();

/**
 * \internal
 * Initializes the context of a new thread that is being created.
 * \param ctxsave a pointer to a field ctxsave inside a Thread class that need
 * to be filled
 * \param sp starting stack pointer of newly created thread. For architectures
 * that save all registers in ctxsave, this value will be used to initialize the
 * stack pointer register in ctxsave. Additionally, for architectures that save
 * some of the registers on the stack, this function will need to push on the
 * stack a frame with the initial values of the stack-saved registers
 * \param pc starting program counter of newly created thread, corresponds to
 * the entry point of a function taking two arguments
 * \param arg0 first argument of the thread entry function
 * \param arg1 second argument of the thread entry function
 */
void initCtxsave(unsigned int *ctxsave, unsigned int *sp,
                 void (*pc)(void *(*)(void*),void*),
                 void *(*arg0)(void*), void *arg1);

#ifdef WITH_PROCESSES

/**
 * This class allows to access the parameters that a process passed to
 * the operating system as part of a system call
 */
class SyscallParameters
{
public:
    /**
     * Constructor, initialize the class starting from the thread's userspace
     * context
     */
    SyscallParameters(unsigned int *context);
    
    /**
     * \return the syscall id, used to identify individual system calls
     */
    int getSyscallId() const;

    /**
     * \param index 0=first syscall parameter, 1=second syscall parameter, ...
     * The maximum number of syscall parameters is architecture dependent
     * \return the syscall parameter. The returned result is meaningful
     * only if the syscall (identified through its id) has the requested parameter
     */
    unsigned int getParameter(unsigned int index) const;
    
    /**
     * Set the value that will be returned by the syscall.
     * Invalidates the corresponding parameter so must be called only after the
     * syscall parameteres have been read.
     * \param index 0=first syscall parameter, 1=second syscall parameter, ...
     * The maximum number of syscall parameters is architecture dependent
     * \param value value that will be returned by the syscall.
     */
    void setParameter(unsigned int index, unsigned int value);

private:
    unsigned int *registers;
};

/**
 * This class contains information about whether a fault occurred in a process.
 * It is used to terminate processes that fault.
 */
class FaultData
{
public:
    /**
     * Constructor, initializes the object
     */
    FaultData() : id(0) {}
    
    /**
     * Constructor, initializes a FaultData object
     * \param id id of the fault
     * \param pc program counter at the moment of the fault
     * \param arg eventual additional argument, depending on the fault id
     */
    FaultData(int id, unsigned int pc, unsigned int arg=0)
            : id(id), pc(pc), arg(arg) {}
    
    /**
     * \return true if a fault happened within a process
     */
    bool faultHappened() const { return id!=0; }
    
    /**
     * Print information about the occurred fault
     */
    void print() const;
    
private:
    int id; ///< Id of the fault or zero if no faults
    unsigned int pc; ///< Program counter value at the time of the fault
    unsigned int arg;///< Eventual argument, valid only for some id values
};

/**
 * \internal
 * Initializes a ctxsave array when a thread is created.
 * This version is to initialize the userspace context of processes.
 * It is used by the kernel, and should not be used by end users.
 * \param ctxsave a pointer to a field ctxsave inside a Thread class that need
 * to be filled
 * \param pc starting program counter of newly created thread
 * \param argc number of arguments passed to main
 * \param argvSp pointer to argument array. Since the args block is stored
 * above the stack and this is the pointer into the first byte of the args
 * block, this pointer doubles as the initial stack pointer when the process
 * is started.
 * \param envp pointer to environment variables
 * \param gotBase base address of the global offset table, for userspace
 * processes
 * \param heapEnd when creating the main thread in a process, pass the pointer
 * to the end of the heap area. When creating additional threads in the process,
 * this value is irrelevant. In Miosix sbrk is not a syscall for processes as
 * the memory area allocated to a process is fixed at process creation
 */
void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), int argc,
        void *argvSp, void *envp, unsigned int *gotBase, unsigned int *heapEnd);

/**
 * \internal
 * Cause a supervisor call that will switch the thread back to kernelspace
 * It is used by the kernel, and should not be used by end users.
 */
inline void portableSwitchToUserspace();

#endif //WITH_PROCESSES

} //namespace miosix

/**
 * \}
 */

#include "interfaces-impl/cpu_impl.h"
