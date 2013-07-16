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

#include <sys/stat.h>
#include "kernel/intrusive.h"

#ifndef FILE_H
#define	FILE_H

namespace miosix {

// Forward decls
class FilesystemBase;
class StringPart;

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
    virtual int isatty() const;
    
    /**
     * Wait until all I/O operations have completed on this file.
     * \return 0 on success, or a negative number in case of errors
     */
    virtual int sync();
    
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
 * All filesystems derive from this class. Classes of this type are reference
 * counted, must be allocated on the heap and managed through
 * intrusive_ref_ptr<FilesystemBase>
 */
class FilesystemBase : public IntrusiveRefCounted
{
public:
    /**
     * Constructor
     */
    FilesystemBase();
    
    /**
     * Open a file
     * \param file the file object will be stored here, if the call succeeds
     * \param name the name of the file to open, relative to the local
     * filesystem
     * \param flags file flags (open for reading, writing, ...)
     * \param mode file permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
            int flags, int mode)=0;
    
    /**
     * Obtain information on a file, identified by a path name. Does not follow
     * symlinks
     * \param name path name, relative to the local filesystem
     * \param pstat file information is stored here
     * \return 0 on success, or a negative number on failure
     */
    virtual int lstat(StringPart& name, struct stat *pstat)=0;
    
    /**
     * Create a directory
     * \param name directory name
     * \param mode directory permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int mkdir(StringPart& name, int mode)=0;
    
    /**
     * Follows a symbolic link
     * \param path path identifying a symlink, relative to the local filesystem
     * \param target the link target is returned here if the call succeeds.
     * Note that the returned path is not relative to this filesystem, and can
     * be either relative or absolute.
     * \return 0 on success, a negative number on failure
     */
    virtual int readlink(StringPart& name, std::string& target);
    
    /**
     * \return true if the filesystem supports symbolic links.
     * In this case, the filesystem should override readlink
     */
    virtual bool supportsSymlinks() const;
    
    /**
     * \internal
     * \return true if all files belonging to this filesystem are closed 
     */
    virtual bool areAllFilesClosed();
    
    /**
     * \internal
     * Called by file destructors whenever a file belonging to this
     * filesystem is closed. Never call this function from user code.
     */
    void fileCloseHook();
            
    /**
     * Destructor
     */
    virtual ~FilesystemBase();
    
protected:
    /**
     * Must be called every time a new file is successfully opened, to update
     * the open files count
     */
    void newFileOpened();
    
    const short int filesystemId; ///< The unique filesystem id, used by lstat
    
private:
    FilesystemBase(const FilesystemBase&);
    FilesystemBase& operator= (const FilesystemBase&);
    
    volatile int openFileCount; ///< Number of open files
};

} //namespace miosix

#endif //FILE_H
