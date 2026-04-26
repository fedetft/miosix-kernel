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

#ifndef COMPILING_MIOSIX
#error "This is header is private, it can't be used outside Miosix itself."
#error "If your code depends on a private header, it IS broken."
#endif //COMPILING_MIOSIX

#include "miosix_settings.h"
#include "interfaces/cpu_const.h"

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file cpu.h
 * This file contains cpu-specific functions used by the kernel to implement
 * multithreading.
 *
 * In addition to implementing all the functions in this header, architecture
 * specific code in cpu_impl.h shall provide the following:
 *
 * \code
 * //Save context from an interrupt
 * #define saveContext()
 *
 * //Restore context in an IRQ where saveContext() is used
 * #define restoreContext()
 *
 * \endcode
 */

/**
 * This global variable is used to point to the context of the currently running
 * thread. It is kept even though global variables are generally bad due to
 * performance reasons. It is used by
 * - saveContext() / restoreContext(), to perform context switches
 * - the schedulers, to set the newly running thread before a context switch
 * - IRQportableStartKernel(), to perform the first context switch
 * It is declared in thread.cpp
 */
extern "C" {
extern volatile unsigned int *ctxsave[miosix::CPU_NUM_CORES];
}

namespace miosix {

/**
 * \internal
 * Initializes the context of a new kernel thread or kernelspace side of a
 * userspace thread that is being created.
 * \param ctxsave a pointer to a field ctxsave inside a Thread class that need
 * to be filled
 * \param pc starting program counter of newly created kernel thread,
 * \param sp starting stack pointer of newly created thread. For architectures
 * that save all registers in ctxsave, this value will be used to initialize the
 * stack pointer register in ctxsave. Additionally, for architectures that save
 * some of the registers on the stack, this function will need to push on the
 * stack a frame with the initial values of the stack-saved registers
 * corresponds to the entry point of a function taking two arguments
 * \param spLimit bottom of the stack, used on some architectures for hardware
 * stack overflow detection
 * \param arg0 first argument of the thread entry function
 * \param arg1 second argument of the thread entry function
 */
void initKernelThreadCtxsave(unsigned int *ctxsave, void (*pc)(void *(*)(void*),void*),
                             unsigned int *sp, unsigned int *spLimit,
                             void *(*arg0)(void*), void *arg1) noexcept;

/**
 * \internal
 * Handle the architecture-specific part of starting the kernel. It is used by
 * the kernel, and should not be called by end users.
 * This function should at minimum enable interrupts and perform the first
 * context switch.
 *
 * NOTE: the miosix::kernel_started flag is set to true before calling this
 * function, even though the kernel cannot be considered started until this
 * function completes, causing the first context switch.
 * For this reason this function must be written with great care not to call
 * code that uses GlobalIrqLock, such as general purpose driver classes
 * that would be ran either before or after start of the kernel.
 */
void IRQportableStartKernel() noexcept;

/**
 * \internal
 * Architecture-specific way to perform a context switch
 * Must be callable both with and without the global lock. If called with the
 * global lock, the scheduler is called as soon as interrupts are enabled again
 */
inline void IRQinvokeScheduler() noexcept;

#ifndef WITH_SMP
/**
 * \internal
 * On single core architectures IRQinvokeSchedulerOnCore falls back to a simple
 * IRQinvokeScheduler, otherwise this function must be provided in smp.h
 */
inline void IRQinvokeSchedulerOnCore(unsigned char) noexcept
{
    IRQinvokeScheduler();
}
#endif //WITH_SMP

} //namespace miosix

/**
 * \}
 */

#include "interfaces-impl/cpu_impl.h"
