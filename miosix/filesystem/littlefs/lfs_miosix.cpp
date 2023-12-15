#include "lfs_miosix.h"
#include "filesystem/ioctl.h"
#include "filesystem/stringpart.h"
#include "kernel/logging.h"
#include <fcntl.h>
#include <memory>

// configuration of the filesystem is provided by this struct
const struct lfs_config LITTLEFS_CONFIG = {
    // block device operations
    .read = nullptr,
    .prog = nullptr,
    .erase = nullptr,
    .sync = nullptr,

    // block device configuration
    .read_size = 512,
    .prog_size = 512,
    .block_size = 512,
    .block_cycles = 500,
    .cache_size = 512,
    .lookahead_size = 512,
};

miosix::LittleFS::LittleFS(intrusive_ref_ptr<FileBase> disk)
    : // Put the drive instance into the config context. Note that a raw pointer
      // is passed, but the object is kept alive by the intrusive_ref_ptr in the
      // drv member variable. Hence, the object is deleted when the LittleFS
      // object is deleted.
      context(disk.get()) {
  int err;
  drv = disk;
  config = LITTLEFS_CONFIG;
  config.context = &context;

  config.read = miosix_block_device_read;
  config.prog = miosix_block_device_prog;
  config.erase = miosix_block_device_erase;
  config.sync = miosix_block_device_sync;

  config.lock = miosix_lfs_lock;
  config.unlock = miosix_lfs_unlock;

  err = lfs_mount(&lfs, &config);
  mountError = convert_lfs_error_into_posix(err);
}

int miosix::LittleFS::open(intrusive_ref_ptr<FileBase> &file, StringPart &name,
                           int flags, int mode) {
  if (mountFailed())
    return -ENOENT;

  // Check if we are trying to open a file or a directory
  if (flags & _FDIRECTORY) {
    return open_directory(file, name, flags, mode);
  } else {
    return open_file(file, name, flags, mode);
  }
}

int miosix::LittleFS::open_directory(intrusive_ref_ptr<FileBase> &directory,
                                     StringPart &name, int flags, int mode) {
  auto dir = std::make_unique<lfs_dir_t>();

  int err = lfs_dir_open(&lfs, dir.get(), name.c_str());
  if (err) {
    return convert_lfs_error_into_posix(err);
  }

  directory = intrusive_ref_ptr<LittleFSDirectory>(
      new LittleFSDirectory(intrusive_ref_ptr<LittleFS>(this), std::move(dir)));

  return 0;
}

int miosix::LittleFS::open_file(intrusive_ref_ptr<FileBase> &file,
                                StringPart &name, int flags, int mode) {
  auto lfs_file_obj = std::make_unique<lfs_file_t>();

  int err = lfs_file_open(&lfs, lfs_file_obj.get(), name.c_str(),
                          convert_posix_open_to_lfs_flags(flags));
  if (err) {
    return convert_lfs_error_into_posix(err);
  }

  file = intrusive_ref_ptr<LittleFSFile>(
      new LittleFSFile(intrusive_ref_ptr<LittleFS>(this),
                       std::move(lfs_file_obj), flags & O_SYNC, name));

  return 0;
}

int miosix::LittleFS::lstat(StringPart &name, struct stat *pstat) {
  if (mountFailed())
    return -ENOENT;

  memset(pstat, 0, sizeof(struct stat));
  pstat->st_dev = filesystemId;
  pstat->st_nlink = 1;
  pstat->st_blksize = LITTLEFS_CONFIG.block_size;

  if (name.empty()) {
    // Root directory
    // TODO: Check this is true?
    pstat->st_ino = 1;               // Root inode is 1 by convention
    pstat->st_mode = S_IFDIR | 0755; // drwxr-xr-x
    return 0;
  }

  struct lfs_info lfsStat;

  int err = lfs_stat(&lfs, name.c_str(), &lfsStat);
  if (err)
    return convert_lfs_error_into_posix(err);

  pstat->st_ino = -1; // TODO: Implement inode number on lfs
  pstat->st_mode = lfsStat.type == LFS_TYPE_DIR ? S_IFDIR | 0755  // drwxr-xr-x
                                                : S_IFREG | 0755; // -rwxr-xr-x
  pstat->st_size = lfsStat.size;
  pstat->st_blocks = (lfsStat.size + LITTLEFS_CONFIG.block_size - 1) /
                     LITTLEFS_CONFIG.block_size;

  return 0;
}

