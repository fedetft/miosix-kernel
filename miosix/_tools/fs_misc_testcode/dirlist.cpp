
#include <cstdio>
// #include "miosix.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

using namespace std;
// using namespace miosix;

void printStat(struct dirent *de, struct stat *st)
{
    struct stat st2;
    if(!S_ISDIR(st->st_mode))
    {
        int fd=open(de->d_name,O_RDONLY);
        if(fd<0) perror("open");
        else {
            if(fstat(fd,&st2)) perror("fstat");
            close(fd);
        }
    } else st2.st_ino=st->st_ino; //Can't fstat() an fd to a directory
    printf("%13s iD=%d iS=%d iF=%d dev=%d mode=%#o size=%d\n",
        de->d_name,de->d_ino,st->st_ino,st2.st_ino,st->st_dev,st->st_mode,st->st_size);
}

int main()
{
    char buffer[256];
    for(;;)
    {
        string dir;
        cin>>dir;
        if(chdir(dir.c_str())) perror("chdir");
        printf("cwd: %s\n",getcwd(buffer,sizeof(buffer)));
    
        DIR *d=opendir(".");
        if(d==NULL) perror("opendir");
        else {
            struct dirent *de;
            while(de=readdir(d))
            {
                struct stat st;
                if(stat(de->d_name,&st)<0) perror(de->d_name);
                else printStat(de,&st);
            }
            closedir(d);
        }
    }
    
    for(;;) ;
}
