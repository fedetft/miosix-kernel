#include "lfs_miosix.h"

miosix::LittleFS::LittleFS(intrusive_ref_ptr<FileBase> disk) {
  isMountFailed = true;
}

int miosix::LittleFS::open(intrusive_ref_ptr<FileBase> &file, StringPart &name,
                           int flags, int mode) {
  return EPERM;
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

miosix::LittleFS::~LittleFS() {}
