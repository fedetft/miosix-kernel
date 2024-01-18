
#include <cstdio>
#include <fcntl.h>
#include <dirent.h>
#include "miosix.h"
#include "filesystem/file_access.h"
#include "filesystem/fat32/fat32.h"

using namespace std;
using namespace miosix;

void fail(const char *err)
{
    puts(err);
    exit(1);
}

void loopMount(const char *imageFile, const char *mountPoint)
{
    // Get an instance to the filesystem manager
    FilesystemManager& fsm=FilesystemManager::instance();
    
    // Resolve the file path to get a pointer to filesystem and relative path
    string path=imageFile;
    ResolvedPath openData=fsm.resolvePath(path);
    if(openData.result<0) fail("Image file not found");
    StringPart relativePath(path,string::npos,openData.off);
    
    // Open the file to get an instance of a FileBase
    intrusive_ref_ptr<FileBase> image;
    if(openData.fs->open(image,relativePath,O_RDWR,0)<0)
        fail("Can't open image file");
    
    // Create a new FAT32 filesystem object
    intrusive_ref_ptr<Fat32Fs> diskFs(new Fat32Fs(image));
    if(diskFs->mountFailed()) fail("Can't create FAT32 fs from image file");
    
    // Finally, mount the image file to a mountpoint
    mkdir(mountPoint,0755);
    if(fsm.kmount(mountPoint,diskFs)!=0) fail("Can't mount image file");
}

int main()
{
    // Loop mount /sd/disk.dd to /sd/disk
    // /sd/disk.dd file must exist and be a FAT32 disk image
    loopMount("/sd/disk.dd","/sd/disk");
    
    // Now you can read and write to the disk image
    puts("Listing content of disk image root dir...");
    DIR *d=opendir("/sd/disk");
    if(d==NULL) fail("Can't open directory");
    struct dirent *de;
    while((de=readdir(d)))
    {
        puts(de->d_name);
    }
    closedir(d);
    puts("Ok");
    
    puts("Appending to file.txt in the disk image...");
    FILE *f=fopen("/sd/disk/file.txt","a");
    if(f==NULL) fail("Can't open file");
    fiprintf(f,"Test line\n");
    fclose(f);
    puts("Ok");
    
    // Finally, umount the filesystem
    FilesystemManager& fsm=FilesystemManager::instance();
    if(fsm.umount("/sd/disk")<0) fail("Can't umount filesystem");
    puts("Shutting down");
}
