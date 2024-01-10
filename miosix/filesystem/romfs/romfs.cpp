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

#include "romfs.h"
#include <string>
#include <errno.h>
#include <fcntl.h>
#include "filesystem/stringpart.h"

using namespace std;

namespace miosix {

static void fillStatHelper(struct stat* pstat, unsigned int st_ino,
                           short st_dev, unsigned int size)
{
    memset(pstat,0,sizeof(struct stat));
    pstat->st_ino=st_ino;
    pstat->st_dev=st_dev;
    pstat->st_mode=S_IFREG | 0755; //-rwxr-xr-x
    pstat->st_nlink=1;
    pstat->st_size=size;
    pstat->st_blksize=1;
    pstat->st_blocks=st_size;
}

/**
 * This file type is for reading and writing from devices
 */
class MemoryMappedRomFsFile : public FileBase
{
public:
    /**
     * Constructor
     * \param fs pointer to DevFs
     * \param base pointer to first byte of file
     * \param len file length
     * \param inode inode number
     */
    MemoryMappedRomFsFile(intrusive_ref_ptr<FilesystemBase> fs,
            const char *base, int len, int inode) : FileBase(fs), base(base),
            len(len), inode(inode), seekPoint(0) {}

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

private:
    const char *base;
    int len;
    int inode;
    off_t seekPoint; ///< Seek point (note that off_t is 64bit)
};

ssize_t DevFsFile::write(const void *data, size_t len) { return -EINVAL; }

ssize_t MemoryMappedRomFsFile::read(void *data, size_t len)
{
    if(seekPoint>=len) return 0;
    int toRead=min(len,len-seekPoint);
    memcpy(data,base+seekPoint,toRead);
    seekPoint+=toRead;
    return toRead;
}

off_t MemoryMappedRomFsFile::lseek(off_t pos, int whence)
{
    off_t newSeekPoint=seekPoint;
    switch(whence)
    {
        case SEEK_CUR:
            newSeekPoint+=pos;
            break;
        case SEEK_SET:
            newSeekPoint=pos;
            break;
        case SEEK_END:
            newSeekPoint=len-pos;
        default:
            return -EINVAL;
    }
    if(newSeekPoint<0) return -EOVERFLOW;
    seekPoint=newSeekPoint;
    return seekPoint;
}

int MemoryMappedRomFsFile::fstat(struct stat *pstat) const
{
    fillStatHelper(pstat,inode,parent->getFsId(),len)
}

/**
 * Directory class for DevFs 
 */
class MemoryMappedRomFsDirectory : public DirectoryBase
{
public:
    /**
     * \param parent parent filesystem
     */
    MemoryMappedRomFsDirectory(intrusive_ref_ptr<FilesystemBase> parent)
            : DirectoryBase(parent), index(-1) {}

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
    int index; ///< First unhandled item in directory
};

int MemoryMappedRomFsDirectory::getdents(void *dp, int len)
{
    if(len<minimumBufferSize) return -EINVAL;
    if(index>=parent.header->fileCount) return 0;
    
    char *begin=reinterpret_cast<char*>(dp);
    char *buffer=begin;
    char *end=buffer+len;
    if(index==-1)
    {
        index=0;
        addDefaultEntries(&buffer,MemoryMappedRomFs::rootDirInode,
                          parent.parentFsMountpointInode);
    }
    for(;index<parent.header->fileCount;index++)
    {
        StringPart name(parent->files[i].name);
        if(addEntry(&buffer,end,index+1,S_IFREG | 0755,name)>0) continue;
        //Buffer finished
        index++;
        return buffer-begin;
    }
    addTerminatingEntry(&buffer,end);
    return buffer-begin;
}

//
// class DevFs
//

MemoryMappedRomFs::MemoryMappedRomFs(const void *baseAddress)
    : header(reinterpret_cast<RomFsHeader>(baseAddress)),
      files(reinterpret_cast<RomFsFileInfo>(
          reinterpret_cast<char*>(baseAddress+sizeof(RomFsHeader))), failed(false)
{
    for(int i=0;i<32;i++) if(header->marker[i]!='w') failed=true;
    if(strncmp(header->fsName,"RomFs 1.01"),16)
    {
        errorLog("Unexpected FS version %s\n",header->fsName);
        failed=true;
    }
}

int MemoryMappedRomFs::open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
        int flags, int mode)
{
    if(failed) return -ENOENT;
    if(flags & (O_APPEND | O_EXCL | O_WRONLY | O_RDWR)) return -EROFS;
    if(name.empty()) //Trying to open the root directory of the fs
    {
        file=intrusive_ref_ptr<FileBase>(new MemoryMappedRomFsDirectory(shared_from_this());
        return 0;
    }
    int inode;
    RomFsFileInfo *info=findFile(name.c_str(),inode);
    if(info==nullptr) return -ENOENT;
    file=intrusive_ref_ptr<FileBase>(new MemoryMappedRomFsFile(
        shared_from_this(),reinterpret_cast<char*>(header)+info->start,
        info->length,inode);
    return 0;
}

int MemoryMappedRomFs::lstat(StringPart& name, struct stat *pstat)
{
    if(failed) return -ENOENT;
    if(name.empty())
    {
        fillStatHelper(pstat,rootDirInode,filesystemId,S_IFDIR | 0755);//drwxr-xr-x
        return 0;
    }
    int inode;
    RomFsFileInfo *info=findFile(name.c_str(),inode);
    if(info==nullptr) return -ENOENT;
    fillStatHelper(pstat,inode,getFsId(),info->size)
    return 0;
}

int MemoryMappedRomFs::unlink(StringPart& name) { return -EROFS; }
int MemoryMappedRomFs::rename(StringPart& oldName, StringPart& newName) { return -EROFS; }
int MemoryMappedRomFs::mkdir(StringPart& name, int mode) { return -EROFS; }
int MemoryMappedRomFs::rmdir(StringPart& name) { return -EROFS; }

const RomFsFileInfo *MemoryMappedRomFs::findFile(const char *name, int *inode)
{
    for(int i=0;i<header->fileCount;i++)
    {
        if(strncmp(name,files[i].name,romFsFileMax)) continue;
        if(inode) *inode=i+1;
        return &files[i];
    }
    return nullptr;
}

} //namespace miosix
