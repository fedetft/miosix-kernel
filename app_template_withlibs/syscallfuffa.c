
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <reent.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>

void pthread_mutex_unlock() {}
void pthread_mutex_lock() {}
void pthread_mutex_destroy() {}

int __register_exitproc(int type, void (*fn)(void), void *arg, void *d) { return 0; }
void __call_exitprocs(int code, void *d) {}

void *__dso_handle=(void*) &__dso_handle;

void *_sbrk_r(struct _reent *ptr, ptrdiff_t incr) { return (void*)-1; }
void __malloc_lock() {}
void __malloc_unlock() {}
int _open_r(struct _reent *ptr, const char *name, int flags, int mode) { return -1; }
int _close_r(struct _reent *ptr, int fd) { return -1; }

int _write(int fd, const void *buf, size_t cnt)
{
    return write(fd,buf,cnt);
}

int _write_r(struct _reent *ptr, int fd, const void *buf, size_t cnt)
{
    return write(fd,buf,cnt);
}

int _read(int fd, void *buf, size_t cnt)
{
    return read(fd,buf,cnt);
}

int _read_r(struct _reent *ptr, int fd, void *buf, size_t cnt)
{
    return read(fd,buf,cnt);
}

off_t _lseek(int fd, off_t pos, int whence) { return -1; }
off_t _lseek_r(struct _reent *ptr, int fd, off_t pos, int whence) { return -1; }
int _fstat(int fd, struct stat *pstat) { return -1; }
int _fstat_r(struct _reent *ptr, int fd, struct stat *pstat) { return -1; }
int _stat_r(struct _reent *ptr, const char *file, struct stat *pstat) { return -1; }
int isatty(int fd) { return -1; }
int _isatty(int fd) { return -1; }
int mkdir(const char *path, mode_t mode) { return -1; }
int _unlink_r(struct _reent *ptr, const char *file) { return -1; }
clock_t _times_r(struct _reent *ptr, struct tms *tim) { return -1; }
int _link_r(struct _reent *ptr, const char *f_old, const char *f_new) { return -1; }
int _kill(int pid, int sig) { return -1; }
int _kill_r(struct _reent* ptr, int pid, int sig) { return -1; }
int _getpid() { return 1; }
int _getpid_r(struct _reent* ptr) { return 1; }
int _fork_r(struct _reent *ptr) { return -1; }
int _wait_r(struct _reent *ptr, int *status) { return -1; }
