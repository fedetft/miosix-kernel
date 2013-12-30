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

#include "devfs.h"
#include <string>
#include <errno.h>
#include <fcntl.h>
#include "filesystem/stringpart.h"
#include "interfaces/disk.h" //FIXME: remove
#include "filesystem/ioctl.h" //FIXME: remove

using namespace std;

#ifdef WITH_DEVFS

namespace miosix {

static void fillStatHelper(struct stat* pstat, unsigned int st_ino,
        short st_dev, mode_t mode)
{
    memset(pstat,0,sizeof(struct stat));
    pstat->st_dev=st_dev;
    pstat->st_ino=st_ino;
    pstat->st_mode=mode;
    pstat->st_nlink=1;
    pstat->st_blksize=0; //If zero means file buffer equals to BUFSIZ
}

/**
 * This file type is for reading and writing from devices
 */
class DevFsFile : public FileBase
{
public:
    /**
     * Constructor
     * \param fs pointer to DevFs
     * \param dev the device to which this file refers
     * \param seekable true if the device is seekable
     */
    DevFsFile(intrusive_ref_ptr<FilesystemBase> fs,
            intrusive_ref_ptr<Device> dev, bool seekable) : FileBase(fs),
            dev(dev), seekPoint(seekable ? 0 : -1) {}

    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param len the number of bytes to write
     * \return the number of written characters, or a negative number in
     * case of errors
     */
    virtual ssize_t write(const void *data, size_t len);

    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param len the number of bytes to read
     * \return the number of read characters, or a negative number in
     * case of errors
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
     * Perform various operations on a file descriptor
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    virtual int ioctl(int cmd, void *arg);

private:
    intrusive_ref_ptr<Device> dev; ///< Device file
    ssize_t seekPoint;             ///< Seek point, or -1 if unseekabble
};

ssize_t DevFsFile::write(const void *data, size_t len)
{
    if(seekPoint>0 && seekPoint+len<0)
        len=numeric_limits<size_t>::max()-seekPoint-len;
    int result=dev->writeBlock(data,len,seekPoint);
    if(result>0 && seekPoint>0) seekPoint+=result;
    return result;
}

ssize_t DevFsFile::read(void *data, size_t len)
{
    if(seekPoint>0 && seekPoint+len<0)
        len=numeric_limits<size_t>::max()-seekPoint-len;
    int result=dev->readBlock(data,len,seekPoint);
    if(result>0 && seekPoint>0) seekPoint+=result;
    return result;
}

off_t DevFsFile::lseek(off_t pos, int whence)
{
    if(seekPoint<0) return -EBADF; //No seek support
    
    ssize_t newSeekPoint=seekPoint;
    switch(whence)
    {
        case SEEK_CUR:
            newSeekPoint+=pos;
            break;
        case SEEK_SET:
            newSeekPoint=pos;
            break;
        default:
            return -EINVAL; //TODO: how to implement SEEK_END?
    }
    if(newSeekPoint<0) return -EOVERFLOW;
    seekPoint=newSeekPoint;
    return seekPoint;
}

int DevFsFile::fstat(struct stat *pstat) const
{
    return dev->lstat(pstat);
}

int DevFsFile::ioctl(int cmd, void *arg)
{
    return dev->ioctl(cmd,arg);
}

//
// class Device
//

int Device::open(intrusive_ref_ptr<FileBase>& file,
        intrusive_ref_ptr<FilesystemBase> fs, int flags, int mode)
{
    file=intrusive_ref_ptr<FileBase>(
        new DevFsFile(fs,shared_from_this(),seekable));
    return 0;
}

int Device::lstat(struct stat* pstat) const
{
    mode_t mode=(block ? S_IFBLK : S_IFCHR) | 0750;//brwxr-x--- | crwxr-x---
    fillStatHelper(pstat,st_ino,st_dev,mode);
    return 0;
}

int Device::readBlock(void *buffer, int size, int where)
{
    memset(buffer,0,size); //Act as /dev/zero
    return size;
}

int Device::writeBlock(const void *buffer, int size, int where)
{
    return size; //Act as /dev/null
}

int Device::ioctl(int cmd, void *arg)
{
    return -ENOTTY; //Means the operation does not apply to this descriptor
}

Device::~Device() {}

//
// class DiskAdapter
//

// FIXME temporary -- begin
DiskAdapter::DiskAdapter() : Device(true,true)
{
    if(Disk::isAvailable()) Disk::init();
}

int DiskAdapter::readBlock(void* buffer, int size, int where)
{
    if(Disk::isInitialized()==false) return -EBADF;
    if(where % 512 || size % 512) return -EFAULT;
    Lock<FastMutex> l(mutex);
    if(Disk::read((unsigned char*)buffer,where/512,size/512)) return size;
    else return -EFAULT;
}

int DiskAdapter::writeBlock(const void* buffer, int size, int where)
{
    if(Disk::isInitialized()==false) return -EBADF;
    if(where % 512 || size % 512) return -EFAULT;
    Lock<FastMutex> l(mutex);
    if(Disk::write((const unsigned char*)buffer,where/512,size/512)) return size;
    else return -EFAULT;
}

int DiskAdapter::ioctl(int cmd, void *arg)
{
    if(cmd!=IOCTL_SYNC) return -ENOTTY;
    Lock<FastMutex> l(mutex);
    Disk::sync();
    return 0;
}
// FIXME temporary -- begin

/**
 * Directory class for DevFs 
 */
class DevFsDirectory : public DirectoryBase
{
public:
    /**
     * \param parent parent filesystem
     * \param mutex mutex to lock when accessing the file map
     * \param files file map
     * \param currentInode inode of the directory we're listing
     * \param parentInode inode of the parent directory
     */
    DevFsDirectory(intrusive_ref_ptr<FilesystemBase> parent,
            FastMutex& mutex,
            map<StringPart,intrusive_ref_ptr<Device> >& files,
            int currentInode, int parentInode)
            : DirectoryBase(parent), mutex(mutex), files(files),
              currentInode(currentInode), parentInode(parentInode),
              first(true), last(false)
    {
        Lock<FastMutex> l(mutex);
        if(files.empty()==false) currentItem=files.begin()->first.c_str();
    }

