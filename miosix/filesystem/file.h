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

#include <cstring>
#include <cstddef>
#include <errno.h>
#include <sys/stat.h>
#include "kernel/sync.h"
#include "kernel/intrusive.h"

#ifndef FILE_H
#define	FILE_H

namespace miosix {

// Forward decls
class FilesystemBase;

/**
 * The unix file abstraction. Also some device drivers are seen as files.
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 */
class FileBase : public IntrusiveRefCounted
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    FileBase(intrusive_ref_ptr<FilesystemBase> parent) : parent(parent) {}
    
    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param len the number of bytes to write
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    virtual ssize_t write(const void *data, size_t len)=0;
    
    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param len the number of bytes to read
     * \return the number of read characters, or a negative number in case
     * of errors
     */
    virtual ssize_t read(void *data, size_t len)=0;
    
    /**
     * Move file pointer, if the file supports random-access.
     * \param pos offset to sum to the beginning of the file, current position
     * or end of file, depending on whence
     * \param whence SEEK_SET, SEEK_CUR or SEEK_END
     * \return the offset from the beginning of the file if the operation
     * completed, or a negative number in case of errors
     */
    virtual off_t lseek(off_t pos, int whence)=0;
    
    /**
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    virtual int fstat(struct stat *pstat) const=0;
    
    /**
     * Check whether the file refers to a terminal.
     * \return 1 if it is a terminal, 0 if it is not, or a negative number in
     * case of errors
     */
    virtual int isatty() const=0;
    
    /**
     * Wait until all I/O operations have completed on this file.
     * \return 0 on success, or a negative number in case of errors
     */
    virtual int sync()=0;
    
    /**
     * \return a pointer to the parent filesystem
     */
    const intrusive_ref_ptr<FilesystemBase> getParent() const { return parent; }
    
    /**
     * File destructor
     */
    virtual ~FileBase();
    
private:
    FileBase(const FileBase&);
    FileBase& operator=(const FileBase&);
    
    intrusive_ref_ptr<FilesystemBase> parent; ///< Files may have a parent fs
};

/**
 * A file where write operations do nothing at all
 */
class NullFile : public FileBase
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    NullFile() : FileBase(intrusive_ref_ptr<FilesystemBase>()) {}
    
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
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    virtual int fstat(struct stat *pstat) const;
    
    /**
     * Check whether the file refers to a terminal.
     * \return 1 if it is a terminal, 0 if it is not, or a negative number in
     * case of errors
     */
    virtual int isatty() const;
    
    /**
     * Wait until all I/O operations have completed on this file.
     * \return 0 on success, or a negative number in case of errors
     */
    virtual int sync();
};

/**
 * A file where read operations return zeroed out memory
 */
class ZeroFile : public FileBase
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    ZeroFile() : FileBase(intrusive_ref_ptr<FilesystemBase>()) {}
    
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
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    virtual int fstat(struct stat *pstat) const;
    
    /**
     * Check whether the file refers to a terminal.
     * \return 1 if it is a terminal, 0 if it is not, or a negative number in
     * case of errors
     */
    virtual int isatty() const;
    
    /**
     * Wait until all I/O operations have completed on this file.
     * \return 0 on success, or a negative number in case of errors
     */
    virtual int sync();
};

/**
 * Teriminal device, proxy object supporting additional terminal-specific
 * features
 */
class TerminalDevice : public FileBase
{
public:
    /**
     * Constructor
     * \param device proxed device.
     */
    TerminalDevice(intrusive_ref_ptr<FileBase> device);
    
    /**
     * Convenience function to write a text string, terminated with \0.
     * \param str the string to write.
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    int write(const char *str)
    {
        return write(str,std::strlen(str));
    }
    
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
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    virtual int fstat(struct stat *pstat) const;
    
    /**
     * Check whether the file refers to a terminal.
     * \return 1 if it is a terminal, 0 if it is not, or a negative number in
     * case of errors
     */
    virtual int isatty() const;
    
    /**
     * Wait until all I/O operations have completed on this file.
     * \return 0 on success, or a negative number in case of errors
     */
    virtual int sync();
    
    /**
     * Enables or disables echo of commands on the terminal
     * \param echo true to enable echo, false to disable it
     */
    void setEcho(bool echoMode) { echo=echoMode; }
    
    /**
     * \return true if echo is enabled 
     */
    bool isEchoEnabled() const { return echo; }
    
    /**
     * Selects whether the terminal sholud be transparent to non ASCII data
     * \param rawMode true if raw mode is required
     */
    void setBinary(bool binaryMode) { binary=binaryMode; }
    
    /**
     * \return true if the terminal allows binary data 
     */
    bool isBinary() const { return binary; }
    
private:
    intrusive_ref_ptr<FileBase> device;
    FastMutex mutex;
    bool echo;
    bool binary;
    bool skipNewline;
};

/**
 * FIXME remove this when removing Console interface!
 */
class ConsoleAdapter : public FileBase
{
public:
    ConsoleAdapter() : FileBase(intrusive_ref_ptr<FilesystemBase>()) {}
    virtual ssize_t write(const void *data, size_t len);
    virtual ssize_t read(void *data, size_t len);
    virtual off_t lseek(off_t pos, int whence);
    virtual int fstat(struct stat *pstat) const;
    virtual int isatty() const;
    virtual int sync();
};

} //namespace miosix

#endif //FILE_H
