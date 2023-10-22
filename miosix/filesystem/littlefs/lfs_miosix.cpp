#include "lfs_miosix.h"
#include "filesystem/stringpart.h"

#include <fcntl.h>

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

miosix::LittleFS::LittleFS(intrusive_ref_ptr<FileBase> disk) {
  drv = disk;
  struct lfs_config config = LITTLEFS_CONFIG;
  // Put the drive instance into the config context. Note that a raw pointer is
  // passed, but the object is kept alive by the intrusive_ref_ptr in the drv
  // member variable. Hence, the object is deleted when the LittleFS object is
  // deleted.
  config.context = drv.get();
  config.read = miosix_block_device_read;
  config.prog = miosix_block_device_prog;
  config.erase = miosix_block_device_erase;
  config.sync = miosix_block_device_sync;

  int err = lfs_mount(&lfs, &config);
  mountError = convert_lfs_error_into_posix(err);
}

int miosix::LittleFS::open(intrusive_ref_ptr<FileBase> &file, StringPart &name,
                           int flags, int mode) {
  if (mountFailed())
    return -ENOENT;

  // Convert MIOSIX flags to LITTLEFS flags
  int lfsFlags = 0;

  if (flags & O_RDONLY)
    lfsFlags |= LFS_O_RDONLY;
  if (flags & O_WRONLY)
    lfsFlags |= LFS_O_WRONLY;
  if (flags & O_APPEND)
    lfsFlags |= LFS_O_APPEND;
  if (flags & O_CREAT)
    lfsFlags |= LFS_O_CREAT;
  if (flags & O_TRUNC)
    lfsFlags |= LFS_O_TRUNC;
  if (flags & O_EXCL)
    lfsFlags |= LFS_O_EXCL;

  struct lfs_file littlefsFile;
  int err = lfs_file_open(&lfs, &littlefsFile, name.c_str(), flags);

  if (err) {
    return convert_lfs_error_into_posix(err);
  }

  file = intrusive_ref_ptr<LittleFSFile>(new LittleFSFile(
      intrusive_ref_ptr<LittleFS>(this), littlefsFile, flags & O_SYNC));
  return 0;
}

int miosix::LittleFS::lstat(StringPart &name, struct stat *pstat) {
  return EPERM;
}

int miosix::LittleFS::unlink(StringPart &name) { return EPERM; }

int miosix::LittleFS::rename(StringPart &oldName, StringPart &newName) {
  return EPERM;
}

int miosix::LittleFS::mkdir(StringPart &name, int mode) { return EPERM; }

int miosix::LittleFS::rmdir(StringPart &name) { return EPERM; }

miosix::LittleFS::~LittleFS() {
  // Implement close
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

int miosix::LittleFSFile::read(void *buf, size_t count) {
  // Get the LittleFS driver istance using getParent()
  LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
  auto readSize = lfs_file_read(lfs_driver->getLfs(), &file, buf, count);
  return readSize;
}

int miosix::LittleFSFile::write(const void *buf, size_t count) {
  LittleFS *lfs_driver = static_cast<LittleFS *>(getParent().get());
  auto writeSize = lfs_file_write(lfs_driver->getLfs(), &file, buf, count);
  if (forceSync)
    lfs_file_sync(lfs_driver->getLfs(), &file);
  return writeSize;
}

off_t miosix::LittleFSFile::lseek(off_t pos, int whence) {
  // TODO: Implement using lfs APIs
  return -ENOENT;
}

int miosix::LittleFSFile::fstat(struct stat *pstat) const {
  // TODO: Implement using lfs APIs
  return -ENOENT;
}

int miosix::miosix_block_device_read(const lfs_config *c, lfs_block_t block,
                                     lfs_off_t off, void *buffer,
                                     lfs_size_t size) {
  // TODO: Implement using MIOSIX APIs
  FileBase *drv = static_cast<FileBase *>(c->context);
  return -ENOENT;
}

int miosix::miosix_block_device_prog(const lfs_config *c, lfs_block_t block,
                                     lfs_off_t off, const void *buffer,
                                     lfs_size_t size) {
  // TODO: Implement using MIOSIX APIs
  FileBase *drv = static_cast<FileBase *>(c->context);
  return -ENOENT;
}

int miosix::miosix_block_device_erase(const lfs_config *c, lfs_block_t block) {
  // TODO: Implement using MIOSIX APIs
  FileBase *drv = static_cast<FileBase *>(c->context);
  return -ENOENT;
}

int miosix::miosix_block_device_sync(const lfs_config *c) {
  // TODO: Implement using MIOSIX APIs
  FileBase *drv = static_cast<FileBase *>(c->context);
  return -ENOENT;
}