    /**
     * Also directories can be opened as files. In this case, this system
     * call allows to retrieve directory entries.
     * \param dp pointer to a memory buffer where one or more struct dirent
     * will be placed. dp must be four words aligned.
     * \param len memory buffer size.
     * \return the number of bytes read on success, or a negative number on
     * failure.
     */
    virtual int getdents(void *dp, int len);

private:
    FastMutex& mutex;                 ///< Mutex of parent class
    map<StringPart,intrusive_ref_ptr<Device> >& files; ///< Directory entries
    string currentItem;               ///< First unhandled item in directory
    int currentInode,parentInode;     ///< Inodes of . and ..

    bool first; ///< True if first time getdents is called
    bool last;  ///< True if directory has ended
};

int DevFsDirectory::getdents(void *dp, int len)
{
    if(len<minimumBufferSize) return -EINVAL;
    if(last) return 0;
    
    Lock<FastMutex> l(mutex);
    char *begin=reinterpret_cast<char*>(dp);
    char *buffer=begin;
    char *end=buffer+len;
    if(first)
    {
        first=false;
        addDefaultEntries(&buffer,currentInode,parentInode);
    }
    if(currentItem.empty()==false)
    {
        map<StringPart,intrusive_ref_ptr<Device> >::iterator it;
        it=files.find(StringPart(currentItem));
        //Someone deleted the exact directory entry we had saved (unlikely)
        if(it==files.end()) return -EBADF;
        for(;it!=files.end();++it)
        {
            struct stat st;
            it->second->lstat(&st);
            if(addEntry(&buffer,end,st.st_ino,st.st_mode>>12,it->first)>0)
                continue;
            //Buffer finished
            currentItem=it->first.c_str();
            return buffer-begin;
        }
    }
    addTerminatingEntry(&buffer,end);
    last=true;
    return buffer-begin;
}

//
// class DevFs
//

DevFs::DevFs() : mutex(FastMutex::RECURSIVE), inodeCount(rootDirInode+1)
{
    addDevice("null",intrusive_ref_ptr<Device>(new Device));
    addDevice("zero",intrusive_ref_ptr<Device>(new Device));
}

bool DevFs::addDevice(const char *name, intrusive_ref_ptr<Device> dev)
{
    if(name==0 || name[0]=='\0') return false;
    int len=strlen(name);
    for(int i=0;i<len;i++) if(name[i]=='/') return false;
    Lock<FastMutex> l(mutex);
    bool result=files.insert(make_pair(StringPart(name),dev)).second;
    //Assign inode to the file
    if(result) dev->setFileInfo(atomicAddExchange(&inodeCount,1),filesystemId);
    return result;
}

bool DevFs::remove(const char* name)
{
    if(name==0 || name[0]=='\0') return false;
    Lock<FastMutex> l(mutex);
    map<StringPart,intrusive_ref_ptr<Device> >::iterator it;
    it=files.find(StringPart(name));
    if(it==files.end()) return false;
    files.erase(StringPart(name));
    return true;
}

int DevFs::open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
        int flags, int mode)
{
    Lock<FastMutex> l(mutex);
    if(name.empty()) //Trying to open the root directory of the fs
    {
        if(flags & (O_WRONLY | O_RDWR | O_APPEND | O_TRUNC)) return -EACCES;
        file=intrusive_ref_ptr<FileBase>(
            new DevFsDirectory(shared_from_this(),
                mutex,files,rootDirInode,parentFsMountpointInode));
        return 0;
    }
    map<StringPart,intrusive_ref_ptr<Device> >::iterator it=files.find(name);
    if(it==files.end()) return -ENOENT;
    return it->second->open(file,shared_from_this(),flags,mode);
}

int DevFs::lstat(StringPart& name, struct stat *pstat)
{
    Lock<FastMutex> l(mutex);
    if(name.empty())
    {
        fillStatHelper(pstat,rootDirInode,filesystemId,S_IFDIR | 0755);//drwxr-xr-x
        return 0;
    }
    map<StringPart,intrusive_ref_ptr<Device> >::iterator it=files.find(name);
    if(it==files.end()) return -ENOENT;
    return it->second->lstat(pstat);
}

int DevFs::unlink(StringPart& name)
{
    Lock<FastMutex> l(mutex);
    if(files.erase(name)==1) return 0;
    return -ENOENT;
}

int DevFs::rename(StringPart& oldName, StringPart& newName)
{
    Lock<FastMutex> l(mutex);
    map<StringPart,intrusive_ref_ptr<Device> >::iterator it=files.find(oldName);
    if(it==files.end()) return -ENOENT;
    for(unsigned int i=0;i<newName.length();i++)
        if(newName[i]=='/')
            return -EACCES; //DevFs does not support subdirectories
    files.erase(newName); //If it exists
    files.insert(make_pair(newName,it->second));
    files.erase(it);
    return 0;
}

int DevFs::mkdir(StringPart& name, int mode)
{
    return -EACCES; // No directories support in DevFs yet
}

int DevFs::rmdir(StringPart& name)
{
    return -EACCES; // No directories support in DevFs yet
}

} //namespace miosix

#endif //WITH_DEVFS
