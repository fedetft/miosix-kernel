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
#include "filesystem/path.h"
#include "kernel/logging.h"
#include "interfaces/endianness.h"
#include "util/util.h"
#include "romfs_types.h"

#ifdef WITH_FILESYSTEM

using namespace std;

namespace miosix {

const void *getRomFsAddressAfterKernel()
{
    // We don't (yet) have a symbol marking the end of the kernel, but we can
    // compute it
    extern char _data asm("_data");
    extern char _edata asm("_edata");
    extern char _etext asm("_etext");
    const char *kernelEnd=&_etext+(&_edata-&_data);
    // Align the resulting pointer
    const unsigned int align=romFsImageAlignment;
    kernelEnd=reinterpret_cast<const char*>(
             (reinterpret_cast<unsigned int>(kernelEnd)+align-1) & (0-align));
    // Check for romfs start marker
    bool valid=true;
    for(int i=0;i<5;i++) if(kernelEnd[i]!='w') valid=false;
    if(valid) return kernelEnd;
    errorLog("Error finding RomFs, expecting it @ %p\n",kernelEnd);
    #ifdef WITH_ERRLOG
    memDump(kernelEnd-32,64);
    #endif //WITH_ERRLOG
    return nullptr;
}

/**
 * Fill a struct stat
 * \param pstat struct stat to fill
 * \param entry Directory entry of file/directory to stat
 * \param dev Id of current filesystem
 */
static void fillStatHelper(struct stat* pstat, const RomFsDirectoryEntry *entry,
                           unsigned short dev)
{
    memset(pstat,0,sizeof(struct stat));
    pstat->st_dev=dev;
    pstat->st_ino=fromLittleEndian32(entry->inode);
    pstat->st_mode=fromLittleEndian16(entry->mode);
    pstat->st_nlink=1;
    pstat->st_uid=fromLittleEndian16(entry->uid);
    pstat->st_gid=fromLittleEndian16(entry->gid);
    pstat->st_size=fromLittleEndian32(entry->size);
    pstat->st_blksize=0; //If zero means file buffer equals to BUFSIZ
    //NOTE: st_blocks should be number of 512 byte blocks regardless of st_blksize
    pstat->st_blocks=(pstat->st_size+512-1)/512;
}

/**
 * Compute address of next RomFsDirectoryEntry entry
 * \param entry current directory entry
 * \return pointer to next entry.
 * Note, if entry was the last one, the returned pointer is one past the last one
 */
static const RomFsDirectoryEntry *nextEntry(const RomFsDirectoryEntry *entry)
{
    auto last=reinterpret_cast<unsigned int>(entry->name+strlen(entry->name)+1);
    return reinterpret_cast<const RomFsDirectoryEntry *>(
        (last+romFsStructAlignment-1) & (0-romFsStructAlignment));
}

/**
 * File class for MemoryMappedRomFs
 */
class MemoryMappedRomFsFile : public FileBase
{
public:
    /**
     * Constructor
     * \param parent pointer to parent filesystem
     * \param flags file open flags
     * \param entry directory entry containing the file information
     */
    MemoryMappedRomFsFile(intrusive_ref_ptr<FilesystemBase> parent, int flags,
            const RomFsDirectoryEntry *entry) : FileBase(parent,flags),
            entry(entry), seekPoint(0) {}

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
     * For filesystems whose backing storage is memory-mapped and additionally
     * for files that are stored as a contiguous block, allow access to the
     * underlying storage. Mostly used for execute-in-place of processes.
     *
     * \return information about the in-memory storage of the file, or return
     * {nullptr,0} if this feature is not supported.
     */
    virtual MemoryMappedFile getFileFromMemory();

private:
    const RomFsDirectoryEntry * const entry;
    off_t seekPoint; ///< Seek point (note that off_t is 64bit)
};

ssize_t MemoryMappedRomFsFile::write(const void *data, size_t len) { return -EINVAL; }

ssize_t MemoryMappedRomFsFile::read(void *data, size_t len)
{
    unsigned int size=fromLittleEndian32(entry->size);
    if(seekPoint>=size) return 0;
    size_t toRead=min<size_t>(len,size-seekPoint);
    #ifdef __NO_EXCEPTIONS
    auto parent=static_pointer_cast<MemoryMappedRomFs>(getParent());
    #else
    auto parent=dynamic_pointer_cast<MemoryMappedRomFs>(getParent());
    #endif
    memcpy(data,parent->ptr(fromLittleEndian32(entry->inode))+seekPoint,toRead);
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
            newSeekPoint=pos+fromLittleEndian32(entry->size);
        default:
            return -EINVAL;
    }
    if(newSeekPoint<0) return -EOVERFLOW;
    seekPoint=newSeekPoint;
    return seekPoint;
}

int MemoryMappedRomFsFile::ftruncate(off_t size) { return -EROFS; }

int MemoryMappedRomFsFile::fstat(struct stat *pstat) const
{
    fillStatHelper(pstat,entry,getParent()->getFsId());
    return 0;
}

MemoryMappedFile MemoryMappedRomFsFile::getFileFromMemory()
{
    #ifdef __NO_EXCEPTIONS
    auto parent=static_pointer_cast<MemoryMappedRomFs>(getParent());
    #else
    auto parent=dynamic_pointer_cast<MemoryMappedRomFs>(getParent());
    #endif
    return MemoryMappedFile(parent->ptr(fromLittleEndian32(entry->inode)),
                            fromLittleEndian32(entry->size));
}

/**
 * Directory class for MemoryMappedRomFs
 */
