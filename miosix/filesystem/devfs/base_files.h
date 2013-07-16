/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
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

#ifndef BASE_FILES_H
#define	BASE_FILES_H

#include "devfs.h"
#include "console_device.h"

namespace miosix {

/**
 * A file where write operations do nothing at all
 */
class NullFile : public ConsoleDevice
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    NullFile() {}
    
    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param len the number of bytes to write
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    virtual ssize_t write(const void *data, size_t len);
    
    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param len the number of bytes to read
     * \return the number of read characters, or a negative number in case
     * of errors
     */
    virtual ssize_t read(void *data, size_t len);
    
    /**
     * Write a string to the Console.
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt.
     * \param str the string to write. The string must be NUL terminated.
     */
    virtual void IRQwrite(const char *str);
};

/**
 * A file where read operations return zeroed out memory
 */
class ZeroFile : public DeviceFile
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    ZeroFile() {}
    
    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param len the number of bytes to write
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    virtual ssize_t write(const void *data, size_t len);
    
    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param len the number of bytes to read
     * \return the number of read characters, or a negative number in case
     * of errors
     */
    virtual ssize_t read(void *data, size_t len);
};

} //namespace miosix

#endif //BASE_FILES_H
