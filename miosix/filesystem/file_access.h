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
#include <cassert>
#include <errno.h>
#include <sys/stat.h>
#include "kernel/sync.h"
#include "kernel/intrusive.h"
#include "config/miosix_settings.h"
#include "file.h"

namespace miosix {

//Forward decls
class ResolvedPath;
class StringPart;

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
    FilesystemBase() : openFileCount(0) {}
    
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
    
private:
    FilesystemBase(const FilesystemBase&);
    FilesystemBase& operator= (const FilesystemBase&);
    
    volatile int openFileCount; ///< Number of open files
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
    FileDescriptorTable();
    
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
        if(data==0) return -EFAULT;
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
        if(data==0) return -EFAULT;
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
        if(pstat==0) return -EFAULT;
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
     * Return file information, follows last symlink
     * \param path file to stat
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    int stat(const char *name, struct stat *pstat)
    {
        return statImpl(name,pstat,true);
    }
    
    /**
     * Return file information, does not follow last symlink
     * \param path file to stat
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    int lstat(const char *name, struct stat *pstat)
    {
        return statImpl(name,pstat,false);
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
     * Change current directory
     * \param path new current directory
     * \return 0 on success, or a negative number on failure
     */
    int chdir(const char *name);
    
    /**
     * Retrieves an entry in the file descriptor table
     * \param fd file descriptor, index into the table
     * \return a refcounted poiter to the file at the desired entry
     * (which may be empty), or an empty refcounted pointer if the index is
     * out of bounds 
     */
    intrusive_ref_ptr<FileBase> getFile(int fd) const
    {
        if(fd<0 || fd>=MAX_OPEN_FILES) return intrusive_ref_ptr<FileBase>();
        return atomic_load(files+fd);
    }
    
    /**
     * Destructor
     */
    ~FileDescriptorTable();
    
private:
    /**
     * Append cwd to path if it is not an absolute path
     * \param path an absolute or relative path, must not be null
     * \return an absolute path, or an empty string if the path would exceed
     * PATH_MAX
     */
    std::string absolutePath(const char *path);
    
    /**
     * Return file information (implements both stat and lstat)
     * \param path file to stat
     * \param pstat pointer to stat struct
     * \param f true to follow last synlink (stat),
     * false to not follow it (lstat)
     * \return 0 on success, or a negative number on failure
     */
    int statImpl(const char *name, struct stat *pstat, bool f);
    
    FastMutex mutex; ///< Locks on writes to file object pointers, not on accesses
    
    std::string cwd; ///< Current working directory
    
    /// Holds the mapping between fd and file objects
    intrusive_ref_ptr<FileBase> files[MAX_OPEN_FILES];
};

/**
 * \internal
 * This class is used to take a substring of a string containing a file path
 * without creating a substring, and therefore requiring additional memory
 * allocation.
 * 
 * When parsing a path like "/home/test/directory/file" it is often necessary
 * to create substrings at the path component boundaries, such as "/home/test".
 * In this case, it is possible to temporarily make a substring by replacing a
 * '/' with a '\0'. The std::string will not 'forget' its orginal size, and
 * when the '\0' will be converted back to a '/', the string will look identical
 * to the previous one.
 * 
 * This is an optimization made for filesystem mountpoint lookups, and is not
 * believed to be useful outside of that purpose. Given that in Miosix, the
 * mountpoints are stored in a map, this class supports operator< to correctly
 * lookup mountpoints in the map.
 */
class StringPart
{
public:
    /**
     * Default constructor
     */
    StringPart() : cstr(&saved), index(0), offset(0), saved('\0'),
            owner(false), type(CSTR)
    {
        //We need an empty C string, that is, a pointer to a char that is \0,
        //so we make cstr point to saved, and set saved to \0.
    }
    