class MemoryMappedRomFsDirectory : public DirectoryBase
{
public:
    /**
     * Constructor
     * \param parent pointer to parent filesystem
     * \param entry directory entry containing the directory information
     */
    MemoryMappedRomFsDirectory(intrusive_ref_ptr<FilesystemBase> parent,
            const RomFsDirectoryEntry *entry) : DirectoryBase(parent), entry(entry) {}

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
    const RomFsDirectoryEntry * const entry;
    unsigned int index=0; ///< First unhandled directory entry
};

int MemoryMappedRomFsDirectory::getdents(void *dp, int len)
{
    if(len<minimumBufferSize) return -EINVAL;
    
    char *begin=reinterpret_cast<char*>(dp);
    char *buffer=begin;
    char *end=buffer+len;
    #ifdef __NO_EXCEPTIONS
    auto parent=static_pointer_cast<MemoryMappedRomFs>(getParent());
    #else
    auto parent=dynamic_pointer_cast<MemoryMappedRomFs>(getParent());
    #endif
    if(!parent) return -EBADF;

    unsigned int inode=fromLittleEndian32(entry->inode);
    if(index==0)
    {
        auto first=parent->ptr<const RomFsFirstEntry*>(inode);
        int upIno=first->parentInode;
        if(upIno==0) upIno=parent->getParentFsMountpointInode(); //Root dir?
        addDefaultEntries(&buffer,inode,upIno);
        index=inode+sizeof(RomFsFirstEntry);
    }
    unsigned int last=inode+fromLittleEndian32(entry->size);
    while(index<last)
    {
        auto e=parent->ptr<const RomFsDirectoryEntry*>(index);
        unsigned int entryInode=fromLittleEndian32(e->inode);
        unsigned char entryMode=modeToType(fromLittleEndian16(e->mode));
        if(addEntry(&buffer,end,entryInode,entryMode,e->name)<0)
            return buffer-begin;
        index+=sizeof(RomFsDirectoryEntry)+strlen(e->name)+1; // +1 for the \0
        index=(index+romFsStructAlignment-1) & (0-romFsStructAlignment);
    }
    addTerminatingEntry(&buffer,end);
    return buffer-begin;
}

//
// class MemoryMappedRomFs
//

MemoryMappedRomFs::MemoryMappedRomFs(const void *baseAddress)
    : base(reinterpret_cast<const char*>(baseAddress)), failed(false)
{
    auto header=ptr<const RomFsHeader*>(0);
    if(strncmp(header->fsName,"RomFs 2.01",11)==0) return;
    errorLog("Unexpected FS version %s\n",header->fsName);
    failed=true;
}

int MemoryMappedRomFs::open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
        int flags, int mode)
{
    if(failed) return -ENOENT;
    if(flags & (O_APPEND | O_EXCL | O_WRONLY | O_RDWR)) return -EROFS;
    const RomFsDirectoryEntry *entry=findEntry(name);
    if(entry==nullptr) return -ENOENT;
    switch(fromLittleEndian16(entry->mode) & S_IFMT)
    {
        case S_IFREG:
            file=intrusive_ref_ptr<FileBase>(new MemoryMappedRomFsFile(
                shared_from_this(),flags,entry));
            break;
        case S_IFDIR:
            file=intrusive_ref_ptr<FileBase>(new MemoryMappedRomFsDirectory(
                shared_from_this(),entry));
            break;
        case S_IFLNK:
            return -EFAULT; // We should not arrive at open with a symlink
        default:
            return -ENOENT;
    }
    return 0;
}

int MemoryMappedRomFs::lstat(StringPart& name, struct stat *pstat)
{
    if(failed) return -ENOENT;
    const RomFsDirectoryEntry *entry=findEntry(name);
    if(entry==nullptr) return -ENOENT;
    fillStatHelper(pstat,entry,getFsId());
    return 0;
}

int MemoryMappedRomFs::truncate(StringPart& name, off_t size) { return -EROFS; }
int MemoryMappedRomFs::unlink(StringPart& name) { return -EROFS; }
int MemoryMappedRomFs::rename(StringPart& oldName, StringPart& newName) { return -EROFS; }
int MemoryMappedRomFs::mkdir(StringPart& name, int mode) { return -EROFS; }
int MemoryMappedRomFs::rmdir(StringPart& name) { return -EROFS; }

int MemoryMappedRomFs::readlink(StringPart& name, string& target)
{
    if(failed) return -ENOENT;
    const RomFsDirectoryEntry *entry=findEntry(name);
    if(entry==nullptr) return -ENOENT;
    if((fromLittleEndian16(entry->mode) & S_IFMT)!=S_IFLNK) return -EINVAL;
    target.assign(ptr(fromLittleEndian32(entry->inode)),fromLittleEndian32(entry->size));
    return 0;
}

bool MemoryMappedRomFs::supportsSymlinks() const { return true; }

const RomFsDirectoryEntry *MemoryMappedRomFs::findEntry(StringPart& name)
{
    auto entry=ptr<const RomFsDirectoryEntry *>(sizeof(RomFsHeader));
    if(name.empty()) return entry;
    NormalizedPathWalker pw(name);
    while(auto element=pw.next())
    {
        if((fromLittleEndian16(entry->mode) & S_IFMT)!=S_IFDIR) return nullptr;
        unsigned int inode=fromLittleEndian32(entry->inode);
        const void *end=ptr(inode+fromLittleEndian32(entry->size));
        entry=ptr<const RomFsDirectoryEntry *>(inode+sizeof(RomFsFirstEntry));
        while(entry<end)
        {
            if(strcmp(element->c_str(),entry->name)==0) break;
            entry=nextEntry(entry);
        }
        if(entry>=end) return nullptr; //Not found
    }
    return entry;
}

} //namespace miosix

#endif //WITH_FILESYSTEM
