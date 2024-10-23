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

#include <utility>
#include "config/miosix_settings.h"

/**
 * \addtogroup Interfaces
 * \{
 */
/**
 * \file userspace.h
 * This file contains cpu-specific functions used by the kernel to implement
 * userspace support.
 */

namespace miosix {

#ifdef WITH_PROCESSES

/**
 * \internal
 * Initializes the context of the userspace side of a userspace thread that is
 * being created.
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
 * Cause a supervisor call that will switch the kernelspace side of a userspace
 * thread back to running in userspace
 *
 * This function shall return when the userspace thread causes an action that
 * requires the kernel, such as a syscall or a fault
 */
inline void portableSwitchToUserspace();

/**
 * \internal
 * This class allows to access the parameters that a process passed to
 * the kernel as part of a system call
 */
class SyscallParameters
{
public:
    /**
     * Constructor, initialize the class starting from the thread's userspace
     * context
     * \param context the ctxSave array of the userspace side of the thread
     * that caused the syscall. From this pointer, architecture specific code
     * can extract the syscall parameters from registers and/or the stack.
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
    unsigned int *registers; ///< Architecture-specific pointer
};

/**
 * \internal
 * This class contains information about whether a fault occurred in a process.
 * It is used to terminate processes that fault.
 */
class FaultData
{
public:
    /**
     * Constructor, initializes the object to a no-fault state
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
 * This class is used to manage the MemoryProtectionUnit
 */
class MPUConfiguration
{
public:
    /**
     * Default constructor, leaves the MPU regions unconfigured
     */
    MPUConfiguration() {}

    /**
     * Provide a valid MPU configuration allowing access only to the memory
     * regions of a process
     * \param elfBase base address of the ELF file
     * \param elfSize size of the ELF file
     * \param imageBase base address of the Process RAM image
     * \param imageSize size of the Process RAM image
     */
    MPUConfiguration(const unsigned int *elfBase, unsigned int elfSize,
            const unsigned int *imageBase, unsigned int imageSize);

    /**
     * This method is used to configure the Memoy Protection region for a
     * Process during a context-switch to a userspace thread.
     * Can only be called inside an IRQ, not even with interrupts disabled
     */
    void IRQenable();

    /**
     * This method is used to disable the MPU during a context-switch to a
     * kernelspace thread.
     * Can only be called inside an IRQ, not even with interrupts disabled
     */
    static void IRQdisable();

    /**
     * Print the MPU configuration for debugging purposes
     */
    void dumpConfiguration();

    /**
     * Some MPU implementations may not allow regions of arbitrary size,
     * this function allows to round a size up to the minimum value that
     * the MPU support.
     * \param size the size of a memory area to be configured as an MPU
     * region
     * \return the size rounded to the minimum MPU region allowed that is
     * greater or equal to the given size
     */
    static unsigned int roundSizeForMPU(unsigned int size);

    /**
     * Some MPU implementations may not allow regions of arbitrary size,
     * this function allows to round a memory region up to the minimum value
     * that the MPU support.
     * \param ptr pointer to the original memory region
     * \param size original size of the memory region
     * \return a pair with a possibly enlarged memory region which contains the
     * original memory region but is aligned to be used as an MPU region
     */
    static std::pair<const unsigned int*, unsigned int>
    roundRegionForMPU(const unsigned int *ptr, unsigned int size);

    /**
     * Check if a buffer is within a readable segment of the process
     * \param ptr base pointer of the buffer to check
     * \param size buffer size
     * \return true if the buffer is correctly within the process
     */
    bool withinForReading(const void *ptr, size_t size) const;

    /**
     * Check if a buffer is within a writable segment of the process
     * \param ptr base pointer of the buffer to check
     * \param size buffer size
     * \return true if the buffer is correctly within the process
     */
    bool withinForWriting(const void *ptr, size_t size) const;

    /**
     * Check if a nul terminated string is entirely contained in the process,
     * \param str a pointer to a nul terminated string
     * \return true if the buffer is correctly within the process
     */
    bool withinForReading(const char *str) const;

    //Uses default copy constructor and operator=
private:
    ///These value are copied into the MPU registers to configure them
    unsigned int regValues[4];
};

#endif //WITH_PROCESSES

/**
 * \internal
 * To be called at boot to enable the MPU.
 * Without calling this function, the MPU will not work even if regions are
 * configured in MPUConfiguration.
 * On some architectures the MPU is also used to set cacheability regions in the
 * address space, thus this function is useful also when processes are disabled
 */
inline void IRQenableMPUatBoot();

} //namespace miosix

/**
 * \}
 */

#include "interfaces-impl/userspace_impl.h"
