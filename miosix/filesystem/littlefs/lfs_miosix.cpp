#include "lfs_miosix.h"
#include "filesystem/stringpart.h"
#include "kernel/logging.h"
#include <fcntl.h>

// TODO: Remove this
#define LFS_MIOSIX_IN_RAM

#ifdef LFS_MIOSIX_IN_RAM
#include "bd/lfs_rambd.c"
#include "bd/lfs_rambd.h"
#endif

// configuration of the filesystem is provided by this struct
const struct lfs_config LITTLEFS_CONFIG = {
    // block device operations
    .read = nullptr,
    .prog = nullptr,
    .erase = nullptr,
    .sync = nullptr,

    // TODO: Check this are OK for Miosix
    // block device configuration
    .read_size = 16,
    .prog_size = 16,
    .block_size = 4096,
    .block_count = 128,
    .block_cycles = 500,
    .cache_size = 16,
    .lookahead_size = 16,
};

struct lfs_rambd_config LFS_FILE_CONFIG = {
    .read_size = 16,
    .prog_size = 16,
    .erase_size = 4096,
    .erase_count = 128,
};

miosix::LittleFS::LittleFS(intrusive_ref_ptr<FileBase> disk) {
  int err;
  drv = disk;
  config = LITTLEFS_CONFIG;
#ifdef LFS_MIOSIX_IN_RAM
  config.read = lfs_rambd_read;
  config.prog = lfs_rambd_prog;
  config.erase = lfs_rambd_erase;
  config.sync = lfs_rambd_sync;
  config.context = &this->rambd;
  err = lfs_rambd_create(&config, &LFS_FILE_CONFIG);
  if (err) {
    bootlog("Error creating filebd: %d\n", err);
    mountError = convert_lfs_error_into_posix(err);
    return;
  }
  err = lfs_format(&lfs, &config);
  if (err) {
    bootlog("Error formatting LittleFS: %d\n", err);
    mountError = convert_lfs_error_into_posix(err);
    return;
  }
#else
  // Put the drive instance into the config context. Note that a raw pointer is
  // passed, but the object is kept alive by the intrusive_ref_ptr in the drv
  // member variable. Hence, the object is deleted when the LittleFS object is
  // deleted.
  config.context = drv.get();
  config.read = miosix_block_device_read;
  config.prog = miosix_block_device_prog;
  config.erase = miosix_block_device_erase;
  config.sync = miosix_block_device_sync;
#endif
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
  directory = intrusive_ref_ptr<LittleFSDirectory>(
      new LittleFSDirectory(intrusive_ref_ptr<LittleFS>(this)));

  int err = lfs_dir_open(
      &lfs,
      static_cast<LittleFSDirectory *>(directory.get())->getDirReference(),
      name.c_str());
  if (err) {
    directory.reset();
    return convert_lfs_error_into_posix(err);
  }

  static_cast<LittleFSDirectory *>(directory.get())->setAsOpen();
  return 0;
}

int miosix::LittleFS::open_file(intrusive_ref_ptr<FileBase> &file,
                                StringPart &name, int flags, int mode) {
  file = intrusive_ref_ptr<LittleFSFile>(
      new LittleFSFile(intrusive_ref_ptr<LittleFS>(this), flags & O_SYNC));

  int err = lfs_file_open(
      &lfs, static_cast<LittleFSFile *>(file.get())->getFileReference(),
      name.c_str(), convert_posix_open_to_lfs_flags(flags));
  if (err) {
    file.reset();
    return convert_lfs_error_into_posix(err);
  }
  static_cast<LittleFSFile *>(file.get())->setAsOpen();

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
#ifdef LFS_MIOSIX_IN_RAM
  lfs_rambd_destroy(&config);
#endif
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
  assert(isOpen);
  // Get the LittleFS driver istance using getParent()
  LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
  auto readSize = lfs_file_read(lfs_driver->getLfs(), &file, buf, count);
  return readSize;
}

int miosix::LittleFSFile::write(const void *buf, size_t count) {
  assert(isOpen);
  LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
  auto writeSize = lfs_file_write(lfs_driver->getLfs(), &file, buf, count);
  if (forceSync)
    lfs_file_sync(lfs_driver->getLfs(), &file);
  return writeSize;
}

off_t miosix::LittleFSFile::lseek(off_t pos, int whence) {
  assert(isOpen);
  // TODO: Implement using lfs APIs
  return -ENOENT;
}

int miosix::LittleFSFile::fstat(struct stat *pstat) const {
  assert(isOpen);
  // TODO: Implement using lfs APIs
  return -ENOENT;
}

int miosix::LittleFSDirectory::getdents(void *dp, int len) {
  assert(isOpen);
  if (len < minimumBufferSize) {
    return -EINVAL;
  }

  LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
  lfs_info dirInfo;

  char *bufferBegin = static_cast<char *>(dp);
  char *bufferEnd = bufferBegin + len;
  char *bufferCurPos = bufferBegin; // Current position in the buffer

  int dirReadResult;
  while ((dirReadResult = lfs_dir_read(lfs_driver->getLfs(), &dir, &dirInfo)) >=
         0) {
    if (dirReadResult == 0) {
      // No more entries
      if (addTerminatingEntry(&bufferCurPos, bufferEnd) < 0) {
        // The terminating entry does not fit in the buffer
        return -ENOMEM;
      }
      break;
    }

    // TODO: Add correct inode number instead of `-10`
    if (addEntry(&bufferCurPos, bufferEnd, -10,
                 dirInfo.type == LFS_TYPE_REG ? DT_REG : DT_DIR,
                 StringPart(dirInfo.name)) < 0) {
      // The entry does not fit in the buffer
      return -ENOMEM;
    }
  };

  // We did not naturally reach the end, but a read error occurred
  if (dirReadResult < 0) {
    return convert_lfs_error_into_posix(dirReadResult);
  }

  return bufferCurPos - bufferBegin;
}

int miosix::miosix_block_device_read(const lfs_config *c, lfs_block_t block,
                                     lfs_off_t off, void *buffer,
                                     lfs_size_t size) {
  // TODO: Implement using MIOSIX APIs
  //FileBase *drv = static_cast<FileBase *>(c->context);
  return -ENOENT;
}

int miosix::miosix_block_device_prog(const lfs_config *c, lfs_block_t block,
                                     lfs_off_t off, const void *buffer,
                                     lfs_size_t size) {
  // TODO: Implement using MIOSIX APIs
  //FileBase *drv = static_cast<FileBase *>(c->context);
  return -ENOENT;
}

int miosix::miosix_block_device_erase(const lfs_config *c, lfs_block_t block) {
  // TODO: Implement using MIOSIX APIs
  //FileBase *drv = static_cast<FileBase *>(c->context);
  return -ENOENT;
}

int miosix::miosix_block_device_sync(const lfs_config *c) {
  // TODO: Implement using MIOSIX APIs
  //FileBase *drv = static_cast<FileBase *>(c->context);
  return -ENOENT;
}