int miosix::LittleFS::unlink(StringPart &name) {
  if (mountFailed())
    return -ENOENT;

  int err = lfs_remove(&lfs, name.c_str());
  return convert_lfs_error_into_posix(err);
}

int miosix::LittleFS::rename(StringPart &oldName, StringPart &newName) {
  if (mountFailed())
    return -ENOENT;

  int err = lfs_rename(&lfs, oldName.c_str(), newName.c_str());
  return convert_lfs_error_into_posix(err);
}

int miosix::LittleFS::mkdir(StringPart &name, int mode) {
  if (mountFailed())
    return -ENOENT;

  // TODO: What to do with mode? Ignore it? Check that it is 0755?

  int err = lfs_mkdir(&lfs, name.c_str());
  return convert_lfs_error_into_posix(err);
}

int miosix::LittleFS::rmdir(StringPart &name) {
  if (mountFailed())
    return -ENOENT;

  int err = lfs_remove(&lfs, name.c_str());
  return convert_lfs_error_into_posix(err);
}

miosix::LittleFS::~LittleFS() {
  if (mountFailed())
    return;
  int err = lfs_unmount(&lfs);
  err = convert_lfs_error_into_posix(err);

  if (err)
    bootlog("Unmounted LittleFS with error code %d\n", mountError);
}

int miosix::convert_lfs_error_into_posix(int lfs_err) {
  if (lfs_err) {
    if (lfs_err == LFS_ERR_CORRUPT)
      return -EIO;
    if (lfs_err == LFS_ERR_NOTEMPTY)
      return -ENOTEMPTY;
    if (lfs_err == LFS_ERR_NAMETOOLONG)
      return -ENAMETOOLONG;
  }

  return lfs_err;
}

int miosix::convert_posix_open_to_lfs_flags(int posix_flags) {
  int lfsFlags = 0;

  if ((posix_flags & O_RDONLY) == O_RDONLY)
    lfsFlags |= LFS_O_RDONLY;
  if ((posix_flags & O_WRONLY) == O_WRONLY)
    lfsFlags |= LFS_O_WRONLY;
  if ((posix_flags & O_APPEND) == O_APPEND)
    lfsFlags |= LFS_O_APPEND;
  if ((posix_flags & O_CREAT) == O_CREAT)
    lfsFlags |= LFS_O_CREAT;
  if ((posix_flags & O_TRUNC) == O_TRUNC)
    lfsFlags |= LFS_O_TRUNC;
  if ((posix_flags & O_EXCL) == O_EXCL)
    lfsFlags |= LFS_O_EXCL;
  return lfsFlags;
}

int miosix::LittleFSFile::read(void *buf, size_t count) {
  assert(isOpen());
  // Get the LittleFS driver istance using getParent()
  LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
  auto readSize = lfs_file_read(lfs_driver->getLfs(), file.get(), buf, count);
  return readSize;
}

int miosix::LittleFSFile::write(const void *buf, size_t count) {
  assert(isOpen());
  LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
  auto writeSize = lfs_file_write(lfs_driver->getLfs(), file.get(), buf, count);
  if (forceSync)
    lfs_file_sync(lfs_driver->getLfs(), file.get());
  return writeSize;
}

