#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define N(x) printf(#x"=%d\n",(unsigned int)x)
#define O(x) printf(#x"=%#o\n",(unsigned int)x)
#define T(x) { struct tm tt; localtime_r(&x,&tt); \
  char ts[256]; strftime(ts,sizeof(ts),"%c",&tt); \
  printf(#x"=%s\n",ts); }
#define E(x) if(x<0) { perror(#x); return 1; }

void pst(struct stat st)
{
	N(major(st.st_dev)); N(minor(st.st_dev));
	N(st.st_ino);
	O(st.st_mode);
	N(st.st_nlink);
	N(st.st_uid);
	N(st.st_gid);
	N(major(st.st_rdev)); N(minor(st.st_rdev));
	N(st.st_size);
	N(st.st_blksize);
	N(st.st_blocks);
	T(st.st_atime);
	T(st.st_mtime);
	T(st.st_ctime);
}

int main(int argc, char *argv[])
{
	if(argc!=2) return 1;
	struct stat st;
	E(lstat(argv[1],&st));
	puts("=== lstat()"); pst(st);

	if(!S_ISLNK(st.st_mode)) return 0;
	char target[1024];
	int len=readlink(argv[1],target,sizeof(target));
	if(len<0) { perror("readlink"); return 1; }
	target[len]='\0';
	printf("symlink to %s\n\n",target);
	E(stat(argv[1],&st));
	puts("=== stat()"); pst(st);
}

/*
Run on a standard file on ext4
./xstat file
=== lstat()
major(st.st_dev)=252
minor(st.st_dev)=0
st.st_ino=786606
st.st_mode=0100664
st.st_nlink=1
st.st_uid=1000
st.st_gid=1000
major(st.st_rdev)=0
minor(st.st_rdev)=0
st.st_size=88
st.st_blksize=4096
st.st_blocks=8
st.st_atime=Mon May  6 23:21:25 2013
st.st_mtime=Mon May  6 23:21:25 2013
st.st_ctime=Mon May  6 23:21:25 2013


Run on a standard file with an hardlink on ext4
./xstat file2
=== lstat()
major(st.st_dev)=252
minor(st.st_dev)=0
st.st_ino=786606
st.st_mode=0100664
st.st_nlink=2
st.st_uid=1000
st.st_gid=1000
major(st.st_rdev)=0
minor(st.st_rdev)=0
st.st_size=88
st.st_blksize=4096
st.st_blocks=8
st.st_atime=Mon May  6 23:22:42 2013
st.st_mtime=Mon May  6 23:21:25 2013
st.st_ctime=Mon May  6 23:22:42 2013


Run on a symlink to a regular file on ext4
./xstat file3
=== lstat()
major(st.st_dev)=252
minor(st.st_dev)=0
st.st_ino=786604
st.st_mode=0120777
st.st_nlink=1
st.st_uid=1000
st.st_gid=1000
major(st.st_rdev)=0
minor(st.st_rdev)=0
st.st_size=4
st.st_blksize=4096
st.st_blocks=0
st.st_atime=Mon May  6 23:19:10 2013
st.st_mtime=Mon May  6 23:19:09 2013
st.st_ctime=Mon May  6 23:19:09 2013
symlink to file
=== stat()
major(st.st_dev)=252
minor(st.st_dev)=0
st.st_ino=786606
st.st_mode=0100664
st.st_nlink=1
st.st_uid=1000
st.st_gid=1000
major(st.st_rdev)=0
minor(st.st_rdev)=0
st.st_size=88
st.st_blksize=4096
st.st_blocks=8
st.st_atime=Mon May  6 23:22:42 2013
st.st_mtime=Mon May  6 23:21:25 2013
st.st_ctime=Mon May  6 23:23:11 2013


Run on a symlink to a symlink to a regular file on ext4
./xstat file4
=== lstat()
major(st.st_dev)=252
minor(st.st_dev)=0
st.st_ino=786607
st.st_mode=0120777
st.st_nlink=1
st.st_uid=1000
st.st_gid=1000
major(st.st_rdev)=0
minor(st.st_rdev)=0
st.st_size=5
st.st_blksize=4096
st.st_blocks=0
st.st_atime=Mon May  6 23:19:34 2013
st.st_mtime=Mon May  6 23:19:33 2013
st.st_ctime=Mon May  6 23:19:33 2013
symlink to file3
=== stat()
major(st.st_dev)=252
minor(st.st_dev)=0
st.st_ino=786606
st.st_mode=0100664
st.st_nlink=1
st.st_uid=1000
st.st_gid=1000
major(st.st_rdev)=0
minor(st.st_rdev)=0
st.st_size=88
st.st_blksize=4096
st.st_blocks=8
st.st_atime=Mon May  6 23:22:42 2013
st.st_mtime=Mon May  6 23:21:25 2013
st.st_ctime=Mon May  6 23:23:11 2013


Run on a directory with 3 files on ext4
./xstat dir
=== lstat()
major(st.st_dev)=252
minor(st.st_dev)=0
st.st_ino=397253
st.st_mode=040775
st.st_nlink=2
st.st_uid=1000
st.st_gid=1000
major(st.st_rdev)=0
minor(st.st_rdev)=0
st.st_size=4096
st.st_blksize=4096
st.st_blocks=8
st.st_atime=Mon May  6 23:32:02 2013
st.st_mtime=Mon May  6 23:32:01 2013
st.st_ctime=Mon May  6 23:32:01 2013


Run on a directory on which /dev is mounted
./xstat /dev
=== lstat()
major(st.st_dev)=0
minor(st.st_dev)=5
st.st_ino=1025
st.st_mode=040755
st.st_nlink=18
st.st_uid=0
st.st_gid=0
major(st.st_rdev)=0
minor(st.st_rdev)=0
st.st_size=4360
st.st_blksize=4096
st.st_blocks=0
st.st_atime=Mon May  6 23:17:51 2013
st.st_mtime=Mon May  6 23:17:48 2013
st.st_ctime=Mon May  6 23:17:48 2013


Run on a device file
./xstat /dev/ttyUSB0 
=== lstat()
major(st.st_dev)=0
minor(st.st_dev)=5
st.st_ino=157801
st.st_mode=020660
st.st_nlink=1
st.st_uid=0
st.st_gid=20
major(st.st_rdev)=188
minor(st.st_rdev)=0
st.st_size=0
st.st_blksize=4096
st.st_blocks=0
st.st_atime=Mon May  6 23:17:48 2013
st.st_mtime=Mon May  6 23:17:48 2013
st.st_ctime=Mon May  6 23:17:48 2013


Run on /dev/null
./xstat /dev/null
=== lstat()
major(st.st_dev)=0
minor(st.st_dev)=5
st.st_ino=1029
st.st_mode=020666
st.st_nlink=1
st.st_uid=0
st.st_gid=0
major(st.st_rdev)=1
minor(st.st_rdev)=3
st.st_size=0
st.st_blksize=4096
st.st_blocks=0
st.st_atime=Mon May  6 19:51:31 2013
st.st_mtime=Mon May  6 19:51:31 2013
st.st_ctime=Mon May  6 19:51:31 2013


Run on a directory on which an usb key is mounted (fat32 formatted)
./xstat /media/A563-201B
=== lstat()
major(st.st_dev)=8
minor(st.st_dev)=17
st.st_ino=1
st.st_mode=040700
st.st_nlink=2
st.st_uid=1000
st.st_gid=1000
major(st.st_rdev)=0
minor(st.st_rdev)=0
st.st_size=4096
st.st_blksize=4096
st.st_blocks=8
st.st_atime=Thu Jan  1 01:00:00 1970
st.st_mtime=Thu Jan  1 01:00:00 1970
st.st_ctime=Thu Jan  1 01:00:00 1970


Run on a file inside a fat32 filesystem
./xstat /media/A563-201B/fatfile
=== lstat()
major(st.st_dev)=8
minor(st.st_dev)=17
st.st_ino=10
st.st_mode=0100644
st.st_nlink=1
st.st_uid=1000
st.st_gid=1000
major(st.st_rdev)=0
minor(st.st_rdev)=0
st.st_size=6
st.st_blksize=4096
st.st_blocks=8
st.st_atime=Mon May  6 23:34:03 2013
st.st_mtime=Mon May  6 23:34:03 2013
st.st_ctime=Mon May  6 23:34:03 2013

*/