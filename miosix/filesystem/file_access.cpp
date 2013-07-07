
#include <vector>
#include <climits>
#include "file_access.h"

using namespace std;

namespace miosix {

/*
 * A note on the use of strings in this file. This file uses three string
 * types: C string, C++ std::string and StringPart which is an efficent
 * in-place substring of either a C or C++ string.
 * 
 * The functions which are meant to be used by clients of the filesystem
 * API take file names as C strings. This is becase that's the signature
 * of the POSIX calls, such as fopen(), stat(), umount(), ... It is
 * worth noticing that these C strings are const. However, resolving paths
 * requires a writable scratchpad string to be able to remove
 * useless path components, such as "/./", go backwards when a "/../" is
 * found, and follow symbolic links. To this end, all these functions
 * make a copy of the passed string into a temporary C++ string that
 * will be deallocated as soon as the function returns.
 * Resolving a path, however, requires to scan all its path components
 * one by one, checking if the path up to that point is a symbolic link
 * or a mountpoint of a filesystem.
 * For example, resolving "/home/test/file" requires to make three
 * substrings, "/home", "/home/test" and "/home/test/file". Of course,
 * one can simply use the substr() member function of the std::string
 * class. However, this means making lots of tiny memory allocations on
 * the heap, increasing the RAM footprint to resolve a path. To this end,
 * the StringPart class was introduced, to create fast in-place substring
 * of a string.
 */

/**
 * The result of resolvePath(). This class is not in file_Access.h as
 * resolvePath() is not meant to be called outside of this file.
 */
class ResolvedPath
{
public:
    /**
     * Constructor
     */
    ResolvedPath() : result(-EINVAL), fs(0), off(0) {}
    
    /**
     * Constructor
     * \param result error code
     */
    explicit ResolvedPath(int result) : result(result), fs(0), off(0) {}
    
    /**
     * Constructor
     * \param fs filesystem
     * \param off offset into path where the subpath relative to the current
     * filesystem starts
     */
    ResolvedPath(intrusive_ref_ptr<FilesystemBase> fs, int offset)
            : result(0), fs(fs), off(offset) {}
    
