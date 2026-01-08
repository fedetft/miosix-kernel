/***************************************************************************
 *   Copyright (C) 2018-2024 by Terraneo Federico                          *
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
#include "interfaces/arch_registers.h"
#include "interfaces/cpu_const.h"
#include <cassert>

namespace miosix {

#ifdef WITH_PROCESSES

// NOTE: workaround for weird compiler bug. The r7 register is the syscall ID
// in Miosix. When compiling with O0 however, r7 is also used as frame pointer.
// And when compiling with O0, if asm code using r7 gets inlined in a too
// complex function, GCC gives up with an "error: r7 cannot be used in asm here"
// Since this function needs to do a syscall and thus must use r7, let's use the
// __OPTIMIZE__ macro to figure out if we're being compiled with O0 or not, and
// if so don't inline this function.
#ifdef __OPTIMIZE__
inline void portableSwitchToUserspace()
{
    asm volatile("movs r7, #1\n\t"
                 "svc  0"
                 :::"r7", "cc", "memory");
}
#endif //__OPTIMIZE__

namespace {
/**
 * \internal
 * Offset in ctxsave of the register used ad syscall ID. On ARM, we use r7,
 * whose offset is 4. Note that the syscall ID should NOT be chosen to be a
 * register that is saved on the stack when the context is saved, as we peek
 * at that value multiple times, thus coud cause a toctou issue in syscall
 * validation.
 */
constexpr unsigned int SYSCALL_ID_OFFSET_IN_CTXSAVE=4;
}

//
// class SyscallParameters
//

inline SyscallParameters::SyscallParameters(unsigned int *context)
{
    archPtr=context;
}

inline unsigned int SyscallParameters::getSyscallId() const
{
    return archPtr[SYSCALL_ID_OFFSET_IN_CTXSAVE];
}

inline unsigned int SyscallParameters::getParameter(unsigned int index) const
{
    assert(index<MAX_NUM_SYSCALL_PARAMETERS);
    auto *psp=reinterpret_cast<unsigned int*>(archPtr[STACK_OFFSET_IN_CTXSAVE]);
    return psp[index];
}

inline void SyscallParameters::setParameter(unsigned int index, unsigned int value)
{
    assert(index<MAX_NUM_SYSCALL_PARAMETERS);
    auto *psp=reinterpret_cast<unsigned int*>(archPtr[STACK_OFFSET_IN_CTXSAVE]);
    psp[index]=value;
}

inline unsigned int peekSyscallId(unsigned int *context)
{
    return context[SYSCALL_ID_OFFSET_IN_CTXSAVE];
}

namespace fault {
/**
 * \internal
 * Possible kind of faults that the ARM Cortex CPUs can report.
 * They are used to print debug information if a process causes a fault.
 * This is a regular enum enclosed in a namespace instead of an enum class
 * as due to the need to loosely couple fault types for different architectures
 * the arch-independent code uses int to store generic fault types.
 */
enum FaultType
{
    NONE=0,          //Not a fault
    STACKOVERFLOW=1, //Stack overflow
    MP=2,            //Process attempted data access outside its memory
    MP_NOADDR=3,     //Process attempted data access outside its memory (missing addr)
    MP_XN=4,         //Process attempted code access outside its memory
    MP_STACK=5,      //Process had invalid SP while entering IRQ
    UF_DIVZERO=6,    //Process attempted to divide by zero
    UF_UNALIGNED=7,  //Process attempted unaligned memory access
    UF_COPROC=8,     //Process attempted a coprocessor access
    UF_EXCRET=9,     //Process attempted an exception return
    UF_EPSR=10,      //Process attempted to access the EPSR
    UF_UNDEF=11,     //Process attempted to execute an invalid instruction
    UF_UNEXP=12,     //Unexpected usage fault
    HARDFAULT=13,    //Hardfault (for example process executed a BKPT instruction)
    BF=14,           //Busfault
    BF_NOADDR=15     //Busfault (missing addr)
};

} //namespace fault

//
// class MPUConfiguration
//

inline void MPUConfiguration::IRQenable()
{
    #if __MPU_PRESENT==1

    #if __CORTEX_M == 33
    // ARM-V8m
    MPU->RNR =regValues[0];
    MPU->RBAR=regValues[1];
    MPU->RLAR=regValues[2];

    MPU->RNR =regValues[3];
    MPU->RBAR=regValues[4];
    MPU->RLAR=regValues[5];
    #else
    // ARM-V7m
    MPU->RBAR=regValues[0];
    MPU->RASR=regValues[1];
    MPU->RBAR=regValues[2];
    MPU->RASR=regValues[3];
    #endif

    // Set bit 0 of CONTROL register to switch thread mode to unprivileged. When
    // we'll return from the interrupt the MPU will check the access permissions
    // for unprivileged processes which only allow access to regions 6 and 7
    __set_CONTROL(3);
    #endif //__MPU_PRESENT==1
}

inline void MPUConfiguration::IRQdisable()
{
    #if __MPU_PRESENT==1
    // Clear bit 0 of CONTROL register to switch thread mode to privileged. When
    // we'll return from the interrupt the MPU will check the access permissions
    // for privileged processes which includes the default memory map as we set
    // MPU_CTRL_PRIVDEFENA at boot plus additional regions to set constraints
    // such as cacheability. Thus we never truly disable the MPU.
    __set_CONTROL(2);
    #endif //__MPU_PRESENT==1
}

#endif //WITH_PROCESSES

inline void IRQenableMPUatBoot()
{
    #if __MPU_PRESENT==1
    MPU->CTRL = MPU_CTRL_HFNMIENA_Msk
              | MPU_CTRL_PRIVDEFENA_Msk
              | MPU_CTRL_ENABLE_Msk;
    #endif //__MPU_PRESENT==1
}

/**
 * \internal
 * Convert a memory region size to a bit pattern that can be written in the MPU
 * registers.
 * On some architectures the MPU is also used to set cacheability regions in the
 * address space, thus this function is useful also when processes are disabled
 * \param size in bytes >32
 * \return a value that can be written to MPU->RASR to represent that size
 */
unsigned int sizeToMpu(unsigned int size);

} //namespace miosix
