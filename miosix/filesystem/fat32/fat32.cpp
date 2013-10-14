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

#include "fat32.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include "filesystem/stringpart.h"
#include "interfaces/disk.h"

namespace miosix {

/**
 * Translate between FATFS error codes and POSIX ones
 * \param ec FATS error code
 * \return POSIX error code
 */
static int translateError(int ec)
{
    switch(ec)
    {
        case FR_OK:
            return 0;
        case FR_NO_FILE:
        case FR_NO_PATH:
            return -ENOENT;
        case FR_DENIED:
            return -ENOSPC;
        case FR_EXIST:
            return -EEXIST;
        case FR_WRITE_PROTECTED:
            return -EROFS;
        case FR_LOCKED:
            return -EBUSY;
        case FR_NOT_ENOUGH_CORE:
            return -ENOMEM;
        case FR_TOO_MANY_OPEN_FILES:
            return -ENFILE;
        default:
            return -EACCES;
    }
}

/**
 * RAII class for pthread_mutex_t
 */
class PthreadMutexLock
{
public:
    PthreadMutexLock(pthread_mutex_t& mutex) : mutex(mutex)
    {
        pthread_mutex_lock(&mutex);
    }
    
    ~PthreadMutexLock()
    {
        pthread_mutex_unlock(&mutex);
    }
private:
    pthread_mutex_t& mutex;
};

/**
 * Files of the Fat32Fs filesystem
 */
class Fat32File : public FileBase
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    Fat32File(intrusive_ref_ptr<FilesystemBase> parent);
    
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
     * Wait until all I/O operations have completed on this file.
     * \return 0 on success, or a negative number in case of errors
     */
    virtual int sync();
    
    /**
     * Destructor
     */
    ~Fat32File();
    
    FIL file;
    int inode;
};

//
// class Fat32File
//

Fat32File::Fat32File(intrusive_ref_ptr<FilesystemBase> parent)
        : FileBase(parent), inode(0) {}

ssize_t Fat32File::write(const void *data, size_t len)
{
    unsigned int bytesWritten;
    if(int result=translateError(f_write(&file,data,len,&bytesWritten)))
        return result;
    #ifdef SYNC_AFTER_WRITE
    //Sync for safety in case of power failure or device removal
    if(f_sync(&file)!=FR_OK) return -1;
    #endif //SYNC_AFTER_WRITE    
    return static_cast<int>(bytesWritten);
}

ssize_t Fat32File::read(void *data, size_t len)
{
    unsigned int bytesRead;
    if(int result=translateError(f_read(&file,data,len,&bytesRead)))
        return result;
    return static_cast<int>(bytesRead);
}

off_t Fat32File::lseek(off_t pos, int whence)
{
    int offset=-1;
    switch(whence)
    {
        case SEEK_CUR:
            offset=file.fptr+pos;
            break;
        case SEEK_SET:
            offset=pos;
            break;
        case SEEK_END:
            offset=file.fsize+pos;
            break;
        default:
            return -EINVAL;
    }
    if(offset<0) return -EOVERFLOW;
    if(int result=translateError(
        f_lseek(&file,static_cast<unsigned long>(offset)))) return result;
    return offset;
}

int Fat32File::fstat(struct stat *pstat) const
{
    memset(pstat,0,sizeof(struct stat));
    pstat->st_dev=getParent()->getFsId();
    pstat->st_ino=inode;
    pstat->st_mode=S_IFREG | 0755; //-rwxr-xr-x
    pstat->st_nlink=1;
    pstat->st_size=file.fsize;
    pstat->st_blksize=512;
    pstat->st_blocks=(file.fsize+511)/512;
    return 0;
}

int Fat32File::sync()
{
    return translateError(f_sync(&file));
}

Fat32File::~Fat32File()
{
    if(inode) f_close(&file); //TODO: what to do with error code?
}

//
// class Fat32Fs
//

Fat32Fs::Fat32Fs() : failed(true)
{
    //Don't lock filesystem.sobj here, as pthread_mutex_init hasn't been called
    if(!Disk::isAvailable()) return;
    failed=f_mount(&filesystem,"",1)!=FR_OK;
}

int Fat32Fs::open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
        int flags, int mode)
{
    BYTE openflags=0;
    flags++; //To convert from O_RDONLY, O_WRONLY, ... to _FREAD, _FWRITE, ...
    if(flags & _FREAD)  openflags|=FA_READ;
    if(flags & _FWRITE) openflags|=FA_WRITE;
    if(flags & _FTRUNC) openflags|=FA_CREATE_ALWAYS;//Truncate
    else if(flags & _FAPPEND) openflags|=FA_OPEN_ALWAYS;//If not exists create
    else openflags|=FA_OPEN_EXISTING;//If not exists fail
    
    intrusive_ref_ptr<Fat32File> f(new Fat32File(shared_from_this()));
    PthreadMutexLock l(filesystem.sobj);
    if(int result=translateError(f_open(&f->file,name.c_str(),openflags)))
        return result;
    f->inode=1; //FIXME: where to get inode?
    
    #ifdef SYNC_AFTER_WRITE
    //Sync for safety in case of power failure or device removal
    if(f_sync(&f->file)!=FR_OK) return -EFAULT;
    #endif //SYNC_AFTER_WRITE

    //If file opened for appending, seek to end of file
    if(flags & _FAPPEND)
        if(f_lseek(&f->file,f->file.fsize)!=FR_OK) return -EFAULT;
    
    file=f;
    return 0;
}

int Fat32Fs::lstat(StringPart& name, struct stat *pstat)
{
    PthreadMutexLock l(filesystem.sobj);
    FILINFO info;
    if(int result=translateError(f_stat(name.c_str(),&info))) return result;
    memset(pstat,0,sizeof(struct stat));
    pstat->st_dev=filesystemId;
    pstat->st_ino=1; //FIXME: where to get inode?
    pstat->st_mode=(info.fattrib & AM_DIR) ?
        S_IFDIR | 0755  //drwxr-xr-x
      : S_IFREG | 0755; //-rwxr-xr-x
    pstat->st_nlink=1;
    pstat->st_size=info.fsize;
    pstat->st_blksize=512;
    pstat->st_blocks=(info.fsize+511)/512;
    return 0;
}

int Fat32Fs::unlink(StringPart& name)
{
    PthreadMutexLock l(filesystem.sobj);
    FILINFO info;
    if(int result=translateError(f_stat(name.c_str(),&info))) return result;
    if(info.fattrib & AM_DIR) return -EISDIR;
    return translateError(f_unlink(name.c_str()));
}

int Fat32Fs::rename(StringPart& oldName, StringPart& newName)
{
    PthreadMutexLock l(filesystem.sobj);
    return translateError(f_rename(oldName.c_str(),newName.c_str()));
}

int Fat32Fs::mkdir(StringPart& name, int mode)
{
    PthreadMutexLock l(filesystem.sobj);
    return translateError(f_mkdir(name.c_str()));
}

int Fat32Fs::rmdir(StringPart& name)
{
    PthreadMutexLock l(filesystem.sobj);
    FILINFO info;
    if(int result=translateError(f_stat(name.c_str(),&info))) return result;
    if((info.fattrib & AM_DIR)==0) return -ENOTDIR;
    return translateError(f_unlink(name.c_str()));
}

Fat32Fs::~Fat32Fs()
{
    PthreadMutexLock l(filesystem.sobj);
    f_mount(0,0,0); //TODO: what to do with error code?
    Disk::sync();
}

} //namespace miosix