    /**
     * Constructor from C++ string
     * \param str original string. A pointer to the string is taken, the string
     * is NOT copied. Therefore, the caller is responsible to guarantee the
     * string won't be deallocated while this class is alive. Note that the
     * original string is modified by inserting a '\0' at the index position,
     * if index is given. The string will be restored to the exact original
     * content only when this class is destroyed.
     * \param idx if this parameter is given, this class becomes an in-place
     * substring of the original string. Otherwise, this class will store the
     * entire string passed. In this case, the original string will not be
     * modified.
     * \param off if this parameter is given, the first off characters of the
     * string are skipped. Note that idx calculations take place <b>before</b>
     * offset computation, so idx is relative to the original string.
     */
    explicit StringPart(std::string& str, unsigned int idx=std::string::npos,
               unsigned int off=0);
    
    /**
     * Constructor from C string
     * \param s original string. A pointer to the string is taken, the string
     * is NOT copied. Therefore, the caller is responsible to guarantee the
     * string won't be deallocated while this class is alive. Note that the
     * original string is modified by inserting a '\0' at the index position,
     * if index is given. The string will be restored to the exact original
     * content only when this class is destroyed.
     * \param idx if this parameter is given, this class becomes an in-place
     * substring of the original string. Otherwise, this class will store the
     * entire string passed. In this case, the original string will not be
     * modified.
     * \param off if this parameter is given, the first off characters of the
     * string are skipped. Note that idx calculations take place <b>before</b>
     * offset computation, so idx is relative to the original string.
     */
    explicit StringPart(char *s, unsigned int idx=std::string::npos,
            unsigned int off=0);
    
    /**
     * Substring constructor. Given a StringPart, produce another StringPart
     * holding a substring of the original StringPart. Unlike the normal copy
     * constructor that does deep copy, this one does shallow copy, and
     * therefore the newly created object will share the same string pointer
     * as rhs. Useful for making substrings of a substring without memory
     * allocation.
     * \param rhs a StringPart
     */
    StringPart(StringPart& rhs, unsigned int idx, unsigned int off);
    
    /**
     * Copy constructor. Note that deep copying is used, so that the newly
     * created StringPart is a self-contained string. It has been done like
     * that to be able to store the paths of mounted filesystems in an std::map
     * \param rhs a StringPart
     */
    StringPart(const StringPart& rhs);
    
    /**
     * Operator = Note that deep copying is used, so that the assigned
     * StringPart becomes a self-contained string. It has been done like
     * that to be able to store the paths of mounted filesystems in an std::map
     * \param rhs a StringPart
     */
    StringPart& operator= (const StringPart& rhs);
    
    /**
     * Compare two StringParts for inequality
     * \param rhs second StringPart to compare to
     * \return true if *this < rhs
     */
    bool operator<(const StringPart& rhs) const
    {
        return strcmp(this->c_str(),rhs.c_str())<0;
    }
    
    /**
     * \param rhs a StringPart
     * \return true if this starts with rhs
     */
    bool startsWith(const StringPart& rhs) const;
    
    /**
     * \return the StringPart length, which is the same as strlen(this->c_str())
     */
    unsigned int length() const { return index-offset; }
    
    /**
     * \return the StringPart as a C string 
     */
    const char *c_str() const
    {
        return type==CSTR ? cstr+offset : str->c_str()+offset;
    }
    
    char operator[] (unsigned int index) const
    {
        return type==CSTR ? cstr[offset+index] : (*str)[offset+index];
    }
    
    /**
     * \return true if the string is empty
     */
    bool empty() const { return length()==0; }
    
    /**
     * Make this an empty string
     */
    void clear();
    
    /**
     * Destructor
     */
    ~StringPart() { clear(); }
    
private:
    /**
     * To implement copy constructor and operator=. *this must be empty.
     * \param rhs other StringPart that is assigned to *this.
     */
    void assign(const StringPart& rhs);
    
    union {
        std::string *str; ///< Pointer to underlying C++ string
        char *cstr;       ///< Pointer to underlying C string
    };
    unsigned int index;  ///< Index into the character substituted by '\0'
    unsigned int offset; ///< Offset to skip the first part of a string
    char saved;          ///< Char that was replaced by '\0'
    bool owner;          ///< True if this class owns str
    char type;           ///< either CPPSTR or CSTR. Using char to reduce size
    enum { CPPSTR, CSTR }; ///< Possible values fot type
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
     * Low level mount operation, meant to be used only inside the kernel,
     * and board support packages. It is the only mount operation that can
     * mount the root filesystem.
     * \param path path where to mount the filesystem
     * \param fs filesystem to mount. Ownership of the pointer is transferred
     * to the FilesystemManager class
     * \return 0 on success, a negative number on failure
     */
    int kmount(const char *path, intrusive_ref_ptr<FilesystemBase> fs);
    