off_t miosix::LittleFSFile::lseek(off_t pos, int whence) {
  assert(isOpen());

  lfs_whence_flags whence_lfs;
  switch (whence) {
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

  LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
  off_t lfs_off = static_cast<off_t>(
      lfs_file_seek(lfs_driver->getLfs(), file.get(),
                    static_cast<lfs_off_t>(pos), whence_lfs));

  return lfs_off;
}

int miosix::LittleFSFile::fstat(struct stat *pstat) const {
  assert(isOpen());

  LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
  StringPart &nameClone = const_cast<StringPart &>(name);
  lfs_driver->lstat(nameClone, pstat);

  return 0;
}

int miosix::LittleFSDirectory::getdents(void *dp, int len) {
  assert(isOpen());
  if (len < minimumBufferSize) {
    return -EINVAL;
  }

  LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());

  char *bufferBegin = static_cast<char *>(dp);
  char *bufferEnd = bufferBegin + len;
  char *bufferCurPos = bufferBegin; // Current position in the buffer

  int dirReadResult;

  if (directoryTraversalUnfinished) {
    // Last time we did not have enough memory to store all the directory,
    // continue from where we left
    directoryTraversalUnfinished = false;
    if (addEntry(&bufferCurPos, bufferEnd, -10,
                 dirInfo.type == LFS_TYPE_REG ? DT_REG : DT_DIR,
                 StringPart(dirInfo.name)) < 0) {
      // ??? How is it possible that not even a single entry fits the buffer ???
      return -ENOMEM;
    }
  }

  while ((dirReadResult =
              lfs_dir_read(lfs_driver->getLfs(), dir.get(), &dirInfo)) >= 0) {
    if (dirReadResult == 0) {
      // No more entries
      addTerminatingEntry(&bufferCurPos, bufferEnd);
      break;
    }

    // TODO: Add correct inode number instead of `-10`
    if (addEntry(&bufferCurPos, bufferEnd, -10,
                 dirInfo.type == LFS_TYPE_REG ? DT_REG : DT_DIR,
                 StringPart(dirInfo.name)) < 0) {
      // The entry does not fit in the buffer. Signal that we did not finish
      directoryTraversalUnfinished = true;
      return bufferCurPos - bufferBegin;
    }
  };

  // We did not naturally reach the end, but a read error occurred
  if (dirReadResult < 0) {
    return convert_lfs_error_into_posix(dirReadResult);
  }

  return bufferCurPos - bufferBegin;
}

#define GET_DRIVER_FROM_LFS_CONTEXT(config)                                    \
  static_cast<FileBase *>(                                                     \
      static_cast<lfs_driver_context *>(config->context)->disk);

int miosix::miosix_block_device_read(const lfs_config *c, lfs_block_t block,
                                     lfs_off_t off, void *buffer,
                                     lfs_size_t size) {
  FileBase *drv = GET_DRIVER_FROM_LFS_CONTEXT(c);

  if (drv->lseek(static_cast<off_t>(c->block_size * block + off), SEEK_SET) <
      0) {
    return LFS_ERR_IO;
  }
  if (drv->read(buffer, size) != static_cast<ssize_t>(size)) {
    return LFS_ERR_IO;
  }
  return LFS_ERR_OK;
}

int miosix::miosix_block_device_prog(const lfs_config *c, lfs_block_t block,
                                     lfs_off_t off, const void *buffer,
                                     lfs_size_t size) {
  FileBase *drv = GET_DRIVER_FROM_LFS_CONTEXT(c);

  if (drv->lseek(static_cast<off_t>(c->block_size * block + off), SEEK_SET) <
      0) {
    return LFS_ERR_IO;
  }
  if (drv->write(buffer, size) != static_cast<ssize_t>(size)) {
    return LFS_ERR_IO;
  }
  return LFS_ERR_OK;
}

int miosix::miosix_block_device_erase(const lfs_config *c, lfs_block_t block) {
  FileBase *drv = GET_DRIVER_FROM_LFS_CONTEXT(c);

  std::unique_ptr<int> buffer(new int[c->block_size]);
  memset(buffer.get(), 0, c->block_size);

  if (drv->lseek(static_cast<off_t>((block)*c->block_size), SEEK_SET) < 0) {
    return LFS_ERR_IO;
  }
  if (drv->write(buffer.get(), c->block_size) !=
      static_cast<ssize_t>(c->block_size)) {
    return LFS_ERR_IO;
  }

  return LFS_ERR_OK;
}

int miosix::miosix_block_device_sync(const lfs_config *c) {
  FileBase *drv = GET_DRIVER_FROM_LFS_CONTEXT(c);

  if (drv->ioctl(IOCTL_SYNC, nullptr) != 0) {
    return LFS_ERR_IO;
  }
  return -LFS_ERR_OK;
}

#define GET_MUTEX_FROM_LFS_CONTEXT(config)                                     \
  static_cast<miosix::Mutex *>(                                                \
      &static_cast<lfs_driver_context *>(config->context)->mutex)

int miosix::miosix_lfs_lock(const lfs_config *c) {
  GET_MUTEX_FROM_LFS_CONTEXT(c)->lock();
  return LFS_ERR_OK;
}

int miosix::miosix_lfs_unlock(const lfs_config *c) {
  GET_MUTEX_FROM_LFS_CONTEXT(c)->unlock();
  return LFS_ERR_OK;
}
