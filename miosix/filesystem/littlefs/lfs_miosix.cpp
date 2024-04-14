/***************************************************************************
 *   Copyright (C) 2023 by Andrea Somaini, Simone Paternoster              *
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

#include "lfs_miosix.h"
#include "filesystem/ioctl.h"
#include "filesystem/stringpart.h"
#include "kernel/logging.h"
#include <fcntl.h>
#include <memory>

namespace miosix {

#ifdef WITH_FILESYSTEM

/**
 * Convert the error flag given by LittleFS into a POSIX error code
 * \param lfs_err the error flag given by LittleFS
 * \return the respective POSIX error code
 */
static int lfsErrorToPosix(int lfs_err);

/**
 * Convert a POSIX open flag into a LittleFS open flag
 * \param posix_flags POSIX open flag
 * \return LittleFS open flag
 */
static int posixOpenToLfsFlags(int posix_flags);

// * Wrappers for LFS block device operations
static int miosixBlockDeviceRead(const struct lfs_config *c, lfs_block_t block,
                             lfs_off_t off, void *buffer, lfs_size_t size);

static int miosixBlockDeviceProg(const struct lfs_config *c, lfs_block_t block,
                             lfs_off_t off, const void *buffer,
                             lfs_size_t size);

static int miosixBlockDeviceErase(const struct lfs_config *c, lfs_block_t block);

static int miosixBlockDeviceSync(const struct lfs_config *c);

// Lock the underlying block device. Negative error codes
// are propagated to the user.
static int miosixLfsLock(const struct lfs_config *c);

// Unlock the underlying block device. Negative error codes
// are propagated to the user.
static int miosixLfsUnlock(const struct lfs_config *c);

class LittleFSFile : public FileBase
{
public:
    /**
     * @brief Constructs a LittleFSFile object.
     *
     * @param parentFS A reference to the parent LittleFS object.
     * \param flags file open flags
     * @param file A pointer to the underlying lfs_file_t object. This object
     * takes ownership of the pointer.
     * @param forceSync A boolean indicating whether to force synchronization
     * after writes
     */
    LittleFSFile(intrusive_ref_ptr<LittleFS> parentFS, int flags,
                 std::unique_ptr<lfs_file_t> file, bool forceSync,
                 StringPart &name)
        : FileBase(parentFS,flags), file(std::move(file)), forceSync(forceSync),
          name(name) {}

    virtual ssize_t write(const void *buf, size_t count) override;
    virtual ssize_t read(void *buf, size_t count) override;
    virtual off_t lseek(off_t pos, int whence) override;
    virtual int ftruncate(off_t size) override;
    virtual int fstat(struct stat *pstat) const override;

    ~LittleFSFile()
    {
        LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
        lfs_file_close(lfs_driver->getLfs(), file.get());
    }

private:
    std::unique_ptr<lfs_file_t> file;

    /// Force the file to be synced on every write
    bool forceSync;
    StringPart name;
};

class LittleFSDirectory : public DirectoryBase
{
public:
    enum Level
    {
        RootDirectory,
        ChildOfRootDirectory,
        OtherDirectory,
    };

    LittleFSDirectory(intrusive_ref_ptr<LittleFS> parentFS,
                      std::unique_ptr<lfs_dir_t> dir,
                      LittleFSDirectory::Level level)
        : DirectoryBase(parentFS), dir(std::move(dir)), level(level) {}

    /**
     * Also directories can be opened as files. In this case, this system call
     * allows to retrieve directory entries.
     * \param dp pointer to a memory buffer where one or more struct dirent
     * will be placed. dp must be four words aligned.
     * \param len memory buffer size.
     * \return the number of bytes read on success, or a negative number on
     * failure.
     */
    virtual int getdents(void *dp, int len);

    ~LittleFSDirectory()
    {
        LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
        lfs_dir_close(lfs_driver->getLfs(), dir.get());
    }

private:
    std::unique_ptr<lfs_dir_t> dir;
    // Used for when `getdents` does not give enough memory to store the whole
    // directory childrens
    bool directoryTraversalUnfinished = false;
    // Level of this directory relative to the root; used by getdents.
    Level level;
    // Last children directory info retrieved
    lfs_info dirInfo;

    // Helper method that adds the current `dirInfo` to a given dents buffer
    int addLastLFSDirEntry(char **pos, char *end);
};

