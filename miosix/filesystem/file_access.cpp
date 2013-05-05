
#include "file_access.h"

using namespace std;

namespace miosix {

//
// class FilesystemBase
//

void FilesystemBase::fileCloseHook() {}

FilesystemBase::~FilesystemBase() {}

//
// class Filesystem
//

void Filesystem::newFileOpened()
{
    Lock l(mutex); //TODO: atomic ops?
    openFilesCount++;
}

void Filesystem::waitUntilNoOpenFiles()
{
    Lock l(mutex);
    while(openFilesCount!=0) cond.wait(l);
    return 0;
}

void Filesystem::fileCloseHook()
{
    Lock l(mutex); //TODO: atomic ops?
    if(--openFilesCount==0) cond.broadcast();
}

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
    for(int i=0;i<MAX_OPEN_FILES;i++) this->files[i]=atomic_load(&rhs.files[i]);
    return *this;
}

int FileDescriptorTable::open(const char* name, int flags, int mode)
{
    Lock l(mutex);
    for(int i=0;i<MAX_OPEN_FILES;i++)
    {
        if(files[i]) continue;
        //Found an empty file descriptor
        
        //TODO
        
    }
    return -ENFILE;
}

int FileDescriptorTable::close(int fd)
{
    Lock l(mutex);
    if(fd<0 || fd>=MAX_OPEN_FILES) return -EBADF;
    intrusive_ref_ptr<FileBase> toClose;
    toClose=atomic_exchange(files+fd,intrusive_ref_ptr<FileBase>());
    if(!toClose) return -EBADF; //File entry was not open
    return 0;
}

void FileDescriptorTable::removeIfFromThisFilesystem(const Filesystem* fs)
{
    Lock l(mutex);
    for(int i=0;i<MAX_OPEN_FILES;i++)
        if(files[i] && files[i]->getParent()==fs) files[i]=0;
}

FileDescriptorTable::~FileDescriptorTable()
{
    //No need to lock the mutex because if there are other threads accessing
    //this while we are being deleted we have bigger problems anyway
    for(int i=0;i<MAX_OPEN_FILES;i++) files[i]=0;
}

//
// class FilesystemManager
//

FilesystemManager& FilesystemManager::instance()
{
    static FilesystemManager instance;
    return instance;
}

int FilesystemManager::umount(const char* path)
{
    Lock l(mutex);
    map<string,FilesystemBase*>::iterator it=filesystems.find(path);
    if(it==filesystems.end()) return -ENOENT;
    FilesystemBase *toUmount=it->second;

    //FIXME: if /path and /path/path2 are filesystems, umounting /path has to umount /path2..

    //First, remove the filesystem from the table, so that no more files
    //can be opened from this filesystem
    filesystems.erase(it);
    
    //Then invalidate all entries of open files in this filesystem from the
    //file descriptor tables. Note that this is an expensive operation for
    //how the filesystem data structure is organized, but has been done in
    //this way to minimize the memory occupied by an entry in the file
    //descriptor table (4 bytes)
    list<FileDescriptorTable*>::iterator it;
    for(it=fileTables.begin();it!=fileTables.end();++it)
        it->removeIfFromThisFilesystem(toUmount);
    
    //Ok, that was easy, now comes the hard part, to allow all file operations
    //but open and close to run concurrently, there may be threads with cached
    //file objects into the filesystem to be removed, so we need to wait until
    //all these operations complete before we can actually umount the filesystem
    int result=toUmount->waitUntilNoOpenFiles();
    
    if(result!=0)
    {
        //Failed to umount, all entries in the file tables remain removed, but
        //the filesystem entry must be put back int the list of mounted
        //filesystems
        filesystems[path]=toUmount;
        return result;
    }
    
    //Successfully umounted
    delete toUmount;
    return 0;
}

pair<FilesystemBase*,string> FilesystemManager::resolvePath(const char *path)
{
    
}

FilesystemManager::FilesystemManager() : mutex(Mutex::RECURSIVE) {}

} //namespace miosix
