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

DevFs::DevFs() : mutex(FastMutex::RECURSIVE), inodeCount(rootDirInode+1)
{
    //This will increase openFilesCount to 1, because we don't have an easy way
    //to keep track of open files, due to DeviceFileWrapper having stateless
    //device files open also when no process has it open, and because having
    //the DeviceFile's parent pointer point to the DevFs would generate a loop
    //of refcounted pointers with stateless files. Simply, we don't recommend
    //umounting DevFs, and to do so we leave openFilesCount permanently to 1,
    //so that only a forced umount can umount it.
    newFileOpened();
    
    addDeviceFile("null",intrusive_ref_ptr<StatelessDeviceFile>(new NullFile));
    addDeviceFile("zero",intrusive_ref_ptr<StatelessDeviceFile>(new ZeroFile));
}

bool DevFs::remove(const char* name)
{
    if(name==0 || name[0]=='\0') return false;
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
    if(name.empty()) //Trying to open the root directory of the fs
    {
        if(flags & (O_WRONLY | O_RDWR | O_APPEND | O_TRUNC)) return -EACCES;
        file=intrusive_ref_ptr<FileBase>(
            new DevFsDirectory(shared_from_this(),
                mutex,files,rootDirInode,parentFsMountpointInode));
        return 0;
    }
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

int DevFs::unlink(StringPart& name)
{
    Lock<FastMutex> l(mutex);
    if(files.erase(name)==1) return 0;
    return -ENOENT;
}

int DevFs::rename(StringPart& oldName, StringPart& newName)
{
    Lock<FastMutex> l(mutex);
    map<StringPart,DeviceFileWrapper>::iterator it=files.find(oldName);
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

bool DevFs::addDeviceFile(const char *name, DeviceFileWrapper dfw)
{
    if(name==0 || name[0]=='\0') return false;
    int len=strlen(name);
    for(int i=1;i<len;i++) if(name[i]=='/') return false;
    Lock<FastMutex> l(mutex);
    bool result=files.insert(make_pair(StringPart(name),dfw)).second;
    //Assign inode to the file
    if(result) dfw.setFileInfo(atomicAddExchange(&inodeCount,1),filesystemId);
    return result;
}

//
// class DevFs::DevFsDirctory
//

int DevFs::DevFsDirectory::getdents(void *dp, int len)
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
        map<StringPart,DeviceFileWrapper>::iterator it;
        it=files.find(StringPart(currentItem));
        //Someone deleted the exact directory entry we had saved (unlikely)
        if(it==files.end()) return -EBADF;
        for(;it!=files.end();++it)
        {
            struct stat st;
            it->second.lstat(&st);
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

} //namespace miosix

#endif //WITH_DEVFS