    int result; ///< 0 on success, a negative number on failure
    intrusive_ref_ptr<FilesystemBase> fs; ///< pointer to the filesystem to which the file belongs
    /// path.c_str()+off is a string containing the relative path into the
    /// filesystem for the looked up file
    int off;
};

//
// class FilesystemBase
//

int FilesystemBase::readlink(StringPart& name, string& target)
{
    return -EINVAL; //Default implementation, for filesystems without symlinks
}

bool FilesystemBase::supportsSymlinks() const { return false; }

bool FilesystemBase::areAllFilesClosed() { return openFileCount==0; }

void FilesystemBase::fileCloseHook() { atomicAdd(&openFileCount,-1); }

void FilesystemBase::newFileOpened() { atomicAdd(&openFileCount,1); }

FilesystemBase::~FilesystemBase() {}

//
// class FileDescriptorTable
//

FileDescriptorTable::FileDescriptorTable() : mutex(Mutex::RECURSIVE), cwd("/")
{
    FilesystemManager::instance().addFileDescriptorTable(this);
}

FileDescriptorTable::FileDescriptorTable(const FileDescriptorTable& rhs)
    : mutex(Mutex::RECURSIVE), cwd(rhs.cwd)
{
    //No need to lock the mutex since we are in a constructor and there can't
    //be pointers to this in other threads yet
    for(int i=0;i<MAX_OPEN_FILES;i++) this->files[i]=atomic_load(&rhs.files[i]);
    FilesystemManager::instance().addFileDescriptorTable(this);
}

FileDescriptorTable& FileDescriptorTable::operator=(
        const FileDescriptorTable& rhs)
{
    Lock<Mutex> l(mutex);
    for(int i=0;i<MAX_OPEN_FILES;i++)
        atomic_store(&this->files[i],atomic_load(&rhs.files[i]));
    return *this;
}

int FileDescriptorTable::open(const char* name, int flags, int mode)
{
    if(name==0 || name[0]=='\0') return -EFAULT;
    Lock<Mutex> l(mutex);
    for(int i=0;i<MAX_OPEN_FILES;i++)
    {
        if(files[i]) continue;
        //Found an empty file descriptor
        string path=absolutePath(name);
        if(path.empty()) return -ENAMETOOLONG;
        ResolvedPath openData=FilesystemManager::instance().resolvePath(path);
        if(openData.result<0) return openData.result;
        StringPart sp(path,string::npos,openData.off);
        int result=openData.fs->open(files[i],sp,flags,mode);
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

int FileDescriptorTable::chdir(const char* name)
{
    if(name==0 || name[0]=='\0') return -EFAULT;   
    int len=strlen(name);
    if(name[len-1]!='/') len++; //Reserve room for trailing slash
    Lock<Mutex> l(mutex);
    if(name[0]!='/') len+=cwd.length();
    if(len>PATH_MAX) return -ENAMETOOLONG;
    
    string newCwd;
    newCwd.reserve(len);
    if(name[0]=='/') newCwd=name;
    else {
        newCwd=cwd;
        newCwd+=name;
    }
    ResolvedPath openData=FilesystemManager::instance().resolvePath(newCwd);
    //NOTE: put after resolvePath() as it strips trailing / unless path=="/"
    if(newCwd.length()>1) newCwd+='/';

    if(openData.result<0) return openData.result;
    struct stat st;
    StringPart sp(newCwd,string::npos,openData.off);
    openData.fs->lstat(sp,&st);
    if(!S_ISDIR(st.st_mode)) return -ENOTDIR;
    cwd=newCwd;
    return 0;
}

int FileDescriptorTable::statImpl(const char* name, struct stat* pstat, bool f)
{
    if(name==0 || name[0]=='\0') return -EFAULT;
    string path=absolutePath(name);
    if(path.empty()) return -ENAMETOOLONG;
    ResolvedPath openData=FilesystemManager::instance().resolvePath(path,f);
    if(openData.result<0) return openData.result;
    StringPart sp(path,string::npos,openData.off);
    return openData.fs->lstat(sp,pstat);
}

FileDescriptorTable::~FileDescriptorTable()
{
    FilesystemManager::instance().removeFileDescriptorTable(this);
    //There's no need to lock the mutex and explicitly close files eventually
    //left open, because if there are other threads accessing this while we are
    //being deleted we have bigger problems anyway
}

string FileDescriptorTable::absolutePath(const char* path)
{
    int len=strlen(path);
    if(len>PATH_MAX) return "";
    if(path[0]=='/') return path;
    Lock<Mutex> l(mutex);
    if(len+cwd.length()>PATH_MAX) return "";
    return cwd+path;
}

//
// class StringPart
//

StringPart::StringPart(string& str, unsigned int idx, unsigned int off)
        : str(&str), index(idx), offset(off), saved('\0'), owner(false),
          type(CPPSTR)
{
    if(index==string::npos || index>=str.length()) index=str.length();
    else {
        saved=str[index];
        str[index]='\0';
    }
    offset=min(offset,index);
}

StringPart::StringPart(char* s, unsigned int idx, unsigned int off)
        : cstr(s), index(idx), offset(off), saved('\0'), owner(false),
          type(CSTR)
{
    assert(cstr); //Passed pointer can't be null
    unsigned int len=strlen(cstr);
    if(index==string::npos || index>=len) index=len;
    else {
        saved=cstr[index];
        cstr[index]='\0';
    }
    offset=min(offset,index);
}

StringPart::StringPart(StringPart& rhs, unsigned int idx, unsigned int off)
        : saved('\0'), owner(false), type(rhs.type)
{
    if(type==CSTR) this->cstr=rhs.cstr;
    else this->str=rhs.str;
    if(idx!=string::npos && idx<rhs.length())
    {
        index=rhs.offset+idx;//Make index relative to beginning of original str
        if(type==CPPSTR)
        {
            saved=(*str)[index];
            (*str)[index]='\0';
        } else {
            saved=cstr[index];
            cstr[index]='\0';
        }
    } else index=rhs.index;
    offset=min(rhs.offset+off,index);
}

StringPart::StringPart(const StringPart& rhs)
        : cstr(&saved), index(0), offset(0), saved('\0'), owner(false),
          type(CSTR)
{
    if(rhs.empty()==false) assign(rhs);
}

StringPart& StringPart::operator= (const StringPart& rhs)
{
    if(this==&rhs) return *this; //Self assignment
    //Don't forget that we may own a pointer to someone else's string,
    //so always clear() to detach from a string on assignment!
    clear();
    if(rhs.empty()==false) assign(rhs);
    return *this;
}

bool StringPart::startsWith(const StringPart& rhs) const
{
    if(this->length()<rhs.length()) return false;
    return memcmp(this->c_str(),rhs.c_str(),rhs.length())==0;
}

void StringPart::clear()
{
    if(type==CSTR)
    {
        cstr[index]=saved;//Worst case we'll overwrite terminating \0 with an \0
        if(owner) delete[] cstr;
    } else {
        if(index!=str->length()) (*str)[index]=saved;
        if(owner) delete str;
    }
    cstr=&saved; //Reusing saved as an empty string
    saved='\0';
    index=offset=0;
    owner=false;
    type=CSTR;
}

void StringPart::assign(const StringPart& rhs)
{
    cstr=new char[rhs.length()+1];
    strcpy(cstr,rhs.c_str());
    index=rhs.length();
    offset=0;
    owner=true;
}

/**
 * This class implements the path resolution logic
 */
class PathResolution
{
public:
    /**
     * Constructor
     * \param fs map of all mounted filesystems
     */
    PathResolution(const map<StringPart,intrusive_ref_ptr<FilesystemBase> >& fs)
            : filesystems(fs) {}
    
    /**
     * The main purpose of this class, resolve a path
     * \param path inout parameter with the path to resolve. The rsolved path
     * will be modified in-place in this string. The path must be absolute and
     * start with a "/". The caller is responsible for that.
     * \param followLastSymlink if true, follow last symlink
     * \return a resolved path
     */
    ResolvedPath resolvePath(string& path, bool followLastSymlink);
    
private:
    /**
     * Handle a /../ in a path
     * \param path path string
     * \param slash path[slash] is the / character after the ..
     * \return 0 on success, a negative number on error
     */
    int upPathComponent(string& path, int slash);
    
    /**
     * Handle a normal path component in a path, i.e, a path component
     * that is neither //, /./ or /../
     * \param path path string
     * \param followIfSymlink if true, follow symbolic links
     * \return 0 on success, 1 if a symbolic link was found and followed,
     * or a negative number on error
     */
    int normalPathComponent(string& path, bool followIfSymlink);
    
    /**
     * Follow a symbolic link
     * \param path path string. The relative path into the current filesystem 
     * must be a symbolic link (verified by the caller).
     * \return 0 on success, a negative number on failure
     */
    int followSymlink(string& path);
    
    /**
     * Find to which filesystem this path belongs
     * \param path path string.
     * \return 0 on success, a negative number on failure
     */
    int recursiveFindFs(string& path);
    
    /**
     * Check that last path component is not a forbidden value
     * \return true if the last component is good
     */
    bool checkLastComponent(string& path);
    
    /// Mounted filesystems
    const map<StringPart,intrusive_ref_ptr<FilesystemBase> >& filesystems;
    
    /// Pointer to root filesystem
    intrusive_ref_ptr<FilesystemBase> root;
    
    /// Current filesystem while looking up path
    intrusive_ref_ptr<FilesystemBase> fs;
    
    /// True if current filesystem supports symlinks
    bool syms;
    
    /// path[index] is firs unhandled char
    unsigned int index;
    
    ///  path.substr(indexIntoFs) is relative path to
    ///  current filesystem
    unsigned int indexIntoFs;
    
    /// How many components does the relative path have in current fs
    int depthIntoFs;
    
    /// How many symlinks we've found so far
    int linksFollowed;
    
    /// Maximum number of symbolic links to follow (to avoid endless loops)
    static const int maxLinkToFollow=2;
};

ResolvedPath PathResolution::resolvePath(string& path, bool followLastSymlink)
{
    char rootPath[2]; rootPath[0]='/'; rootPath[1]='\0';//A non-const "/" string 
    map<StringPart,intrusive_ref_ptr<FilesystemBase> >::const_iterator it;
    it=filesystems.find(StringPart(rootPath));
    if(it==filesystems.end()) return ResolvedPath(-ENOENT); //should not happen
    root=fs=it->second;
    syms=fs->supportsSymlinks();
    index=1;       //Skip leading /
    indexIntoFs=0; //Contains leading / NOTE: caller must ensure path[0]=='/'
    depthIntoFs=1;
    linksFollowed=0;
    for(;;)
    {
        unsigned int slash=path.find_first_of('/',index);
        //cout<<path.substr(0,slash)<<endl;
        if(slash==string::npos) //Last component (no trailing /)
        {
            if(checkLastComponent(path)==false) return ResolvedPath(-ENOENT);
            index=path.length()+1; //NOTE: two past the last character
            int result=normalPathComponent(path,followLastSymlink);
            if(result<0) return ResolvedPath(result);
            if(result!=1) return ResolvedPath(fs,indexIntoFs);
        } else if(slash==index)
        {
            //Path component is empty, caused by double slash, remove it
            path.erase(index,1);
        } else if(slash-index==1 && path[index]=='.')
        {
            path.erase(index,2); //Path component is ".", ignore
        } else if(slash-index==2 && path[index]=='.' && path[index+1]=='.')
        {
            int result=upPathComponent(path,slash);
            if(result<0) return ResolvedPath(result);
        } else {
            index=slash+1;
            int result=normalPathComponent(path,true);
            if(result<0) return ResolvedPath(result);
        }
        if(index>=path.length()) //Last component (has trailing /)
        {
            if(path.length()>1) //Remove trailing /, unless path=="/"
                path.erase(path.length()-1); 
            return ResolvedPath(fs,indexIntoFs);
        }
    }
}

int PathResolution::upPathComponent(string& path, int slash)
{
    if(index<=1) return -ENOENT; //root dir has no parent
    unsigned int removeStart=path.find_last_of('/',index-2);
    if(removeStart==string::npos) return -ENOENT; //should not happen
    path.erase(removeStart,slash-removeStart);
    index=removeStart+1;
    if(--depthIntoFs>0) return 0;
    
    //Depth went to zero, escape current filesystem
    return recursiveFindFs(path);
}

int PathResolution::normalPathComponent(string& path, bool followIfSymlink)
{
    if(index<path.length())
    {
        //NOTE: index<path.length() is necessary as for example /dev and
        // /dev/ should resolve to the directory in the root filesystem, not
        //to the root directory of the /dev filesystem
        map<StringPart,intrusive_ref_ptr<FilesystemBase> >::const_iterator it;
        it=filesystems.find(StringPart(path,index-1));
        if(it!=filesystems.end())
        {
            //Jumped to a new filesystem. Not stat-ing the path as we're
            //relying on mount not allowing to mount a filesystem on anything
            //but a directory.
            fs=it->second;
            syms=fs->supportsSymlinks();
            indexIntoFs=index-1;
            depthIntoFs=1;
            return 0;
        }
    }
    if(syms && followIfSymlink)
    {
        struct stat st;
        {
            StringPart sp(path,index-1,indexIntoFs);
            if(int res=fs->lstat(sp,&st)<0) return res;
        }
        if(S_ISLNK(st.st_mode))
        {
            int result=followSymlink(path);
            if(result<0) return result;
            else return 1; //1=found and followed a symlink
        } else if(!S_ISDIR(st.st_mode)) return -ENOENT;
    }
    return 0;
}

int PathResolution::followSymlink(string& path)
{
    if(++linksFollowed>=maxLinkToFollow) return -ELOOP;
    string target;
    {
        StringPart sp(path,index-1,indexIntoFs);
        if(int res=fs->readlink(sp,target)<0) return res;
    }
    if(target.empty()) return -ENOENT; //Should not happen
    if(target[0]=='/')
    {
        //Symlink is absolute
        int newPathLen=target.length()+path.length()-index+1;
        if(newPathLen>PATH_MAX) return -ENAMETOOLONG;
        string newPath;
        newPath.reserve(newPathLen);
        newPath=target;
        if(index<path.length())
        {
            newPath+="/";
            newPath.insert(newPath.length(),path,index,string::npos);
        }
        path.swap(newPath);
        fs=root;
        syms=root->supportsSymlinks();
        index=1;
        indexIntoFs=0;
        depthIntoFs=1;
    } else {
        //Symlink is relative
        int newPathLen=path.length()+target.length()+1;
        if(newPathLen>PATH_MAX) return -ENAMETOOLONG;
        string newPath;
        newPath.reserve(newPathLen);
        newPath.insert(newPath.length(),path,0,index-1);
        newPath+="/";
        newPath+=target;
        if(index<path.length())
        {
            newPath+="/";
            newPath.insert(newPath.length(),path,index,string::npos);
        }
        path.swap(newPath);
    }
    return 0;
}

int PathResolution::recursiveFindFs(string& path)
{
    depthIntoFs=1;
    unsigned int backIndex=index;
    for(;;)
    {
        backIndex=path.find_last_of('/',backIndex-1);
        if(backIndex==string::npos) return -ENOENT; //should not happpen
        if(backIndex==0)
        {
            fs=root;
            indexIntoFs=0;
            break;
        }
        map<StringPart,intrusive_ref_ptr<FilesystemBase> >::const_iterator it;
        it=filesystems.find(StringPart(path,backIndex));
        if(it!=filesystems.end())
        {
            fs=it->second;
            indexIntoFs=backIndex;
            break;
        }
        depthIntoFs++;
    }
    syms=fs->supportsSymlinks();
    return 0;
}

bool PathResolution::checkLastComponent(string& path)
{
    unsigned int componentLen=path.length()-index;
    if(componentLen==1 && path[index]=='.') return false;
    if(componentLen==2 && path[index]=='.' && path[index+1]=='.') return false;
    return true;
}

//
// class FilesystemManager
//

FilesystemManager& FilesystemManager::instance()
{
    static FilesystemManager instance;
    return instance;
}

int FilesystemManager::kmount(const char* path, intrusive_ref_ptr<FilesystemBase> fs)
{
    if(path==0 || path[0]=='\0' || fs==0) return -EFAULT;
    Lock<Mutex> l(mutex);
    //TODO: make sure path exists and mount point is a directory,
    //otherwise mounted fs is inaccessible
    int len=strlen(path);
    if(len>PATH_MAX) return -ENAMETOOLONG;
    string temp(path);
    if(filesystems.insert(make_pair(StringPart(temp),fs)).second==false)
        return -EBUSY; //Means already mounted
    else
        return 0;
}

int FilesystemManager::umount(const char* path, bool force)
{
    typedef
    typename map<StringPart,intrusive_ref_ptr<FilesystemBase> >::iterator fsIt;
    
    if(path==0 || path[0]=='\0') return -ENOENT;
    int len=strlen(path);
    if(len>PATH_MAX) return -ENAMETOOLONG;
    string pathStr(path);
    Lock<Mutex> l(mutex); //A reader-writer lock would be better
    fsIt it=filesystems.find(StringPart(pathStr));
    if(it==filesystems.end()) return -EINVAL;
    
    //This finds all the filesystems that have to be recursively umounted
    //to umount the required filesystem. For example, if /path and /path/path2
    //are filesystems, umounting /path must umount also /path/path2
    vector<fsIt> fsToUmount;
    for(fsIt it2=filesystems.begin();it2!=filesystems.end();++it2)
        if(it2->first.startsWith(it->first)==0) fsToUmount.push_back(it2);
    
    //Now look into all file descriptor tables if there are open files in the
    //filesystems to umount. If there are, return busy. This is an heavy
    //operation given the way the filesystem data structure is organized, but
    //it has been done like this to minimize the size of an entry in the file
    //descriptor table (4 bytes), and because umount happens infrequently.
    //Note that since we are locking the same mutex used by resolvePath(),
    //other threads can't open new files concurrently while we check
    list<FileDescriptorTable*>::iterator it3;
    for(it3=fileTables.begin();it3!=fileTables.end();++it3)
    {
        for(int i=0;i<MAX_OPEN_FILES;i++)
        {
            intrusive_ref_ptr<FileBase> file=(*it3)->getFile(i);
            if(!file) continue;
            vector<fsIt>::iterator it4;
            for(it4=fsToUmount.begin();it4!=fsToUmount.end();++it4)
            {
                if(file->getParent()!=(*it4)->second) continue;
                if(force==false) return -EBUSY;
                (*it3)->close(i); //If forced umount, close the file
            }
        }
    }
    
    //Now there should be no more files belonging to the filesystems to umount,
    //but check if it is really so, as there is a possible race condition
    //which is the read/close,umount where one thread performs a read (or write)
    //operation on a file descriptor, it gets preempted and another thread does
    //a close on that descriptor and an umount of the filesystem. Also, this may
    //happen in case of a forced umount. In such a case there is no entry in
    //the descriptor table (as close was called) but the operation is still
    //ongoing.
    vector<fsIt>::iterator it5;
    const int maxRetry=3; //Retry up to three times
    for(int i=0;i<maxRetry;i++)
    { 
        bool failed=false;
        for(it5=fsToUmount.begin();it5!=fsToUmount.end();++it5)
        {
            if((*it5)->second->areAllFilesClosed()) continue;
            if(force==false) return -EBUSY;
            failed=true;
            break;
        }
        if(!failed) break;
        if(i==maxRetry-1) return -EBUSY; //Failed to umount even if forced
        Thread::sleep(1000); //Wait to see if the filesystem operation completes
    }
    
    //It is now safe to umount all filesystems
    for(it5=fsToUmount.begin();it5!=fsToUmount.end();++it5)
        filesystems.erase(*it5);
    return 0;
}

ResolvedPath FilesystemManager::resolvePath(string& path, bool followLastSymlink)
{
    //see man path_resolution. This code supports arbitrarily mounted
    //filesystems, symbolic links resolution, but no hardlinks to directories
    if(path.length()>PATH_MAX) return ResolvedPath(-ENAMETOOLONG);
    if(path.empty() || path[0]!='/') return ResolvedPath(-ENOENT);

    Lock<Mutex> l(mutex);
    PathResolution pr(filesystems);
    return pr.resolvePath(path,followLastSymlink);
}

} //namespace miosix
