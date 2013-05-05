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

#ifndef FILE_ACCESS_H
#define FILE_ACCESS_H

#include <map>
#include <list>
#include <string>
#include <utility>
#include <cstring>
#include <cstddef>
#include <errno.h>
#include <sys/stat.h>
#include "kernel/intrusive.h"
#include "kernel/sync.h"
#include "config/miosix_settings.h"

namespace miosix {

/**
 * All filesystems derive from this class
 */
class FilesystemBase
{
public:
    /**
     * Open a file
     * \param file the file object will be stored here, if the call succeeds
     * \param name the name of the file to open, relative to the local
     * filesystem
     * \param flags file flags (open for reading, writing, ...)
     * \param mode file permissions
     */
    virtual int open(intrusive_ref_ptr<FileBase>& file, std::string name,
            int flags, int mode)=0;
    
    /**
     * Used to forcedly umount the filesystem
     * Wait until all files related to this filesystem have been closed
     * \return 0 on success, a negative number on failure
     */
    virtual int waitUntilNoOpenFiles()=0;
    
    /**
     * Called every time a file object is destroyed
     */
    virtual void fileCloseHook();
            
    /**
     * Destructor
     */
    virtual ~FilesystemBase();
    
private:
    FilesystemBase(const FilesystemBase&);
    FilesystemBase& operator= (const FilesystemBase&);
};

/**
 * This class implements the default version of the waitUntilNoOpenFiles()
 * required to safely umount the filesystem. Most filesystems will want to
 * derive from this class rather than FilesystemBase.
 */
class Filesystem : public FilesystemBase
{
public:
    /**
     * Constructor 
     */
    Filesystem() : openFilesCount(0) {}
    
    /**
     * Used to forcedly umount the filesystem
     * Wait until all files related to this filesystem have been closed
     * \return 0 on success, a negative number on failure
     */
    virtual int waitUntilNoOpenFiles();
    
    /**
     * Called every time a file object is destroyed
     */
    virtual void fileCloseHook();
    
protected:  
    /**
     * Must be called every time a new file is created, so as to keep the
     * count of open files updated
     */
    void newFileOpened();
    
private:
    Mutex mutex;
    ConditionVariable cond;
    int openFilesCount; ///< Counter of open files
};

/**
 * This class maps file descriptors to file objects, allowing to
 * perform file operations
 */
class FileDescriptorTable
{
public:
    /**
     * Constructor
     */
    FileDescriptorTable() {}
    
    /**
     * Copy constructor
     * \param rhs object to copy from
     */
    FileDescriptorTable(const FileDescriptorTable& rhs);
    
    /**
     * Operator=
     * \param rhs object to copy from
     * \return *this 
     */
    FileDescriptorTable& operator=(const FileDescriptorTable& rhs);
    
    /**
     * Open a file
     * \param name file name
     * \param flags file open mode
     * \param mode allows to set file permissions
     * \return a file descriptor, or a negative number on error
     */
    int open(const char *name, int flags, int mode);
    
    /**
     * Close a file
     * \param fd file descriptor to close
     * \return 0 on success or a negative number on failure
     */
    int close(int fd);
    
    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param len the number of bytes to write
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    ssize_t write(int fd, const void *data, size_t len)
    {
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->write(data,len);
    }
    
    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param len the number of bytes to read
     * \return the number of read characters, or a negative number in case
     * of errors
     */
    ssize_t read(int fd, void *data, size_t len)
    {
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->read(data,len);
    }
    
    /**
     * Move file pointer, if the file supports random-access.
     * \param pos offset to sum to the beginning of the file, current position
     * or end of file, depending on whence
     * \param whence SEEK_SET, SEEK_CUR or SEEK_END
     * \return the offset from the beginning of the file if the operation
     * completed, or a negative number in case of errors
     */
    off_t lseek(int fd, off_t pos, int whence)
    {
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->lseek(pos,whence);
    }
    
    /**
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    int fstat(int fd, struct stat *pstat) const
    {
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->fstat(pstat);
    }
    
    /**
     * Check whether the file refers to a terminal.
     * \return 1 if it is a terminal, 0 if it is not, or a negative number in
     * case of errors
     */
    int isatty(int fd) const
    {
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->isatty();
    }
    
    /**
     * Wait until all I/O operations have completed on this file.
     * \return 0 on success, or a negative number in case of errors
     */
    int sync(int fd)
    {
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->sync();
    }
    
    /**
     * Used by umount, removes all file entries belonging to a filesystem
     * that will soon be umounted
     * \param fs filesystem that will be umounted
     */
    void removeIfFromThisFilesystem(const FilesystemBase *fs);
    
    /**
     * Destructor
     */
    ~FileDescriptorTable();
    
private:
    /**
     * Retrieves an entry in the file descriptor table
     * \param fd file descriptor, index into the table
     * \return a refcounted poiter to the file at the desired entry
     * (which may be empty), or an empty refcounted pointer if the index is
     * out of bounds 
     */
    intrusive_ref_ptr<FileBase> getFile(int fd)
    {
        if(fd<0 || fd>=MAX_OPEN_FILES) return intrusive_ref_ptr<FileBase>();
        return atomic_load(files+fd);
    }
    
    Mutex mutex; ///< Locks on writes to files, not on accesses
    
    /// Holds the mapping between fd and file objects
    intrusive_ref_ptr<FileBase> files[MAX_OPEN_FILES];
};

/**
 * This class contains information on all the mounted filesystems
 */
class FilesystemManager
{
public:
    /**
     * \return the instance of the filesystem manager (singleton)
     */
    static FilesystemManager& instance();
    
    /**
     * 
     */
    int mount();
    
    /**
     * Unmounts a filesystem
     * \param path path to a filesytem
     * \return 0 on success, or a negative number on error
     */
    int umount(const char *path);
    
    /**
     * Resolve a path to identify the filesystem it belongs
     * \param path a path name
     * \return a pair with the pointer to the filesystem it belongs, and the
     * path without the prefix that leads to the filesystem
     */
    std::pair<FilesystemBase*,std::string> resolvePath(const char *path);
    
private:
    /**
     * Constructor, private as it is a singleton
     */
    FilesystemManager();
    
    FilesystemManager(const FilesystemManager&);
    FilesystemManager& operator=(const FilesystemManager&);
    
    Mutex mutex; ///< To protect against concurrent access
    std::map<std::string,FilesystemBase*> filesystems; ///< Mounted filesystem
//    #ifdef WITH_PROCESSES
    std::list<FileDescriptorTable*> fileTables; ///< Process file tables
//    #else //WITH_PROCESSES
//    FileDescriptorTable fileTable; ///< The only file table
//    #endif //WITH_PROCESSES
};

} //namespace miosix

#endif //FILE_ACCESS_H