LittleFS::LittleFS(intrusive_ref_ptr<FileBase> disk)
    : // Put the drive instance into the config context. Note that a raw pointer
      // is passed, but the object is kept alive by the intrusive_ref_ptr in the
      // drv member variable. Hence, the object is deleted when the LittleFS
      // object is deleted.
      context(disk.get())
{
    int err;
    drv = disk;

    config = {};
    config.read_size = 512;
    config.prog_size = 512;
    config.block_size = 512;
    config.block_cycles = 500;
    config.cache_size = 512;
    config.lookahead_size = 512;

    config.context = &context;

    config.read = miosixBlockDeviceRead;
    config.prog = miosixBlockDeviceProg;
    config.erase = miosixBlockDeviceErase;
    config.sync = miosixBlockDeviceSync;

    config.lock = miosixLfsLock;
    config.unlock = miosixLfsUnlock;

    err = lfs_mount(&lfs, &config);
    mountError = lfsErrorToPosix(err);
}

int LittleFS::open(intrusive_ref_ptr<FileBase> &file, StringPart &name,
                   int flags, int mode)
{
    if(mountFailed()) return -ENOENT;

    // Check if we are trying to open a file or a directory
    if(flags & _FDIRECTORY) return openDirectory(file, name, flags, mode);
    else return openFile(file, name, flags, mode);
}

int LittleFS::openDirectory(intrusive_ref_ptr<FileBase> &directory,
                            StringPart &name, int flags, int mode)
{
    auto dir = std::make_unique<lfs_dir_t>();

    int err = lfs_dir_open(&lfs, dir.get(), name.c_str());
    if(err) return lfsErrorToPosix(err);
    LittleFSDirectory::Level lvl;
    if(name.empty())
    {
        lvl = LittleFSDirectory::Level::RootDirectory;
    } else if(name.findLastOf('/')==std::string::npos) {
        lvl = LittleFSDirectory::Level::ChildOfRootDirectory;
    } else {
        lvl = LittleFSDirectory::Level::OtherDirectory;
    }

    directory = intrusive_ref_ptr<LittleFSDirectory>(new LittleFSDirectory(
        intrusive_ref_ptr<LittleFS>(this), std::move(dir), lvl)); //FIXME: use shared_from_this

    return 0;
}

int LittleFS::openFile(intrusive_ref_ptr<FileBase> &file,
                       StringPart &name, int flags, int mode)
{
    auto lfs_file_obj = std::make_unique<lfs_file_t>();

    int err = lfs_file_open(&lfs, lfs_file_obj.get(), name.c_str(),
                            posixOpenToLfsFlags(flags));
    if(err) return lfsErrorToPosix(err);

    file = intrusive_ref_ptr<LittleFSFile>(
        new LittleFSFile(intrusive_ref_ptr<LittleFS>(this), //FIXME: use shared_from_this
                         flags, std::move(lfs_file_obj), flags & O_SYNC, name));

    return 0;
}

int LittleFS::lstat(StringPart &name, struct stat *pstat)
{
    if(mountFailed()) return -ENOENT;

    memset(pstat, 0, sizeof(struct stat));
    pstat->st_dev = filesystemId;
    pstat->st_nlink = 1;
    pstat->st_blksize = config.block_size;

    if(name.empty())
    {
        // Root directory
        pstat->st_ino = 1;               // Root inode is 1 by convention
        pstat->st_mode = S_IFDIR | 0755; // drwxr-xr-x
        return 0;
    }

    struct lfs_info lfsStat;

    int err = lfs_stat(&lfs, name.c_str(), &lfsStat);
    if(err) return lfsErrorToPosix(err);

    pstat->st_ino = lfsStat.block;
    pstat->st_mode = lfsStat.type == LFS_TYPE_DIR ? S_IFDIR | 0755  // drwxr-xr-x
                                                  : S_IFREG | 0755; // -rwxr-xr-x
    pstat->st_size = lfsStat.size;
    //NOTE: st_blocks should be number of 512 byte blocks regardless of st_blksize
    pstat->st_blocks = (lfsStat.size + 512 - 1) / 512;

    return 0;
}

int LittleFS::truncate(StringPart& name, off_t size)
{
    //LittleFs does not have a truncate, so we need to open the file and ftruncate
    intrusive_ref_ptr<FileBase> file;
    if(int result=open(file,name,O_WRONLY,0)) return result;
    return file->ftruncate(size);
}

int LittleFS::unlink(StringPart &name)
{
    if(mountFailed()) return -ENOENT;

    int err = lfs_remove(&lfs, name.c_str());
    return lfsErrorToPosix(err);
}

