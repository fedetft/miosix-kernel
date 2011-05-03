/***************************************************************************
 *   Copyright (C) 2008 2009 2010 by Terraneo Federico                     *
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

#include "filesystem.h"
#include <cstring>
#include <stdexcept>
#include "kernel/logging.h"
#include "interfaces/disk.h"

//Only if WITH_FILESYSTEM in miosix_setings.h is defined we need to include these functions
#ifdef WITH_FILESYSTEM

namespace miosix {

static Mutex fsMutex;///\internal This Mutex is used to guard access to the fs.
static Mutex initMutex;//To guard initialization of the singleton

//
// class Filesystem
//

Filesystem& Filesystem::instance()
{
    static Filesystem *singleton=0;
    {
        Lock<Mutex> l(initMutex);
        if(singleton==0) singleton=new Filesystem;
    }
    return *singleton;
}

char Filesystem::mount()
{
    if(!Disk::isAvailable()) return 2;//If no disk in the drive, return error
    if(mounted==true) return 3; //If filesystem already mounted, return error
    char result=0;
    {
        Lock<Mutex> l(fsMutex);
        //Mount filesystem
        if(f_mount(0,&filesystem)!=FR_OK) result=1;
        //Fatfs will delay actual filesystem mount until the first request is
        //made. So let's force it to mount the filesystem.
        DIR d;
        if(f_opendir(&d,"/")!=FR_OK) result=1;
    }
    if(result==0) mounted=true;
    return result;
}

void Filesystem::umount()
{
    Lock<Mutex> l(fsMutex);
    if(mounted==false) return;
    mounted=false;

    // Close all files eventually left open
    for(int i=0;i<MAX_OPEN_FILES;i++)
    {
        if(files[i].isEmpty()==false)
        {
            f_close(files[i].fptr);
            delete files[i].fptr;
            files[i].fptr=0;
        }
    }

    // Make sure that no pending write process in the physical drive
    Disk::sync();
}

int Filesystem::openFile(const char *name, int flags, int mode)
{
    if(isMounted()==false) return -1;

    {
        Lock<Mutex> l(fsMutex);
        
        int index=findAndFillEmptySlot();//Also fills in the file descriptor
        if(index==-1) return -1;
        
        //Set file flags
        BYTE openflags=0;
        if(flags & _FREAD)  openflags|=FA_READ;
        if(flags & _FWRITE) openflags|=FA_WRITE;
        if(flags & _FTRUNC) openflags|=FA_CREATE_ALWAYS;//Truncate
        else if(flags & _FAPPEND) openflags|=FA_OPEN_ALWAYS;//If not exists create
        else openflags|=FA_OPEN_EXISTING;//If not exists fail

        //Allocate the FatFs file data structure
        #ifdef __NO_EXCEPTIONS
        files[index].fptr=new FIL;
        if(files[index].fptr==0) return -1;
        #else //__NO_EXCEPTIONS
        try {
            files[index].fptr=new FIL;
        } catch(std::bad_alloc)
        {
            return -1;
        }
        #endif //__NO_EXCEPTIONS

        //Open file
        if(f_open(files[index].fptr,name,openflags)!=FR_OK)
        {
            //f_open failed
            delete files[index].fptr;
            files[index].fptr=0;
            return -1;
        }

        #ifdef SYNC_AFTER_WRITE
        //Sync for safety in case of power failure or card removed
        if(f_sync(files[index].fptr)!=FR_OK)
        {
            //f_sync failed
            delete files[index].fptr;
            files[index].fptr=0;
            return -1;
        }
        #endif //SYNC_AFTER_WRITE

        //If file opened for appending, seek to end of file
        if(flags & _FAPPEND)
        {
            if(f_lseek(files[index].fptr,files[index].fptr->fsize)!=FR_OK)
            {
                //Bad, an error occurred while seeking
                delete files[index].fptr;
                files[index].fptr=0;
                return -1;
            }
        }

        //Success
        return files[index].fd;
    }
}

int Filesystem::readFile(int fd, void *buffer, int bufSize)
{
    if(isMounted()==false) return -1;

    {
        Lock<Mutex> l(fsMutex);

        //Find file from descriptor
        int index=findSlot(fd);
        if(index==-1) return -1;//Trying to read from a nonexistent file
        
        unsigned int bytesRead;
        if(f_read(files[index].fptr,buffer,bufSize,&bytesRead)!=FR_OK)
            return -1;
        
        return static_cast<int>(bytesRead);
    }
}

int Filesystem::writeFile(int fd, const void *buffer, int bufSize)
{
    if(isMounted()==false) return -1;

    {
        Lock<Mutex> l(fsMutex);

        //Find file from descriptor
        int index=findSlot(fd);
        if(index==-1) return -1;//Trying to write to a nonexistent file

        unsigned int bytesWritten;
        if(f_write(files[index].fptr,buffer,bufSize,&bytesWritten)!=FR_OK)
            return -1;

        #ifdef SYNC_AFTER_WRITE
        //Sync for safety in case of power failure or card removed
        if(f_sync(files[index].fptr)!=FR_OK) return -1;
        #endif //SYNC_AFTER_WRITE
        
        return static_cast<int>(bytesWritten);
    }
}

int Filesystem::lseekFile(int fd, int pos, int whence)
{
    if(isMounted()==false) return -1;

    {
        Lock<Mutex> l(fsMutex);

        //Find file from descriptor
        int index=findSlot(fd);
        if(index==-1) return -1;//Trying to lseek a nonexistent file

        int offset=-1;
        switch(whence)
        {
            case SEEK_CUR:
                offset=files[index].fptr->fptr+pos;
                break;
            case SEEK_SET:
                offset=pos;
                break;
            case SEEK_END:
                offset=files[index].fptr->fsize+pos;
                break;
        }

        if(offset<0) return -1;
        
        if(f_lseek(files[index].fptr,(unsigned long)offset)!=FR_OK) return -1;

        return offset;
    }
}

int Filesystem::fstatFile(int fd, struct stat *pstat)
{
    if(isMounted()==false) return -1;

    {
        Lock<Mutex> l(fsMutex);

        //Find file from descriptor
        int index=findSlot(fd);
        if(index==-1) return -1;//Trying to fstat a nonexistent file

        memset(pstat,0,sizeof(struct stat));
        pstat->st_mode=S_IFREG;//Regular file
        pstat->st_nlink=1;//No hardlinks in Fat32, only 1 way to access the file
        pstat->st_blksize=0;//Want an unbuffered file, to save ram
        pstat->st_size=(int)files[index].fptr->fsize;//File size
        return 0;
    }
}

int Filesystem::statFile(const char *file, struct stat *pstat)
{
    if(isMounted()==false) return -1;
    FILINFO info;
    {
        Lock<Mutex> l(fsMutex);
        if(f_stat(file,&info)!=FR_OK) return -1;
    }
    memset(pstat,0,sizeof(struct stat));
    pstat->st_mode=S_IFREG;//Regular file
    pstat->st_nlink=1;//No hardlinks in Fat32, only 1 way to access the file
    pstat->st_blksize=0;//Want an unbuffered file, to save ram
    pstat->st_size=(int)info.fsize;//File size
    return 0;
}

int Filesystem::closeFile(int fd)
{
    if(isMounted()==false) return -1;

    {
        Lock<Mutex> l(fsMutex);

        //Find file from descriptor
        int index=findSlot(fd);
        if(index==-1) return -1;//Trying to close a nonexistent file

        int result=0;
        //Close file
        if(f_close(files[index].fptr)!=FR_OK) result=-1;

        freeSlot(fd);
        return result;
    }
}

bool Filesystem::isFileOpen(int fd) const
{
    return findSlot(fd)!=-1;
}

int Filesystem::mkdir(const char *path, int mode)
{
    if(isMounted()==false) return -1;
    
    {
        Lock<Mutex> l(fsMutex);
        switch(f_mkdir(path))
        {
            case FR_OK:
                return 0;
            case FR_EXIST://mkdir returns -2 if directory already exists.
                return -2;
            default:
                return -1;
        }
    }
}

int Filesystem::unlink(const char *path)
{
    if(isMounted()==false) return -1;

    {
        Lock<Mutex> l(fsMutex);
        if(f_unlink(path)!=FR_OK) return -1;
        return 0;
    }
}

Filesystem::Filesystem() : mounted(false), fileDescriptorCounter(3)
{
    
}

int Filesystem::findSlot(int fd) const
{
    //Using fd % MAX_OPEN_FILES as a simple hashing function to speed up finding
    //the right element in the files array
    int index=fd % MAX_OPEN_FILES;
    
    for(int i=0;i<MAX_OPEN_FILES;i++)
    {
        if(files[index].isEmpty()==false && files[index].fd==fd)
        {
            return index;
        } else {
            //Collision in the hashtable
            if(++index>=MAX_OPEN_FILES) index=0;
        }
    }
    return -1;
}

int Filesystem::findAndFillEmptySlot()
{
    //Get a new file descriptor
    int fd=newFileDescriptor();
    //Should never happen
    if(fd<3) return -1;

    //Using fd % MAX_OPEN_FILES as a simple hashing function to speed up finding
    //the right element in the files array
    int index=fd % MAX_OPEN_FILES;

    for(int i=0;i<MAX_OPEN_FILES;i++)
    {
        if(files[index].isEmpty())
        {
            files[index].fd=fd; //Filling in the file descriptor
            return index;
        } else {
            //Collision in the hashtable
            if(++index>=MAX_OPEN_FILES) index=0;
        }
    }
    return -1;
}

void Filesystem::freeSlot(int fd)
{
    int index=findSlot(fd);
    if(index==-1) return;

    delete files[index].fptr;
    files[index].fptr=0;
    
    //If there are other files with same hash (conflict) we need to compact them
    int hash=fd % MAX_OPEN_FILES;
    for(int i=0;i<MAX_OPEN_FILES-1;i++)
    {
        int nextIndex=index+1;
        if(nextIndex==MAX_OPEN_FILES) nextIndex=0;
        if(files[nextIndex].isEmpty()==true ||
           files[nextIndex].fd % MAX_OPEN_FILES!=hash) break;
        //There's a conflict
        files[index].fd=files[nextIndex].fd;
        files[index].fptr=files[nextIndex].fptr;
        files[nextIndex].fptr=0;//Mark as empty
        if(++index==MAX_OPEN_FILES) index=0;
    }
}

int Filesystem::newFileDescriptor()
{
    //This overflows after 2147483647 file opened/closed.
    //For now we'll jut leave it as is.
    return fileDescriptorCounter++;
}

//
// Directory class
//

Directory::Directory()
{
    //Do nothing
}

char Directory::open(const char *name)
{
    //If filesystem not mounted, return
    if(Filesystem::instance().isMounted()==false) return 3;
    
    {
        Lock<Mutex> l(fsMutex);
        if(f_opendir(&d,name)!=FR_OK) return 1;
        return 0;
    }
}

bool Directory::next(char *name, unsigned int& size, unsigned char& attrib)
{
    FILINFO fi;
    {
        Lock<Mutex> l(fsMutex);
        if(f_readdir(&d,&fi)!=FR_OK) return false;
    }
    
    //If fi.fname is a null string return
    if(fi.fname[0]==0) return false;
    std::strcpy(name,fi.fname);
    size=fi.fsize;
    attrib=fi.fattrib;
    return true;
}

bool Directory::exists(const char *name)
{
    Directory d;
    if(d.open(name)==0) return true;
    return false;
}

}; //namespace miosix

#endif //WITH_FILESYSTEM
