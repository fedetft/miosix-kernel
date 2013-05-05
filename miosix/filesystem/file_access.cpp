
#include <vector>
#include "file_access.h"

using namespace std;

namespace miosix {

bool FilesystemBase::areAllFilesClosed() { return openFileCount==0; }

void FilesystemBase::fileCloseHook() { atomicAdd(&openFileCount,-1); }

void FilesystemBase::newFileOpened() { atomicAdd(&openFileCount,1); }

//
// class FilesystemBase
//

FilesystemBase::~FilesystemBase() {}

//
// class FileDescriptorTable
//

FileDescriptorTable::FileDescriptorTable(const FileDescriptorTable& rhs)
{
    //No need to lock the mutex since we are in a costructor and there can't
    //be pointers to this in other threads yet
    for(int i=0;i<MAX_OPEN_FILES;i++) this->files[i]=atomic_load(&rhs.files[i]);
}

FileDescriptorTable& FileDescriptorTable::operator=(
        const FileDescriptorTable& rhs)
{
    Lock l(mutex);
    for(int i=0;i<MAX_OPEN_FILES;i++)
        atomic_store(&this->files[i],atomic_load(&rhs.files[i]));
    return *this;
}

int FileDescriptorTable::open(const char* name, int flags, int mode)
{
    if(name==0 || name[0]=='\0') return -EFAULT;
    Lock l(mutex);
    for(int i=0;i<MAX_OPEN_FILES;i++)
    {
        if(files[i]) continue;
        //Found an empty file descriptor
        pair<Filesystem*,string> openData=
                FilesystemManager::instance().resolvePath(absolutePath(name));
        int result=openData->first->open(files[i],openData->second,flags,mode);
        if(result==0) return i; //The file descriptor
        else return result; //The error code
    }
    return -ENFILE;
}

int FileDescriptorTable::close(int fd)
{
    //No need to lock the mutex when deleting
    if(fd<0 || fd>=MAX_OPEN_FILES) return -EBADF;
    intrusive_ref_ptr<FileBase> toClose;
    toClose=atomic_exchange(files+fd,intrusive_ref_ptr<FileBase>());
    if(!toClose) return -EBADF; //File entry was not open
    return 0;
}

int FileDescriptorTable::stat(const char* path, stat* pstat)
{
    if(path==0 || path[0]=='\0') return -EFAULT;
    pair<Filesystem*,string> fs=
            FilesystemManager::instance().resolvePath(absolutePath(path));
    return fs->first->stat(fs->second,pstat);
}

int FileDescriptorTable::chdir(const char* path)
{
    if(path==0 || path[0]=='\0') return -EFAULT;
    Lock l(mutex);
    return -1;
    //TODO: check that path is a real path to directory, make sure it's absolute
}

string FileDescriptorTable::absolutePath(const char* path)
{
    if(path[0]=='/') return path;
    Lock l(mutex);
    return cwd+path;
}

//
// class FilesystemManager
//

FilesystemManager& FilesystemManager::instance()
{
    static FilesystemManager instance;
    return instance;
}

int FilesystemManager::kmount(const char* path, FilesystemBase* fs)
{
    if(path==0 || path[0]=='\0' || fs==0) return -EFAULT;
    Lock l(mutex);
    map<string,FilesystemBase*>::iterator it=filesystems.find(path);
    if(it!=filesystems.end()) return -EBUSY;
    filesystems[path]=fs;
}

int FilesystemManager::umount(const char* path)
{
    if(path==0 || path[0]=='\0') return -ENOENT;
    Lock l(mutex);
    map<string,FilesystemBase*>::iterator it=filesystems.find(path);
    if(it==filesystems.end()) return -EINVAL;
    
    //This finds all the filesystems that have to be recursively umounted
    //to umount the required filesystem. For example, if /path and /path/path2
    //are filesystems, umounting /path must umount also /path/path2
    vector<map<string,FilesystemBase*>::iterator> fsToUmount;
    map<string,FilesystemBase*>::iterator it2;
    for(it2=filesystems.begin();it2!=filesystems.end();++it2)
        if(it2->first.find_first_of(it->first)==0) fsToUmount.push_back(it2);
    
    //Now look into all file descriptor tables if there are open files in the
    //filesystems to umount. If there are, return busy. This is an heavy
    //operation given the way the filesystem data structure is organized, but
    //it has been done like this to minimize the size of an entry in the file
    //descriptor table (4 bytes), and because umount happens infrequently
    list<FileDescriptorTable*>::iterator it3;
    for(it3=fileTables.begin();it3!=fileTables.end();++it3)
    {
        for(int i=0;i<MAX_OPEN_FILES;i++)
        {
            intrusive_ref_ptr<FileBase> file=it3->getFile(i);
            if(!file) continue;
            vector<map<string,FilesystemBase*>::iterator>::iterator it4;
            for(it4=fsToUmount.begin();it4!=fsToUmount.end();++it4)
            {
                if(file->getParent()!=(*it4)->second) continue;
                return -EBUSY;
            }
        }
    }
    
    //Now there should be no more files belonging to the filesystems to umount,
    //but check if it is really so, as there is a possible race condition
    //which is the read/close,umount where one thread performs a read (or write)
    //operation on a file descriptor, it gets preempted and another thread does
    //a close on that descriptor and an umount of the filesystem.
    //In such a case there is no entry in the descriptor table (as close was
    //called) but the operation is still ongoing.
    vector<map<string,FilesystemBase*>::iterator>::iterator it5;
    for(it5=fsToUmount.begin();it5!=fsToUmount.end();++it5)
        if(it5->second.areAllFilesClosed()==false) return -EBUSY;
    
    //It is now safe to umount all filesystems
    for(it5=fsToUmount.begin();it5!=fsToUmount.end();++it5)
    {
        FilesystemBase *toUmount=(*it5)->second;
        filesystems.erase(*it5);
        delete toUmount;
    }
    return 0;
}

pair<FilesystemBase*,string> FilesystemManager::resolvePath(
        const std::string& path)
{
    //see man 2 path_resolution
}

} //namespace miosix