int LittleFS::rename(StringPart &oldName, StringPart &newName)
{
    if(mountFailed()) return -ENOENT;

    int err = lfs_rename(&lfs, oldName.c_str(), newName.c_str());
    return lfsErrorToPosix(err);
}

int LittleFS::mkdir(StringPart &name, int mode)
{
    if(mountFailed()) return -ENOENT;

    int err = lfs_mkdir(&lfs, name.c_str());
    return lfsErrorToPosix(err);
}

int LittleFS::rmdir(StringPart &name)
{
    if(mountFailed()) return -ENOENT;

    int err = lfs_remove(&lfs, name.c_str());
    return lfsErrorToPosix(err);
}

LittleFS::~LittleFS()
{
    if(mountFailed()) return;
    int err = lfs_unmount(&lfs);
    err = lfsErrorToPosix(err);

    if(err) bootlog("Unmounted LittleFS with error code %d\n", mountError);
}

int lfsErrorToPosix(int lfs_err)
{
    //TODO double check
    if(lfs_err)
    {
        if(lfs_err == LFS_ERR_CORRUPT) return -EIO;
        if(lfs_err == LFS_ERR_NOTEMPTY) return -ENOTEMPTY;
        if(lfs_err == LFS_ERR_NAMETOOLONG) return -ENAMETOOLONG;
    }

    return lfs_err;
}

int posixOpenToLfsFlags(int posix_flags)
{
    int lfsFlags = 0;

    //TODO: find more optimized way
    if((posix_flags & O_RDONLY) == O_RDONLY) lfsFlags |= LFS_O_RDONLY;
    if((posix_flags & O_WRONLY) == O_WRONLY) lfsFlags |= LFS_O_WRONLY;
    if((posix_flags & O_RDWR)   == O_RDWR)   lfsFlags |= LFS_O_RDWR;
    if((posix_flags & O_APPEND) == O_APPEND) lfsFlags |= LFS_O_APPEND;
    if((posix_flags & O_CREAT) == O_CREAT)   lfsFlags |= LFS_O_CREAT;
    if((posix_flags & O_TRUNC) == O_TRUNC)   lfsFlags |= LFS_O_TRUNC;
    if((posix_flags & O_EXCL) == O_EXCL)     lfsFlags |= LFS_O_EXCL;
    return lfsFlags;
}

ssize_t LittleFSFile::write(const void *buf, size_t count)
{
    LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
    auto result=lfs_file_write(lfs_driver->getLfs(), file.get(), buf, count);
    if(forceSync) lfs_file_sync(lfs_driver->getLfs(), file.get());
    if(result>=0) return result;
    return lfsErrorToPosix(result);
}

ssize_t LittleFSFile::read(void *buf, size_t count)
{
    // Get the LittleFS driver instance using getParent()
    LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
    auto result=lfs_file_read(lfs_driver->getLfs(), file.get(), buf, count);
    if(result>=0) return result;
    return lfsErrorToPosix(result);
}

off_t LittleFSFile::lseek(off_t pos, int whence)
{
    lfs_whence_flags whence_lfs;
    switch(whence)
    {
        case SEEK_CUR:
            whence_lfs = LFS_SEEK_CUR;
            break;
        case SEEK_SET:
            whence_lfs = LFS_SEEK_SET;
            break;
        case SEEK_END:
            whence_lfs = LFS_SEEK_END;
            break;
        default:
            return -EINVAL;
            break;
    }

    //TODO: check seek past the end behavior
    LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
    off_t result = static_cast<off_t>(
        lfs_file_seek(lfs_driver->getLfs(), file.get(),
                      static_cast<lfs_off_t>(pos), whence_lfs));
    if(result>=0) return result;
    return lfsErrorToPosix(result);
}

int LittleFSFile::ftruncate(off_t size)
{
    LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
    int err=lfs_file_truncate(lfs_driver->getLfs(),file.get(),
                                 static_cast<lfs_off_t>(size));
    return lfsErrorToPosix(err);
}

int LittleFSFile::fstat(struct stat *pstat) const
{
    LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
    StringPart &nameClone = const_cast<StringPart &>(name);
    lfs_driver->lstat(nameClone, pstat);

    return 0;
}

