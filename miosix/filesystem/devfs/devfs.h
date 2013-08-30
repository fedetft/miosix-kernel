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

#ifndef DEVFS_H
#define	DEVFS_H

#include <map>
#include "filesystem/file.h"
#include "kernel/sync.h"

namespace miosix {

//Forward decls
class DeviceFileGenerator;

/**
 * All device files that are meant to be attached to DevFs must derive from this
 * class.
 */
class DeviceFile : public FileBase
{
public:
    /**
     * Constructor
     */
    DeviceFile() : FileBase(intrusive_ref_ptr<FilesystemBase>()) {}
    
    /**
     * Constructor
     * \param parentDfg the DeviceFileGenerator to which this file belongs
     */
    explicit DeviceFile(intrusive_ref_ptr<DeviceFileGenerator> parentDfg)
        : FileBase(intrusive_ref_ptr<FilesystemBase>()), parentDfg(parentDfg) {}
    
    /**
     * Default implementation that returns an error code, as most device file
     * are not seekable.
     */
    virtual off_t lseek(off_t pos, int whence);
    
    /**
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    virtual int fstat(struct stat *pstat) const;
    
    /**
     * Set the device file's parent DeviceFileGenerator
     * \param parentDfg
     */
    void setDfg(intrusive_ref_ptr<DeviceFileGenerator> parentDfg)
    {
        this->parentDfg=parentDfg;
    }
    
private:
    intrusive_ref_ptr<DeviceFileGenerator> parentDfg;
};

/**
 * Instances of this class take care of producing instances of a specific
 * device file inside DevFs. Device file can be either stateful or stateless.
 * Stateful device files, look like /proc/cpuinfo, They have a state represented
 * by the point at which the process has arrived reading the file, and as such
 * every time the device file is opened, a new instance has to be returned.
 * To support that, the driver developer has to subclass DeviceFileGenerator
 * and DeviceFile to implement the desired logic. Stateless device files are
 * like /dev/random, where there is no state and every time a read or write is
 * performed the same action is done. To support that, the driver developer
 * needs to subclass DeviceFile only, and pass an instance of the subclass to an
 * instance of StatelessDeviceFile.
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 */
class DeviceFileGenerator : public IntrusiveRefCounted
{
public:
    /**
     * Constructor.
     */
    DeviceFileGenerator() {}

    /**
     * Return an instance of the file type managed by this DeviceFileGenerator
     * \param file the file object will be stored here, if the call succeeds
     * \param flags file flags (open for reading, writing, ...)
     * \param mode file permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int open(intrusive_ref_ptr<FileBase>& file, int flags, int mode)=0;
    
    /**
     * Obtain information for the file type managed by this DeviceFileGenerator
     * \param pstat file information is stored here
     * \return 0 on success, or a negative number on failure
     */
    virtual int lstat(struct stat *pstat)=0;
    
    /**
     * This member function is called if the device file managed by this
     * DeviceFileGenerator is removed, or the DevFs is umounted.
     */
    virtual void removeHook();
    
    /**
     * \internal
     * Called be DevFs to assign a device and inode to the DeviceFileGenerator
     */
    void setFileInfo(unsigned int st_ino, short st_dev)
    {
        this->st_ino=st_ino;
        this->st_dev=st_dev;
    }
    
    /**
     * \return the inode of the file 
     */
    unsigned int getIno() const { return st_ino; }
    
    /**
     * \return the device id of the file 
     */
    short getDev() const { return st_dev; }
    
    /**
     * Destructor
     */
    virtual ~DeviceFileGenerator();
    
protected:
    unsigned int st_ino; ///< inode of device file
    short st_dev; ///< device (unique id of the filesystem) of device file
    
private:
    DeviceFileGenerator(const DeviceFileGenerator&);
    DeviceFileGenerator& operator=(const DeviceFileGenerator&);
};

/**
 * This class handles stateless device files, where every time is opened, even
 * from multiple processes, the same instance is returned.
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 */
class StatelessDeviceFileGenerator : public DeviceFileGenerator
{
public:
    /**
     * Constructor
     * \param managedFile the file that is returned every time open is called
     */
    StatelessDeviceFileGenerator(intrusive_ref_ptr<DeviceFile> managedFile);
    