    /**
     * Unmounts a filesystem
     * \param path path to a filesytem
     * \param force true to umount the filesystem even if busy
     * \return 0 on success, or a negative number on error
     */
    int umount(const char *path, bool force=false);
    
    /**
     * Resolve a path to identify the filesystem it belongs
     * \param path an absolute path name, that must start with '/'. Note that
     * this is an inout parameter, the string is modified so as to return the
     * full resolved path. In particular, the returned string differs from the
     * passed one by not containing useless path components, such as "/./" and
     * "//", by not containing back path componenets ("/../"), and may be
     * entirely different from the passed one if a symlink was encountered
     * during name resolution. The use of an inout parameter is to minimize
     * the number of copies of the path string, optimizing for speed and size
     * in the common case, but also means that a copy of the original string
     * needs to be made if the original has to be used later.
     * \param followLastSymlink true if the symlink in the last path component
     *(the one that does not end with a /, if it exists, has to be followed)
     * \return the resolved path
     */
    ResolvedPath resolvePath(std::string& path, bool followLastSymlink=true);
    
    /**
     * \internal
     * Called by FileDescriptorTable's constructor. Never call this function
     * from user code.
     */
    void addFileDescriptorTable(FileDescriptorTable *fdt)
    {
        #ifdef WITH_PROCESSES
        Lock<FastMutex> l(mutex);
        fileTables.push_back(fdt);
        #endif //WITH_PROCESSES
    }
    
    /**
     * \internal
     * Called by FileDescriptorTable's constructor. Never call this function
     * from user code.
     */
    void removeFileDescriptorTable(FileDescriptorTable *fdt)
    {
        #ifdef WITH_PROCESSES
        Lock<FastMutex> l(mutex);
        fileTables.remove(fdt);
        #endif //WITH_PROCESSES
    }
    
private:
    /**
     * Constructor, private as it is a singleton
     */
    FilesystemManager();
    
    FilesystemManager(const FilesystemManager&);
    FilesystemManager& operator=(const FilesystemManager&);
    
    FastMutex mutex; ///< To protect against concurrent access
    
    /// Mounted filesystem
    std::map<StringPart,intrusive_ref_ptr<FilesystemBase> > filesystems;
    
    #ifdef WITH_PROCESSES
    std::list<FileDescriptorTable*> fileTables; ///< Process file tables
    #endif //WITH_PROCESSES
};

/**
 * This class holds the file object related to the console, that is set by
 * the board support package, and used to populate /dev/console in DevFs
 */
class ConsoleDevice
{
public:
    /**
     * \return an instance of this class (singleton) 
     */
    static ConsoleDevice& instance();
    
    /**
     * Called by the board support package, in particular IRQbspInit(), to pass
     * to the kernel the console device. This device file is used as the default
     * one for stdin/stdout/stderr.
     * \param console device file handling console I/O. Can only be called with
     * interrupts disabled. 
     */
    void IRQset(intrusive_ref_ptr<FileBase> console) { this->console=console; }
    
    /**
     * \return the currently installed console device 
     */
    intrusive_ref_ptr<FileBase> get() const { return console; }
    
private:
    /**
     * Constructor, private as it is a singleton
     */
    ConsoleDevice() {}
    
    ConsoleDevice(const ConsoleDevice&);
    ConsoleDevice& operator= (const ConsoleDevice&);
    
    intrusive_ref_ptr<FileBase> console; ///< The console device
};

/**
 * \return a pointer to the file descriptor table associated with the
 * current process. Note: make sure you don't call this function before
 * IRQsetConsole(), otherwise stdin/stdout/stderr won't be set up properly
 */
FileDescriptorTable& getFileDescriptorTable();

} //namespace miosix

#endif //FILE_ACCESS_H
