/***************************************************************************
 *   Copyright (C) 2013-2024 by Terraneo Federico                          *
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

#include "file.h"
#include <cstdio>
#include <string>
#include <fcntl.h>
#include "file_access.h"
#include "miosix_settings.h"

using namespace std;

namespace miosix {

//The filesystem code assumes off_t is 64bit.
static_assert(sizeof(off_t)==8,"");

//
// class FileBase
//

#ifdef WITH_FILESYSTEM

#ifdef WITH_POSIX_PERMISSIONS
bool FileBase::canWriteInFile(uid_t euid, gid_t egid, struct stat& filestat)
{
    if (euid==0) return true;
    if (filestat.st_uid==euid && filestat.st_mode & S_IWUSR) return true;
    if (filestat.st_gid==egid && filestat.st_mode & S_IWGRP) return true;
    if (filestat.st_mode & S_IWOTH) return true;
    return false;
}

bool FileBase::canReadFromFile(uid_t euid, gid_t egid, struct stat& filestat)
{
    if (euid==0) return true;
    if (filestat.st_uid==euid && filestat.st_mode & S_IRUSR) return true;
    if (filestat.st_gid==egid && filestat.st_mode & S_IRGRP) return true;
    if (filestat.st_mode & S_IROTH) return true;
    return false;
}

bool FileBase::canExecuteFile(uid_t euid, gid_t egid, struct stat& filestat)
{
    if (euid==0) return true;
    if (filestat.st_uid==euid && filestat.st_mode & S_IXUSR) return true;
    if (filestat.st_gid==egid && filestat.st_mode & S_IXGRP) return true;
    if (filestat.st_mode & S_IXOTH) return true;
    return false;
}
#endif

FileBase::FileBase(intrusive_ref_ptr<FilesystemBase> parent, int flags)
        : parent(parent), flags(flags)
{
    if(parent) parent->newFileOpened();
}

FileBase::~FileBase()
{
    if(parent) parent->fileCloseHook();
}

int FileBase::isatty() const
{
    return 0;
}

int FileBase::fcntl(int cmd, int opt)
{
    switch(cmd)
    {
        case F_GETFD:
        case F_GETFL: //TODO: also return file access mode
            return flags;
    }
    return -EBADF;
}

int FileBase::ioctl(int cmd, void *arg)
{
    return -ENOTTY; //Means the operation does not apply to this descriptor
}

int FileBase::getdents(void *dp, int len)
{
    return -EBADF;
}

MemoryMappedFile FileBase::getFileFromMemory()
{
    return MemoryMappedFile(nullptr,0); //Not supported
}

//
// class DirectoryBase
//

#ifdef WITH_POSIX_PERMISSIONS
bool DirectoryBase::canEditDirectoryEntries(uid_t euid, gid_t egid, struct stat& dirstat)
{
    // for creating or renaming or moving or unlinking a file in a directory we need write 
    // permissions, but we also need execute permissions to access the directory, but we 
    // have to have both permissions on the same permission set so it is not enough to do 
    // return canWriteToFile(euid, egid, dirstat) || canExecuteFile(euid, egid, dirstat)
    
    if (euid==0) return true;
    if (dirstat.st_uid==euid && dirstat.st_mode & (S_IWUSR | S_IXUSR)) return true;
    if (dirstat.st_gid==egid && dirstat.st_mode & (S_IWGRP | S_IXGRP)) return true;
    if (dirstat.st_mode & (S_IWOTH | S_IXOTH)) return true;
    return false;
}

bool DirectoryBase::canReadDirectory(uid_t euid, gid_t egid, struct stat& dirstat)
{
    // in this case the same permission of files applies to directories
    return FileBase::canReadFromFile(euid, egid, dirstat);
}

bool DirectoryBase::canSearchDirectory(uid_t euid, gid_t egid, struct stat& dirstat)
{
    // we can search a directory if it has execute bit set
    return FileBase::canExecuteFile(euid, egid, dirstat);
}
#endif

ssize_t DirectoryBase::write(const void *data, size_t len)
{
    return -EBADF;
}
    
ssize_t DirectoryBase::read(void *data, size_t len)
{
    return -EBADF;
}

off_t DirectoryBase::lseek(off_t pos, int whence)
{
    return -EBADF;
}

int DirectoryBase::ftruncate(off_t size)
{
    return -EBADF;
}

int DirectoryBase::fstat(struct stat *pstat) const
{
    return -EBADF;
}

int DirectoryBase::addEntry(char **pos, char *end, ino_t ino, char type, const char *n)
{
    int reclen=direntHeaderSizeNoPadding+strlen(n)+1;
    reclen=(reclen+3) & ~0x3; //Align to 4 bytes
    if(reclen>end-*pos) return -1;
    
    struct dirent *data=reinterpret_cast<struct dirent*>(*pos);
    data->d_ino=ino;
    data->d_off=0;
    data->d_reclen=reclen;
    data->d_type=type;
    strcpy(data->d_name,n);
    
    (*pos)+=reclen;
    return reclen;
}

int DirectoryBase::addDefaultEntries(char **pos, ino_t thisIno, ino_t upIno)
{
    struct dirent *data=reinterpret_cast<struct dirent*>(*pos);
    data->d_ino=thisIno;
    data->d_off=0;
    data->d_reclen=direntHeaderSize;
    data->d_type=DT_DIR;
    strcpy(data->d_name,".");
    
    (*pos)+=direntHeaderSize;
    data=reinterpret_cast<struct dirent*>(*pos);
    data->d_ino=upIno;
    data->d_off=0;
    data->d_reclen=direntHeaderSize;
    data->d_type=DT_DIR;
    strcpy(data->d_name,"..");
    
    (*pos)+=direntHeaderSize;
    return 2*direntHeaderSize;
}

int DirectoryBase::addTerminatingEntry(char **pos, char *end)
{
    if(direntHeaderSize>end-*pos) return -1;
    //This sets everything to zero, including d_reclen, terminating the
    //directory listing loop in readdir.c
    memset(*pos,0,direntHeaderSize);
    (*pos)+=direntHeaderSize;
    return direntHeaderSize;
}

unsigned char DirectoryBase::modeToType(unsigned short mode)
{
    switch(mode & S_IFMT)
    {
        case S_IFREG: return DT_REG;
        case S_IFLNK: return DT_LNK;
        case S_IFDIR: return DT_DIR;
        case S_IFIFO: return DT_FIFO;
        case S_IFCHR: return DT_CHR;
        case S_IFBLK: return DT_BLK;
        case S_IFSOCK: return DT_SOCK;
        default: return DT_UNKNOWN;
    }
}

//
// class FilesystemBase
//

FilesystemBase::FilesystemBase() :
        filesystemId(FilesystemManager::getFilesystemId()),
        parentFsMountpointInode(1), openFileCount(0) {}

int FilesystemBase::readlink(StringPart& name, string& target)
{
    return -EINVAL; //Default implementation, for filesystems without symlinks
}

bool FilesystemBase::supportsSymlinks() const { return false; }

void FilesystemBase::newFileOpened() { atomicAdd(&openFileCount,1); }

void FilesystemBase::fileCloseHook()
{
    int result=atomicAddExchange(&openFileCount,-1);
    if(extraChecks!=ExtraChecks::None)
        if(result<0) errorHandler(Error::UNEXPECTED);
}

int FilesystemBase::mkfs()
{
    return 1;
}

FilesystemBase::~FilesystemBase() {}

#endif //WITH_FILESYSTEM

} //namespace miosix