    /**
     * Return an instance of the file type managed by this DeviceFileGenerator
     * \param file the file object will be stored here, if the call succeeds
     * \param flags file flags (open for reading, writing, ...)
     * \param mode file permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int open(intrusive_ref_ptr<FileBase>& file, int flags, int mode);
    
    /**
     * Obtain information for the file type managed by this DeviceFileGenerator
     * \param pstat file information is stored here
     * \return 0 on success, or a negative number on failure
     */
    virtual int lstat(struct stat *pstat);
    
    /**
     * This member function is called if the device file managed by this
     * DeviceFileGenerator is removed, or the DevFs is umounted.
     */
    virtual void removeHook();
    
private:
    intrusive_ref_ptr<DeviceFile> managedFile;
};

/**
 * DevFs is a special filesystem meant to access devices as they were files.
 * For this reason, it is a little different from other filesystems. Normal
 * filesystems create FileBase objects ondemand, to answer an open() call. Such
 * files have a parent pointer that points to the filesystem. On the contrary,
 * DevFs is a collection of DeviceFileGenerators, some of which are stateful,
 * while other are stateless. Each device file is a different subclass of
 * FileBase that overrides some of its member functions to access the handled
 * device. These FileBase subclasses do not have a parent pointer into DevFs,
 * and as such umounting DevFs should better be avoided, as it's not possible
 * to detect if some of its files are currently opened by some application.
 * What will happen is that the individual files (and DeviceFileGenerators)
 * won't be deleted until the processes that have them opened close them.
 */
class DevFs : public FilesystemBase
{
public:
    /**
     * Constructor
     */
    DevFs();
    
    /**
     * Add a device file to DevFs
     * \param name File name, must start with a slash
     * \param dfg Device file generator
     * \return true if the file was successfully added
     */
    bool addDeviceFile(const char *name, intrusive_ref_ptr<DeviceFileGenerator>
            dfg);
    
    /**
     * Remove a device file. This prevents the file from being opened again,
     * but if at the time this member function is called the file is already
     * opened, it won't be deallocated till the application closes it, thanks
     * to the reference counting scheme.
     * \param name name of file to remove
     * \return true if the file was successfully removed
     */
    bool removeDeviceFile(const char *name);
    
    /**
     * Remove a device file. This prevents the file from being opened again,
     * but if at the time this member function is called the file is already
     * opened, it won't be deallocated till the application closes it, thanks
     * to the reference counting scheme.
     * \param dfg the DeviceFileGenerator to remove
     * \return true if the file was successfully removed
     */
    bool removeDeviceFile(intrusive_ref_ptr<DeviceFileGenerator> dfg);
    
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
            int flags, int mode);
    
    /**
     * Obtain information on a file, identified by a path name. Does not follow
     * symlinks
     * \param name path name, relative to the local filesystem
     * \param pstat file information is stored here
     * \return 0 on success, or a negative number on failure
     */
    virtual int lstat(StringPart& name, struct stat *pstat);
     
    /**
     * Create a directory
     * \param name directory name
     * \param mode directory permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int mkdir(StringPart& name, int mode);
    
    /**
     * Destructor
     */
    virtual ~DevFs();
    
private:
    /**
     * Assign an unique inode to the file
     * \param file file in need of an inode
     */
    void assignInode(intrusive_ref_ptr<DeviceFileGenerator> file);
    
    FastMutex mutex;
    std::map<StringPart,intrusive_ref_ptr<DeviceFileGenerator> > files;
    int inodeCount;
};

} //namespace miosix

#endif //DEVFS_H
