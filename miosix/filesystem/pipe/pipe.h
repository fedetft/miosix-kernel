/***************************************************************************
 *   Copyright (C) 2024 by Terraneo Federico                               *
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

#include "filesystem/file.h"
#include "kernel/sync.h"
#include "config/miosix_settings.h"

#ifdef WITH_FILESYSTEM

namespace miosix {

/**
 * Pipe
 * This is an almost standard compliant pipe implementation. A pipe should in
 * theory have two separate file descriptors pointing to two file objects, one
 * for the read end and one for the write end. This pipe implementation is a
 * single file object that does both. This file object is aliased through
 * reference counting to both file descriptors, which can be used
 * interchangeably as read/write end. Code that uses those file descriptor in a
 * standard compliant way won't notice the difference.
 */
class Pipe : public FileBase
{
public:
    /**
     * Constructor
     */
    Pipe();

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
     * Move file pointer, if the file supports random-access.
     * \param pos offset to sum to the beginning of the file, current position
     * or end of file, depending on whence
     * \param whence SEEK_SET, SEEK_CUR or SEEK_END
     * \return the offset from the beginning of the file if the operation
     * completed, or a negative number in case of errors
     */
    virtual off_t lseek(off_t pos, int whence);

    /**
     * Truncate the file
     * \param size new file size
     * \return 0 on success, or a negative number on failure
     */
    virtual int ftruncate(off_t size);

    /**
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    virtual int fstat(struct stat *pstat) const;

    /**
     * Perform various operations on a file descriptor
     * \param cmd specifies the operation to perform
     * \param opt optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    virtual int fcntl(int cmd, int opt);

    /**
     * Destructor
     */
    ~Pipe();

private:
    /**
     * \return true if there is only one file descriptor associated with the pipe
     * Must be called with mutex locked
     */
    bool unconnected();

    static const int defaultSize=256;
    static const int pollTime=100000000; //100ms
    FastMutex m;
    ConditionVariable cv;
    int put, get, size, capacity;
    char *buffer;
};

} //namespace miosix

#endif //WITH_FILESYSTEM
