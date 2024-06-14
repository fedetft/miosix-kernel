/***************************************************************************
 *   Copyright (C) 2018 - 2024 by Terraneo Federico                        *
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
#include <cstddef>
#include <utility>

namespace miosix {

/**
 * \param size in bytes >32
 * \return a value that can be written to MPU->RASR to represent that size
 */
unsigned int sizeToMpu(unsigned int size);

/**
 * To be called at boot to enable the MPU.
 * Without calling this function, the MPU will not work even if regions are
 * configured in MPUConfiguration
 */
inline void IRQenableMPUatBoot()
{
    #if __MPU_PRESENT==1
    MPU->CTRL=MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
    #endif //__MPU_PRESENT==1
}

#ifdef WITH_PROCESSES

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
     * \internal
     * \param elfBase base address of the ELF file
     * \param elfSize size of the ELF file
     * \param imageBase base address of the Process RAM image
     * \param imageSize size of the Process RAM image
     */
    MPUConfiguration(const unsigned int *elfBase, unsigned int elfSize,
            const unsigned int *imageBase, unsigned int imageSize);
    
    /**
     * \internal
     * This method is used to configure the Memoy Protection region for a 
     * Process during a context-switch to a userspace thread.
     * Can only be called inside an IRQ, not even with interrupts disabled
     */
    void IRQenable()
    {
        #if __MPU_PRESENT==1
        MPU->RBAR=regValues[0];
        MPU->RASR=regValues[1];
        MPU->RBAR=regValues[2];
        MPU->RASR=regValues[3];
        __set_CONTROL(3);
        #endif //__MPU_PRESENT==1
    }
    
    /**
     * \internal
     * This method is used to disable the MPU during a context-switch to a
     * kernelspace thread.
     * Can only be called inside an IRQ, not even with interrupts disabled
     */
    static void IRQdisable()
    {
        #if __MPU_PRESENT==1
        __set_CONTROL(2);
        #endif //__MPU_PRESENT==1
    }
    
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

} //namespace miosix