int LittleFSDirectory::getdents(void *dp, int len)
{
    if(len < minimumBufferSize) return -EINVAL;

    LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());

    char *bufferBegin = static_cast<char *>(dp);
    char *bufferEnd = bufferBegin + len;
    char *bufferCurPos = bufferBegin; // Current position in the buffer

    if(!directoryTraversalUnfinished && level == RootDirectory)
    {
        // LittleFS does not provide . and .. entries for the root directory,
        // we need to add them manually
        int myIno = 1;
        int parentIno = lfs_driver->parentFsMountpointInode;
        addDefaultEntries(&bufferCurPos, myIno, parentIno);
    }

    if(directoryTraversalUnfinished)
    {
        // Last time we did not have enough memory to store all the directory,
        // continue from where we left
        directoryTraversalUnfinished = false;
        if(addLastLFSDirEntry(&bufferCurPos, bufferEnd) < 0)
        {
            // If we arrive here, somebody is being really stingy with the bytes!
            // TODO: Are we really sure we shouldn't give them another chance?
            return -EINVAL;
        }
    }

    int dirReadResult = lfs_dir_read(lfs_driver->getLfs(), dir.get(), &dirInfo);
    while(dirReadResult > 0)
    {
        if(addLastLFSDirEntry(&bufferCurPos, bufferEnd) < 0)
        {
            // The entry does not fit in the buffer. Signal that we did not finish
            directoryTraversalUnfinished = true;
            return bufferCurPos - bufferBegin;
        }
        dirReadResult = lfs_dir_read(lfs_driver->getLfs(), dir.get(), &dirInfo);
    }

    if(dirReadResult < 0)
    {
        // We did not naturally reach the end, but a read error occurred
        return lfsErrorToPosix(dirReadResult);
    }

    // dirReadResult == 0; no more entries
    addTerminatingEntry(&bufferCurPos, bufferEnd);
    return bufferCurPos - bufferBegin;
}

int LittleFSDirectory::addLastLFSDirEntry(char **pos, char *end)
{
    int ino;
    if(level == ChildOfRootDirectory && strcmp(dirInfo.name, "..") == 0) ino = 1;
    else ino = dirInfo.block;

    int type = dirInfo.type == LFS_TYPE_REG ? DT_REG : DT_DIR;
    return addEntry(pos, end, ino, type, dirInfo.name);
}

#define GET_DRIVER_FROM_LFS_CONTEXT(config)                                    \
  static_cast<FileBase *>(                                                     \
      static_cast<lfs_driver_context *>(config->context)->disk);

int miosixBlockDeviceRead(const lfs_config *c, lfs_block_t block,
                          lfs_off_t off, void *buffer, lfs_size_t size)
{
    FileBase *drv = GET_DRIVER_FROM_LFS_CONTEXT(c);

    if(drv->lseek(static_cast<off_t>(c->block_size * block + off), SEEK_SET) < 0)
    {
        return LFS_ERR_IO;
    }
    if(drv->read(buffer, size) != static_cast<ssize_t>(size))
    {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

int miosixBlockDeviceProg(const lfs_config *c, lfs_block_t block,
                          lfs_off_t off, const void *buffer, lfs_size_t size)
{
    FileBase *drv = GET_DRIVER_FROM_LFS_CONTEXT(c);

    if(drv->lseek(static_cast<off_t>(c->block_size * block + off), SEEK_SET) < 0)
    {
        return LFS_ERR_IO;
    }
    if(drv->write(buffer, size) != static_cast<ssize_t>(size))
    {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

int miosixBlockDeviceErase(const lfs_config *c, lfs_block_t block)
{
    return LFS_ERR_OK;
}

int miosixBlockDeviceSync(const lfs_config *c)
{
    FileBase *drv = GET_DRIVER_FROM_LFS_CONTEXT(c);

    if (drv->ioctl(IOCTL_SYNC, nullptr) != 0) return LFS_ERR_IO;
    return -LFS_ERR_OK;
}

#define GET_MUTEX_FROM_LFS_CONTEXT(config)  \
  static_cast<Mutex *>(&static_cast<lfs_driver_context *>(config->context)->mutex)

int miosixLfsLock(const lfs_config *c)
{
    GET_MUTEX_FROM_LFS_CONTEXT(c)->lock();
    return LFS_ERR_OK;
}

int miosixLfsUnlock(const lfs_config *c)
{
    GET_MUTEX_FROM_LFS_CONTEXT(c)->unlock();
    return LFS_ERR_OK;
}

#endif //WITH_FILESYSTEM

} //namespace miosix
