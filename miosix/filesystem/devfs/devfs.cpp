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
#include "base_files.h"
#include "filesystem/stringpart.h"

using namespace std;

namespace miosix {

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

int DeviceFile::fstat(struct stat* pstat) const
{
    memset(pstat,0,sizeof(struct stat));
    pstat->st_dev=parentDfg->getDev();
    pstat->st_ino=parentDfg->getIno();
    pstat->st_mode=S_IFCHR | 0755;//crwxr-xr-x Character device
    pstat->st_nlink=1;
    pstat->st_blksize=0; //If zero means file buffer equals to BUFSIZ
    return 0;
}

//
// class DeviceFileGenerator
//

void DeviceFileGenerator::removeHook() {}

DeviceFileGenerator::~DeviceFileGenerator() {}

//
// class StatelessDeviceFileGenerator
//

StatelessDeviceFileGenerator::StatelessDeviceFileGenerator(
    intrusive_ref_ptr<DeviceFile> managedFile) : managedFile(managedFile)
{
    //Note1: this creates a loop of intrusive_ref_ptr, as the managed file
    //holds a pointer to the managed StatelessDeviceFileGenerator, and viceversa
    //but this is fixed by the removeHook()
    //Note2: creating an intrusive_ref_ptr from this works since the reference
    //count is intrusive, if ever we'll switch to shared_ptr, we'll need
    //enable_sahred_from_this
    managedFile->setDfg(intrusive_ref_ptr<DeviceFileGenerator>(this));
}

int StatelessDeviceFileGenerator::open(intrusive_ref_ptr<FileBase>& file,
        int flags, int mode)
{
    file=managedFile;
    return 0;
}

int StatelessDeviceFileGenerator::lstat(struct stat *pstat)
{
    return managedFile->fstat(pstat);
}

void StatelessDeviceFileGenerator::removeHook()
{
    //Break the cycle of smart pointers
    managedFile=0;
}

//
// class DevFs
//

DevFs::DevFs() : inodeCount(1)
{
    //This will increase openFilesCount to 1. Actually, we don't have an easy
    //way to keep track of open files, due to StatelessFileGenerators having
    //the managed file open also when no process has it open, and because
    //having the DeviceFile's parent pointer point to the DevFs would generate
    //a loop of refcounted pointers with stateless files. Simply, we don't
    //recommend umounting DevFs, and to do so we leave openFilesCount
    //permanently 1, so that only a forced umount can umount it
    newFileOpened();
    
    addDeviceFile("/null",intrusive_ref_ptr<DeviceFileGenerator>(
        new StatelessDeviceFileGenerator(
            intrusive_ref_ptr<DeviceFile>(new NullFile))));
    addDeviceFile("/zero",intrusive_ref_ptr<DeviceFileGenerator>(
        new StatelessDeviceFileGenerator(
            intrusive_ref_ptr<DeviceFile>(new ZeroFile))));
}

bool DevFs::addDeviceFile(const char* name,
        intrusive_ref_ptr<DeviceFileGenerator> file)
{
    if(name==0 || name[0]!='/' || name[1]=='\0') return false;
    int len=strlen(name);
    for(int i=1;i<len;i++) if(name[i]=='/') return false;
    Lock<FastMutex> l(mutex);
    bool result=files.insert(make_pair(StringPart(name),file)).second;
    if(result) assignInode(file);
    return result;
}

bool DevFs::removeDeviceFile(const char* name)
{
    if(name==0 || name[0]!='/' || name[1]=='\0') return false;
    Lock<FastMutex> l(mutex);
    map<StringPart,intrusive_ref_ptr<DeviceFileGenerator> >::iterator it;
    it=files.find(StringPart(name));
    if(it==files.end()) return false;
    it->second->removeHook();
    files.erase(StringPart(name));
    return true;
}

bool DevFs::removeDeviceFile(intrusive_ref_ptr<DeviceFileGenerator> dfg)
{
    if(!dfg) return false;
    Lock<FastMutex> l(mutex);
    map<StringPart,intrusive_ref_ptr<DeviceFileGenerator> >::iterator it;
    for(it=files.begin();it!=files.end();++it)
    {
        if(it->second!=dfg) continue;
        it->second->removeHook();
        files.erase(it);
        return true;
    }
    return false;
}

int DevFs::open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
        int flags, int mode)
{
    Lock<FastMutex> l(mutex);
    map<StringPart,intrusive_ref_ptr<DeviceFileGenerator> >::iterator it;
    it=files.find(name);
    if(it==files.end()) return -ENOENT;
    return it->second->open(file,flags,mode);
}

int DevFs::lstat(StringPart& name, struct stat *pstat)
{
    Lock<FastMutex> l(mutex);
    map<StringPart,intrusive_ref_ptr<DeviceFileGenerator> >::iterator it;
    it=files.find(name);
    if(it==files.end()) return -ENOENT;
    return it->second->lstat(pstat);
}

int DevFs::mkdir(StringPart& name, int mode)
{
    return -EACCES; // No directories support in DevFs yet
}

DevFs::~DevFs()
{
    map<StringPart,intrusive_ref_ptr<DeviceFileGenerator> >::iterator it;
    for(it=files.begin();it!=files.end();++it) it->second->removeHook();
}

void DevFs::assignInode(intrusive_ref_ptr<DeviceFileGenerator> file)
{
    file->setFileInfo(atomicAddExchange(&inodeCount,1),filesystemId);
}

} //namespace miosix
