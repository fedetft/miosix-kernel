/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
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

#ifndef FS_BACKEND_H
#define	FS_BACKEND_H

namespace miosix {

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file disk.h
 * This file only contains the Disk class
 */

/**
 * \internal
 * This class is an abstraction over a block device.
 * The kernel uses it to implement filesystem access through the C/C++ standard
 * library. Note that the filesystem code is architecture independent and is
 * part of the kernel. The implementation of this interface only needs to
 * be able to read and write a sector on the disk.
 * The underlying implementation depends on the architecture chosen when
 * compiling Miosix and can be for example an SD, MMC or uSD card, an hard drive
 * or a FLASH chip.
 * This code cannot be called from multiple threads simultaneously. This is
 * achieved at a higher level by a mutex that guards all filesystem accesses.
 * It cannot also be called with interrupts disabled, or inside an IRQ.
 * It can, however, be called when the kernel is paused.
 *
 * The implementation of this class is in
 * arch/arch name/board name/interfaces-impl
 */
class Disk
{
public:

    /**
     * If the block device is removable, this function must return true if the
     * disk is attached and therefore available for read and write operations,
     * and return false if the device is not attached.
     * If the device is not removable this function must always return true.
     * \return true if the device is available for read-write operations
     */
    static bool isAvailable();
    
    /**
     * At system boot, before init() has been called this function must return
     * false. After the device has been initialized with init() it must
     * return true or false depending whether the initialization succeeded or
     * not.
     * \return true if the device is initialized.
     */
    static bool isInitialized()
    {
        //This function is called *many* times during filesystem access
        return diskInitialized;
    }

    /**
     * Initialize the block device.
     * This function is called when the filesystem is mounted.
     */
    static void init();

    /**
     * Read a specified number of disk sectors from the device.
     * The size of a sector must be 512 Bytes.
     * \param buffer place where data read from the device is to be written
     * \param lba Logical Block Address to read, if nSectors is 1. If nSectors
     * is greater than 1 lba is the address of the first block to read.
     * \param nSectors number of sectors to read.
     * \return true on success, false on failure
     */
    static bool read(unsigned char *buffer, unsigned int lba,
            unsigned char nSectors);

    /**
     * Write a specified number of sectors to the device.
     * The size of a sector is 512 Bytes.
     * If the device does not support write operations, this function must
     * always return false.
     * \param buffer a buffer containing the data to be written
     * \param lba Logical Block Address to write, if nSectors is 1. If nSectors
     * is greater than 1 lba is the address of the first block to write.
     * \param nSectors number of sectors to write.
     * \return true on success, false on failure
     */
    static bool write(const unsigned char *buffer, unsigned int lba,
            unsigned char nSectors);

    /**
     * This function should return only after all ongoing write operations on
     * the device are completed.
     * \return true on success, false on failure
     */
    static bool sync();

private:
    static bool diskInitialized; ///\internal true if disk is initialized
};

/**
 * \}
 */

} //namespace miosix

#endif //FS_BACKEND_H
