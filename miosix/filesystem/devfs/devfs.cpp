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
#include "base_files.h"
#include "filesystem/stringpart.h"

using namespace std;

#ifdef WITH_DEVFS

namespace miosix {

static void fillStatHelper(struct stat* pstat, unsigned int st_ino, short st_dev)
{
    memset(pstat,0,sizeof(struct stat));
    pstat->st_dev=st_dev;
    pstat->st_ino=st_ino;
    pstat->st_mode=S_IFCHR | 0755;//crwxr-xr-x Character device
    pstat->st_nlink=1;
    pstat->st_blksize=0; //If zero means file buffer equals to BUFSIZ
}

//
// class DeviceFile
//

off_t DeviceFile::lseek(off_t pos, int whence)
{
    switch(whence)
    {
        case SEEK_SET:
        case SEEK_CUR:
        case SEEK_END:
            return -EBADF;
        default:
            return -EINVAL;
    }
}

//
// class StatelessDeviceFile
//

int StatelessDeviceFile::fstat(struct stat* pstat) const
{
    fillStatHelper(pstat,st_ino,st_dev);
    return 0;
}

//
// class StatefulDeviceFile
//

int StatefulDeviceFile::fstat(struct stat *pstat) const
{
    pair<unsigned int,short> fi=dfg->getFileInfo();
    fillStatHelper(pstat,fi.first,fi.second);
    return 0;
}

//
// class DeviceFileGenerator
//

int DeviceFileGenerator::lstat(struct stat *pstat)
{
    fillStatHelper(pstat,st_ino,st_dev);
    return 0;
}

DeviceFileGenerator::~DeviceFileGenerator() {}

//
// class DevFs
//

DevFs::DevFs() : inodeCount(1)
{
    //This will increase openFilesCount to 1, because we don't have an easy way
    //to keep track of open files, due to DeviceFileWrapper having stateless
    //device files open also when no process has it open, and because having
    //the DeviceFile's parent pointer point to the DevFs would generate a loop
    //of refcounted pointers with stateless files. Simply, we don't recommend
    //umounting DevFs, and to do so we leave openFilesCount permanently to 1,
    //so that only a forced umount can umount it.
    newFileOpened();
    
    addDeviceFile("/null",intrusive_ref_ptr<StatelessDeviceFile>(new NullFile));
    addDeviceFile("/zero",intrusive_ref_ptr<StatelessDeviceFile>(new ZeroFile));
}

bool DevFs::remove(const char* name)
{
    if(name==0 || name[0]!='/' || name[1]=='\0') return false;
    Lock<FastMutex> l(mutex);
    map<StringPart,DeviceFileWrapper>::iterator it=files.find(StringPart(name));
    if(it==files.end()) return false;
    files.erase(StringPart(name));
    return true;
}

int DevFs::open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
        int flags, int mode)
{
    if(flags & O_CREAT) return -EINVAL; //Can't create files in DevFs this way
    Lock<FastMutex> l(mutex);
    map<StringPart,DeviceFileWrapper>::iterator it=files.find(name);
    if(it==files.end()) return -ENOENT;
    return it->second.open(file,flags,mode);
}

int DevFs::lstat(StringPart& name, struct stat *pstat)
{
    Lock<FastMutex> l(mutex);
    map<StringPart,DeviceFileWrapper>::iterator it=files.find(name);
    if(it==files.end()) return -ENOENT;
    return it->second.lstat(pstat);
}

int DevFs::mkdir(StringPart& name, int mode)
{
    return -EACCES; // No directories support in DevFs yet
}

bool DevFs::addDeviceFile(const char *name, DeviceFileWrapper dfw)
{
    if(name==0 || name[0]!='/' || name[1]=='\0') return false;
    int len=strlen(name);
    for(int i=1;i<len;i++) if(name[i]=='/') return false;
    Lock<FastMutex> l(mutex);
    bool result=files.insert(make_pair(StringPart(name),dfw)).second;
    //Assign inode to the file
    if(result) dfw.setFileInfo(atomicAddExchange(&inodeCount,1),filesystemId);
    return result;
}

} //namespace miosix

#endif //WITH_DEVFS
