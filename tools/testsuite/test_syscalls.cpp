/***************************************************************************
 *   Copyright (C) 2008 - 2021 by Terraneo Federico                        *
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
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

// WARNING: this file must be included from testsuite.cpp

//Filesystem test functions
#ifdef WITH_FILESYSTEM
static void fs_test_1();
static void fs_test_2();
static void fs_test_3();
static void fs_test_4();
static void fs_test_5();
static void fs_test_6();
static void fs_test_7();
#if defined(WITH_POSIX_PERMISSIONS) || defined(IN_PROCESS)
static void fs_test_8();
#endif
static void sys_test_pipe();
#endif //WITH_FILESYSTEM
static void sys_test_time();
static void sys_test_getpid();
static void sys_test_isatty();
#ifdef WITH_PROCESSES
static void sys_test_spawn();
#ifdef IN_PROCESS
static void proc_test_global_ctor_dtor();
#endif
#endif

#ifdef IN_PROCESS
void test_syscalls(bool withPermissions)
#else
void test_syscalls()
#endif
{
    #ifndef IN_PROCESS
    ledOn();
    #endif
    #ifdef WITH_FILESYSTEM
    fs_test_1();
    fs_test_2();
    fs_test_3();
    fs_test_4();
    fs_test_5();
    fs_test_6();
    fs_test_7();
    #ifdef WITH_POSIX_PERMISSIONS
    fs_test_8();
    #elif defined(IN_PROCESS)
    if (withPermissions) fs_test_8();
    #endif
    sys_test_pipe();
    #else //WITH_FILESYSTEM
    iprintf("Filesystem tests skipped, filesystem support is disabled\n");
    #endif //WITH_FILESYSTEM
    sys_test_time();
    sys_test_getpid();
    sys_test_isatty();
    #ifdef WITH_PROCESSES
    sys_test_spawn();
    #ifdef IN_PROCESS
    proc_test_global_ctor_dtor();
    #endif
    #endif
    #ifndef IN_PROCESS
    ledOff();
    Thread::sleep(500);//Ensure all threads are deleted.
    #endif
    iprintf("\n*** All tests were successful\n\n");
}

int spawnAndWait(const char *arg[])
{
    const char *env[] = { nullptr };
    return spawnAndWait(arg,env);
}

int spawnAndWait(const char* arg[], const char* env[])
{
    pid_t pid;
    int ec=posix_spawn(&pid,arg[0],NULL,NULL,(char* const*)arg,(char* const*)env);
    if(ec!=0) fail("spawnAndWait posix_spawn");
    pid_t pid2=waitpid(pid,&ec,0);
    if(pid!=pid2) fail("spawnAndWait wrong pid from wait");
    return ec;
}

pid_t spawnWithPipe(const char *arg[], int& pipeFdOut)
{
    //TODO: use posix_spawn arguments to set the stdout fd of the child
    const char *env[] = { nullptr };
    //Replace stdout with the write end of a pipe:
    //dup() stdout to save it
    int oldStdout=dup(STDOUT_FILENO);
    if(oldStdout==-1) fail("spawnWithPipe dup");
    //Create the pipe
    int pipeFds[2];
    int res=pipe(pipeFds);
    if(res!=0) {printf("errno=%d\n", errno); fail("spawnWithPipe pipe");}
    //Do the actual replacement of the stdout fd
    res=dup2(pipeFds[1],STDOUT_FILENO);
    if(res==-1) fail("spawnWithPipe dup2 (set)");
    //Close the original file write-end descriptor
    close(pipeFds[1]);
    //Spawn the process which will inherit the pipe's write end as the stdout
    pid_t pid;
    int ec=posix_spawn(&pid,arg[0],NULL,NULL,(char* const*)arg,(char* const*)env);
    if(ec!=0) fail("spawnWithPipe posix_spawn");
    //Restore stdout
    res=dup2(oldStdout,STDOUT_FILENO);
    if(res==-1) fail("spawnWithPipe dup2 (restore)");
    //Close dup'd stdout
    res=close(oldStdout);
    if(res!=0) fail("spawnWithPipe close (stdout duplicate)");
    //Return read end of the pipe and child pid
    pipeFdOut=pipeFds[0];
    return pid;
}

#ifdef WITH_FILESYSTEM
//
// Filesystem test 1
//
/*
tests:
mkdir()
fopen()
fclose()
fread()
fwrite()
fprintf()
fgets()
fgetc()
fseek()
ftell()
remove()
Also tests concurrent write by opening and writing 3 files from 3 threads
*/
static volatile bool fs_1_error;

static void fs_t1_p1(void *argv)
{
    FILE *f;
    if((f=fopen("/sd/testdir/file_1.txt","w"))==NULL)
    {
        iprintf("Failed on fopen (errno=%d)\n", errno);
        fs_1_error=true;
        return;
    }
    setbuf(f,NULL);
    char *buf=new char[512];
    memset(buf,'1',512);
    int i,j;
    for(i=0;i<512;i++)
    {
        j=fwrite(buf,1,512,f);
        if(j!=512)
        {
            iprintf("Written %d bytes instead of 512\n",j);
            delete[] buf;
            fs_1_error=true;
            fclose(f);
            return;
        }
    }
    delete[] buf;
    if(fclose(f)!=0)
    {
        fs_1_error=true;
        return;
    }
}

static void fs_t1_p2(void *argv)
{
    FILE *f;
    if((f=fopen("/sd/testdir/file_2.txt","w"))==NULL)
    {
        fs_1_error=true;
        return;
    }
    setbuf(f,NULL);
    char *buf=new char[512];
    memset(buf,'2',512);
    int i,j;
    for(i=0;i<512;i++)
    {
        j=fwrite(buf,1,512,f);
        if(j!=512)
        {
            iprintf("Written %d bytes instead of 512\n",j);
            delete[] buf;
            fs_1_error=true;
            fclose(f);
            return;
        }
    }
    delete[] buf;
    if(fclose(f)!=0)
    {
        fs_1_error=true;
        return;
    }
}

static void fs_t1_p3(void *argv)
{
    FILE *f;
    if((f=fopen("/sd/testdir/file_3.txt","w"))==NULL)
    {
        fs_1_error=true;
        return;
    }
    setbuf(f,NULL);
    char *buf=new char[512];
    memset(buf,'3',512);
    int i,j;
    for(i=0;i<512;i++)
    {
        j=fwrite(buf,1,512,f);
        if(j!=512)
        {
            iprintf("Written %d bytes instead of 512\n",j);
            delete[] buf;
            fs_1_error=true;
            fclose(f);
            return;
        }
    }
    delete[] buf;
    if(fclose(f)!=0)
    {
        fs_1_error=true;
        return;
    }
}

static void fs_test_1()
{
    test_name("C standard library file functions + mkdir()");
    iprintf("Please wait (long test)\n");
    //Test mkdir (if possible)
    int result=mkdir("/sd/testdir",0755);
    switch(result)
    {
            case 0: break;
            case -1:
                if(errno==EEXIST)
                {
                    iprintf("Directory test not made because directory"
                        " already exists\n");
                    break;
                } //else fallthrough
            default:
                iprintf("mkdir returned %d\n",result);
                fail("Directory::mkdir()");
    }
    #ifndef IN_PROCESS
    //Test concurrent file write access
    fs_1_error=false;
    Thread *t1=Thread::create(fs_t1_p1,2048+512,DEFAULT_PRIORITY,nullptr,Thread::JOINABLE);
    Thread *t2=Thread::create(fs_t1_p2,2048+512,DEFAULT_PRIORITY,nullptr,Thread::JOINABLE);
    Thread *t3=Thread::create(fs_t1_p3,2048+512,DEFAULT_PRIORITY,nullptr,Thread::JOINABLE);
    t1->join();
    t2->join();
    t3->join();
    if(fs_1_error) fail("Concurrent write");
    #else
    //TODO: threads are not supported inside processes, run tests serially
    fs_t1_p1(NULL);
    fs_t1_p2(NULL);
    fs_t1_p3(NULL);
    if(fs_1_error) fail("write");
    #endif
    //Testing file read
    char *buf=new char[1024];
    int i,j,k;
    FILE *f;
    //file_1.txt
    if((f=fopen("/sd/testdir/file_1.txt","r"))==NULL)
        fail("can't open file_1.txt");
    setbuf(f,NULL);
    i=0;
    for(;;)
    {
        j=fread(buf,1,1024,f);
        if(j==0) break;
        i+=j;
        for(k=0;k<j;k++) if(buf[k]!='1')
            fail("read or write error on file_1.txt");
    }
    if(i!=512*512) fail("file_1.txt : size error");
    if(fclose(f)!=0) fail("Can't close file file_1.txt");
    //file_2.txt
    if((f=fopen("/sd/testdir/file_2.txt","r"))==NULL)
        fail("can't open file_2.txt");
    setbuf(f,NULL);
    i=0;
    for(;;)
    {
        j=fread(buf,1,1024,f);
        if(j==0) break;
        i+=j;
        for(k=0;k<j;k++) if(buf[k]!='2')
            fail("read or write error on file_2.txt");
    }
    if(i!=512*512) fail("file_2.txt : size error");
    if(fclose(f)!=0) fail("Can't close file file_2.txt");
    //file_3.txt
    if((f=fopen("/sd/testdir/file_3.txt","r"))==NULL)
        fail("can't open file_3.txt");
    setbuf(f,NULL);
    i=0;
    for(;;)
    {
        j=fread(buf,1,1024,f);
        if(j==0) break;
        i+=j;
        for(k=0;k<j;k++) if(buf[k]!='3')
            fail("read or write error on file_3.txt");
    }if(i!=512*512) fail("file_3.txt : size error");
    if(fclose(f)!=0) fail("Can't close file file_3.txt");
    delete[] buf;
    //Testing fprintf
    if((f=fopen("/sd/testdir/file_4.txt","w"))==NULL)
        fail("can't open w file_4.txt");
    fprintf(f,"Hello world line 001\n");
    if(fclose(f)!=0) fail("Can't close w file_4.txt");
    //Testing append
    if((f=fopen("/sd/testdir/file_4.txt","a"))==NULL)
        fail("can't open a file_4.txt");
    for(i=2;i<=128;i++) fprintf(f,"Hello world line %03d\n",i);
    if(fclose(f)!=0) fail("Can't close a file_4.txt");
    //Reading to check (only first 2 lines)
    if((f=fopen("/sd/testdir/file_4.txt","r"))==NULL)
        fail("can't open r file_4.txt");
    char line[80];
    fgets(line,sizeof(line),f);
    if(strcmp(line,"Hello world line 001\n")) fail("file_4.txt line 1 error");
    fgets(line,sizeof(line),f);
    if(strcmp(line,"Hello world line 002\n")) fail("file_4.txt line 2 error");
    if(fclose(f)!=0) fail("Can't close r file_4.txt");
    //Test fseek and ftell. When reaching this point file_4.txt contains:
    //Hello world line 001\n
    //Hello world line 002\n
    //  ...
    //Hello world line 128\n
    if((f=fopen("/sd/testdir/file_4.txt","r"))==NULL)
        fail("can't open r2 file_4.txt");
    if(ftell(f)!=0) fail("File opend but cursor not @ address 0");
    fseek(f,-4,SEEK_END);//Seek to 128\n
    if((fgetc(f)!='1')|(fgetc(f)!='2')|(fgetc(f)!='8')) fail("fgetc SEEK_END");
    if(ftell(f)!=(21*128-1))
    {
        iprintf("ftell=%ld\n",ftell(f));
        fail("ftell() 1");
    }
    fseek(f,21+17,SEEK_SET);//Seek to 002\n
    if((fgetc(f)!='0')|(fgetc(f)!='0')|(fgetc(f)!='2')|(fgetc(f)!='\n'))
        fail("fgetc SEEK_SET");
    if(ftell(f)!=(21*2))
    {
        iprintf("ftell=%ld\n",ftell(f));
        fail("ftell() 2");
    }
    fseek(f,21*50+17,SEEK_CUR);//Seek to 053\n
    if((fgetc(f)!='0')|(fgetc(f)!='5')|(fgetc(f)!='3')|(fgetc(f)!='\n'))
        fail("fgetc SEEK_CUR");
    if(ftell(f)!=(21*53))
    {
        iprintf("ftell=%ld\n",ftell(f));
        fail("ftell() 2");
    }
    if(fclose(f)!=0) fail("Can't close r2 file_4.txt");
    //Testing remove()
    if((f=fopen("/sd/testdir/deleteme.txt","w"))==NULL)
        fail("can't open deleteme.txt");
    if(fclose(f)!=0) fail("Can't close deleteme.txt");
    remove("/sd/testdir/deleteme.txt");
    if((f=fopen("/sd/testdir/deleteme.txt","r"))!=NULL) fail("remove() error");
    pass();
}

//
// Filesystem test 2
//
/*
tests:
mkdir
rmdir
unlink
rename
*/

/**
 * \param d scan the content of this directory
 * \param a if not null, this file must be in the directory
 * \param b if not null, this file most not be in the directory
 * \return true on success, false on failure
 */
static bool checkDirContent(const std::string& d, const char *a, const char *b)
{
    DIR *dir=opendir(d.c_str());
    if (dir==NULL)
    {
        return false;
    }
    bool found=false;
    for(;;)
    {
        struct dirent *de=readdir(dir);
        if(de==NULL) break;
        if(a && !strcasecmp(de->d_name,a)) found=true;
        if(b && !strcasecmp(de->d_name,b))
        {
            closedir(dir);
            return false;
        }
    }
    closedir(dir);
    if(a) return found; else return true;
}

static void checkInDir(const std::string& d, bool createFile)
{
    using namespace std;
    const char dirname1[]="test1";
    if(mkdir((d+dirname1).c_str(),0755)!=0) fail("mkdir");
    if(checkDirContent(d,dirname1,0)==false) fail("mkdir 2");
    const char dirname2[]="test2";
    if(rename((d+dirname1).c_str(),(d+dirname2).c_str())) fail("rename");
    if(checkDirContent(d,dirname2,dirname1)==false) fail("rename 2");
    if(rmdir((d+dirname2).c_str())) fail("rmdir");
    if(checkDirContent(d,0,dirname2)==false) fail("rmdir 2");
    
    if(createFile==false) return;
    const char filename1[]="test.txt";
    FILE *f;
    if((f=fopen((d+filename1).c_str(),"w"))==NULL) fail("fopen");
    const char teststr[]="Testing\n";
    fputs(teststr,f);
    fclose(f);
    if(checkDirContent(d,filename1,0)==false) fail("fopen 2");
    const char filename2[]="test2.txt";
    if(rename((d+filename1).c_str(),(d+filename2).c_str())) fail("rename 3");
    if(checkDirContent(d,filename2,filename1)==false) fail("rename 4");
    if((f=fopen((d+filename2).c_str(),"r"))==NULL) fail("fopen");
    char s[32];
    fgets(s,sizeof(s),f);
    if(strcmp(s,teststr)) fail("file content after rename");
    fclose(f);
    if(unlink((d+filename2).c_str())) fail("unlink 3");
    if(checkDirContent(d,0,filename2)==false) fail("unlink 4");
}

static void fs_test_2()
{
    test_name("mkdir/rmdir/rename/unlink");
    checkInDir("/",false);
    DIR *d=opendir("/sd");
    if(d!=NULL)
    {
        // the /sd mountpoint exists, check mkdir/rename/unlink also here
        closedir(d);
        checkInDir("/sd/",true);
    }
    pass();
}

//
// Filesystem test 3
//
/*
tests:
correctness of write/read on large files
*/

static void fs_test_3()
{
    test_name("Large file check");
    iprintf("Please wait (long test)\n");
    
    const char name[]="/sd/testdir/file_5.dat";
    const unsigned int size=1024;
    const unsigned int numBlocks=2048;
    
    FILE *f;
    if((f=fopen(name,"w"))==NULL) fail("open 1");
    setbuf(f,NULL);
    char *buf=new char[size];
    unsigned short checksum=0;
    for(unsigned int i=0;i<numBlocks;i++)
    {
        for(unsigned int j=0;j<size;j++) buf[j]=rand() & 0xff;
        checksum^=crc16(buf,size);
        if(fwrite(buf,1,size,f)!=size) fail("write");
    }
    if(fclose(f)!=0) fail("close 1");

    if((f=fopen(name,"r"))==NULL) fail("open 2");
    setbuf(f,NULL);
    unsigned short outChecksum=0;
    for(unsigned int i=0;i<numBlocks;i++)
    {
        memset(buf,0,size);
        if(fread(buf,1,size,f)!=size) fail("read");
        outChecksum^=crc16(buf,size);
    }
    if(fclose(f)!=0) fail("close 2");
    delete[] buf;
    
    if(checksum!=outChecksum) fail("checksum");
    pass();
}

//
// Filesystem test 4
//
/*
tests:
opendir
readdir
chdir
getcwd
stat/fstat
*/

ino_t checkInodes(const char *dir, ino_t curInode,
        unsigned int parentInode, short curDev, short parentDev)
{
    size_t getcwdBufSz=256;//strlen(dir)+1;
    char *getcwdBuf=new char[getcwdBufSz];
    if(!getcwdBuf) fail("getcwd buffer new");
    char *getcwdRes=getcwd(getcwdBuf,getcwdBufSz);
    if(getcwdRes!=getcwdBuf) fail("getcwd result (1)");
    if(strcmp(getcwdBuf,"/")!=0) fail("getcwd (1)");

    if(chdir(dir)) fail("chdir");
    
    getcwdRes=getcwd(getcwdBuf,getcwdBufSz);
    if(getcwdRes!=getcwdBuf) fail("getcwd result (1)");
    if(strcmp(getcwdBuf,dir)!=0) fail("getcwd (2)");
    delete[] getcwdBuf;
    
    DIR *d=opendir(".");
    if(d==NULL) fail("opendir");
    puts(dir);
    std::set<ino_t> inodes;
    unsigned int result=0;
    for(;;)
    {
        struct dirent *de=readdir(d);
        if(de==NULL) break;
        
        struct stat st;
        if(stat(de->d_name,&st)) fail("stat");
        printf("inode=%llu dev=%d mode=%lo uid=%d gid=%d %s\n",st.st_ino,
            st.st_dev,st.st_mode,st.st_uid,st.st_gid,de->d_name);
        if(de->d_ino!=st.st_ino) fail("inode mismatch");
        
        bool mustBeDir=false;
        if(!strcmp(de->d_name,"."))
        {
            mustBeDir=true;
            if(st.st_ino!=curInode) fail(". inode");
            if(st.st_dev!=curDev) fail("cur dev");
        } else if(!strcmp(de->d_name,"..")) {
            mustBeDir=true;
            if(st.st_ino!=parentInode) fail(".. inode");
            if(st.st_dev!=parentDev) fail("parent dev");
        } else if(!strcasecmp(de->d_name,"testdir")) { 
            mustBeDir=true;
            result=st.st_ino;
            if(st.st_dev!=curDev) fail("parent dev");
        } else if(st.st_dev!=curDev) fail("cur dev");
        
        if(mustBeDir)
        {
            if(de->d_type!=DT_DIR) fail("d_type");
            if(!S_ISDIR(st.st_mode)) fail("st_mode");
        } else {
            if(!S_ISDIR(st.st_mode))
            {
                int fd=open(de->d_name,O_RDONLY);
                if(fd<0) fail("open");
                struct stat st2;
                if(fstat(fd,&st2)) fail("fstat");
                close(fd);
                if(memcmp(&st,&st2,sizeof(struct stat)))
                    fail("stat/fstat mismatch");
            }
        }
        
        if((de->d_type==DT_DIR) ^ (S_ISDIR(st.st_mode))) fail("dir mismatch");
        
        if(st.st_dev==curDev)
        {
            if(inodes.insert(st.st_ino).second==false) fail("duplicate inode");
        }
    }
    closedir(d);
    
    if(chdir("/")) fail("chdir");
    return result;
}

static void fs_test_4()
{
    test_name("Directory listing");
    ino_t curInode=0, parentInode=0, binFsInode=0, sdInode=0;
    short curDevice=0, binDevice=0, sdDevice=0;
    #ifdef WITH_DEVFS
    ino_t devFsInode=0;
    short devDevice=0;
    #endif
    DIR *d=opendir("/");
    if(d==NULL) fail("opendir");
    puts("/");
    for(;;)
    {
        struct dirent *de=readdir(d);
        if(de==NULL) break;
        //de->d_ino may differ from st.st_ino across mountpoints, such as /dev
        //and /sd as one is the inode of the covered directory, and the other
        //the inode of the covering one.
        //The same happens on Linux and many other UNIX based OSes
        struct stat st;
        if(strcmp(de->d_name,"..")) //Don't stat ".."
        {
            if(stat(de->d_name,&st)) fail("stat");
            if((de->d_type==DT_DIR) ^ (S_ISDIR(st.st_mode)))
                fail("dir mismatch");
        }
        if(!strcmp(de->d_name,"."))
        {
            if(de->d_type!=DT_DIR) fail("d_type");
            if(de->d_ino!=st.st_ino) fail("inode mismatch");
            curInode=st.st_ino;
            curDevice=st.st_dev;
        } else if(!strcmp(de->d_name,"..")) {
            if(de->d_type!=DT_DIR) fail("d_type");
            st.st_ino=de->d_ino; //Not stat-ing ".."
            st.st_dev=curDevice;
            parentInode=de->d_ino;
        } else if(!strcmp(de->d_name,"dev")) {
            #ifdef WITH_DEVFS
            if(de->d_type!=DT_DIR) fail("d_type");
            devFsInode=st.st_ino;
            devDevice=st.st_dev;
            #else //WITH_DEVFS
            fail("dev mountpoint exists but WITH_DEVFS is not configured");
            #endif
        } else if(!strcmp(de->d_name,"bin")) {
            if(de->d_type!=DT_DIR) fail("d_type");
            binFsInode=st.st_ino;
            binDevice=st.st_dev;
        } else if(!strcmp(de->d_name,"sd")) {
            if(de->d_type!=DT_DIR) fail("d_type");
            sdInode=st.st_ino;
            sdDevice=st.st_dev;
        }
        
        printf("inode=%llu dev=%d mode=%lo uid=%d gid=%d %s\n",st.st_ino,
            st.st_dev,st.st_mode,st.st_uid,st.st_gid,de->d_name);
    }
    closedir(d);
    
    if(curInode!=parentInode) fail("/..");
    
    #ifdef WITH_DEVFS
    if(devFsInode==0 || devDevice==0) fail("dev");
    checkInodes("/dev",devFsInode,curInode,devDevice,curDevice);
    #endif //WITH_DEVFS
    ///bin may not exist, so if they're both zero it's not an error
    if((binFsInode==0) ^ (binDevice==0)) fail("bin");
    if(binFsInode!=0) checkInodes("/bin",binFsInode,curInode,binDevice,curDevice);
    if(sdInode==0 || sdDevice==0) fail("sd");
    ino_t testdirIno=checkInodes("/sd",sdInode,curInode,sdDevice,curDevice);
    if(testdirIno==0) fail("no testdir");
    checkInodes("/sd/testdir",testdirIno,sdInode,sdDevice,sdDevice);
    pass();
}


//
// Filesystem test 5
//
/*
tests:
POSIX behavior when seeking past the end
*/

static void fs_t5_p1(int nChunks)
{
    const char file[]="/sd/seek.dat";
    constexpr int chunkSize=32;
    char chunk[chunkSize];
    int fd;
    off_t pos;
    size_t count;
    //Remove previous file if present
    fd=open(file,O_RDONLY);
    if(fd>=0)
    {
        close(fd);
        if(unlink(file)!=0) fail("unlink");
    }
    //Open for writing, seek past the end, don't write anything, check file size
    fd=open(file,O_WRONLY|O_CREAT,0777);
    if(fd<0) fail("open 1");
    pos=lseek(fd,nChunks*chunkSize,SEEK_SET);
    if(pos!=nChunks*chunkSize) fail("lseek");
    pos=lseek(fd,0,SEEK_END);
    if(pos!=0) fail("lseek modified size");
    close(fd);
    //Open for reading, seek past the end, check file size again
    fd=open(file,O_RDONLY);
    if(fd<0) fail("open");
    pos=lseek(fd,nChunks*chunkSize,SEEK_SET);
    if(pos!=nChunks*chunkSize) fail("lseek");
    count=read(fd,chunk,chunkSize);
    if(count!=0) fail("read");
    pos=lseek(fd,0,SEEK_END);
    if(pos!=0) fail("lseek modified size");
    close(fd);
    //Open for writing, seek past the end and write, check file size
    fd=open(file,O_WRONLY|O_CREAT,0777);
    if(fd<0) fail("open");
    pos=lseek(fd,nChunks*chunkSize,SEEK_SET);
    if(pos!=nChunks*chunkSize) fail("lseek");
    memset(chunk,1,chunkSize);
    count=write(fd,chunk,chunkSize);
    if(count!=chunkSize) fail("write");
    pos=lseek(fd,0,SEEK_END);
    if(pos!=(nChunks+1)*chunkSize) fail("lseek modified size");
    close(fd);
    //Open for reading, seek past the end, check file size again, check content
    fd=open(file,O_RDONLY);
    if(fd<0) fail("open");
    pos=lseek(fd,2*nChunks*chunkSize,SEEK_SET);
    if(pos!=2*nChunks*chunkSize) fail("lseek");
    pos=lseek(fd,0,SEEK_END);
    if(pos!=(nChunks+1)*chunkSize) fail("lseek modified size");
    pos=lseek(fd,0,SEEK_SET);
    if(pos!=0) fail("lseek");
    for(int i=0;i<nChunks;i++)
    {
        memset(chunk,2,chunkSize);
        count=read(fd,chunk,chunkSize);
        if(count!=chunkSize) fail("read");
        for(int j=0;j<chunkSize;j++) if(chunk[j]!=0) fail("gap not zeroed");
    }
    memset(chunk,2,chunkSize);
    count=read(fd,chunk,chunkSize);
    if(count!=chunkSize) fail("read");
    for(int j=0;j<chunkSize;j++) if(chunk[j]!=1) fail("write/read fail");
    memset(chunk,2,chunkSize);
    count=read(fd,chunk,chunkSize);
    if(count!=0) fail("read past eof");
    for(int j=0;j<chunkSize;j++) if(chunk[j]!=2) fail("buffer altered");
    close(fd);
    //Open for read/write a non-empty file, mix and match of the above
    fd=open(file,O_RDWR);
    if(fd<0) fail("open");
    pos=lseek(fd,2*nChunks*chunkSize,SEEK_SET);
    if(pos!=2*nChunks*chunkSize) fail("lseek");
    count=read(fd,chunk,chunkSize);
    if(count!=0) fail("read");
    pos=lseek(fd,0,SEEK_CUR);
    if(pos!=2*nChunks*chunkSize) fail("read modified pos");
    pos=lseek(fd,0,SEEK_END);
    if(pos!=(nChunks+1)*chunkSize) fail("lseek modified size");
    pos=lseek(fd,2*nChunks*chunkSize,SEEK_SET);
    if(pos!=2*nChunks*chunkSize) fail("lseek");
    memset(chunk,1,chunkSize);
    count=write(fd,chunk,chunkSize);
    if(count!=chunkSize) fail("write");
    pos=lseek(fd,0,SEEK_END);
    if(pos!=(2*nChunks+1)*chunkSize) fail("lseek modified size");
    pos=lseek(fd,0,SEEK_SET);
    if(pos!=0) fail("lseek");
    for(int i=0;i<nChunks;i++)
    {
        memset(chunk,2,chunkSize);
        count=read(fd,chunk,chunkSize);
        if(count!=chunkSize) fail("read");
        for(int j=0;j<chunkSize;j++) if(chunk[j]!=0) fail("gap not zeroed");
    }
    memset(chunk,2,chunkSize);
    count=read(fd,chunk,chunkSize);
    if(count!=chunkSize) fail("read");
    for(int i=0;i<nChunks-1;i++)
    {
        memset(chunk,2,chunkSize);
        count=read(fd,chunk,chunkSize);
        if(count!=chunkSize) fail("read");
        for(int j=0;j<chunkSize;j++) if(chunk[j]!=0) fail("gap not zeroed");
    }
    memset(chunk,2,chunkSize);
    count=read(fd,chunk,chunkSize);
    if(count!=chunkSize) fail("read");
    for(int j=0;j<chunkSize;j++) if(chunk[j]!=1) fail("write/read fail");
    memset(chunk,2,chunkSize);
    count=read(fd,chunk,chunkSize);
    if(count!=0) fail("read past eof");
    close(fd);
}

static void fs_test_5()
{
    test_name("Seek past the end");
    fs_t5_p1(1);  //Less than a sector
    fs_t5_p1(16); //Exactly a sector
    fs_t5_p1(60); //More than a sector
    pass();
}

//
// Filesystem test 6
//
/*
tests:
truncate
ftruncate
*/

void writeFile(FILE *f, unsigned int length)
{
    constexpr unsigned int chunksize=64;
    char chunk[chunksize];
    char c=0;
    while(length>0)
    {
        unsigned int size=min(length,chunksize);
        for(unsigned int i=0;i<size;i++) chunk[i]=c++;
        if(fwrite(chunk,1,size,f)!=size) fail("fwrite");
        length-=size;
    }
}

void writeFile(const char *name, unsigned int length)
{
    FILE *f=fopen(name,"wb");
    if(f==NULL)
    {
        string s=string("writeFile: ")+name;
        fail(s.c_str());
    }
    writeFile(f,length);
    fclose(f);
}

void checkFile(FILE *f, unsigned int length, unsigned int padlength)
{
    rewind(f);
    constexpr unsigned int chunksize=64;
    char chunk[chunksize];
    char c=0;
    while(length>0)
    {
        unsigned int size=min(length,chunksize);
        if(fread(chunk,1,size,f)!=size) fail("file too short");
        for(unsigned int i=0;i<size;i++) if(chunk[i]!=c++) fail("file content");
        length-=size;
    }
    while(padlength>0)
    {
        unsigned int size=min(padlength,chunksize);
        if(fread(chunk,1,size,f)!=size) fail("file too short");
        for(unsigned int i=0;i<size;i++) if(chunk[i]!=0) fail("file padding");
        padlength-=size;
    }
    if(fgetc(f)!=EOF) fail("file too long");
}

void checkFile(const char *name, unsigned int length, unsigned int padlength)
{
    FILE *f=fopen(name,"rb");
    if(f==NULL)
    {
        string s=string("checkFile: ")+name;
        fail(s.c_str());
    }
    checkFile(f,length,padlength);
    fclose(f);
}

void truncateTest(const char *name, unsigned int origsize,
                  unsigned int largesize, unsigned int smallsize)
{
    unlink(name);
    writeFile(name,origsize);
    //Invalid size
    int result=truncate(name,-1);
    if(result!=-1 || errno!=EINVAL) fail("truncate neg");
    checkFile(name,origsize,0);
    //Same size
    if(truncate(name,origsize)!=0) fail("truncate same");
    checkFile(name,origsize,0);
    //First enlarge, then shrink (opposite pattern in ftruncateTest)
    //Larger size
    if(truncate(name,largesize)!=0) fail("truncate large");
    checkFile(name,origsize,largesize-origsize);
    //Smaller size
    if(truncate(name,smallsize)!=0) fail("truncate large");
    checkFile(name,smallsize,0);
}

void ftruncateTest(const char *name, unsigned int origsize,
                  unsigned int largesize, unsigned int smallsize)
{
    unlink(name);
    FILE *f=fopen(name,"w+b");
    if(f==NULL)
    {
        string s=string("ftruncateTest: ")+name;
        fail(s.c_str());
    }
    writeFile(f,origsize);
    //Invalid size
    int result=ftruncate(fileno(f),-1);
    if(result!=-1 || errno!=EINVAL) fail("truncate neg");
    checkFile(f,origsize,0);
    //Same size
    if(ftruncate(fileno(f),origsize)!=0) fail("truncate same");
    checkFile(f,origsize,0);
    //First shrink, then enlarge (opposite pattern in truncateTest)
    //Smaller size
    if(ftruncate(fileno(f),smallsize)!=0) fail("truncate large");
    checkFile(f,smallsize,0);
    //Larger size
    if(ftruncate(fileno(f),largesize)!=0) fail("truncate large");
    checkFile(f,smallsize,largesize-smallsize);
    fclose(f);
}

static void fs_test_6()
{
    test_name("Truncate");
    truncateTest("/sd/trunctest1.txt",100,200,50);        //All within a sector
    truncateTest("/sd/trunctest2.txt",4096,8192,2048);    //Sector boundaries
    truncateTest("/sd/trunctest3.txt",40000,60000,30000); //Large & unaligned
    ftruncateTest("/sd/trunctest4.txt",100,200,50);        //All within a sector
    ftruncateTest("/sd/trunctest5.txt",4096,8192,2048);    //Sector boundaries
    ftruncateTest("/sd/trunctest6.txt",40000,60000,30000); //Large & unaligned
    int result=truncate("/sd/file_does_not_exist.txt",100);
    if(result!=-1 || errno!=ENOENT) fail("truncate missing file");
    FILE *f=fopen("/sd/trunctest1.txt","rb"); //Reuse previously made file
    if(f==NULL) fail("fopen");
    result=ftruncate(fileno(f),0);
    if(result!=-1 || (errno!=EINVAL && errno!=EBADF)) fail("ftruncate open for reading");
    checkFile(f,50,0);
    fclose(f);
    pass();
}

//
// Filesystem test 7
//
/*
tests:
Non-existent files in mountpointfs
*/

static void fs_test_7()
{
    test_name("mountpointfs");
    struct stat statbuf;
    int res=open("/this_file_should_not_exist",O_RDONLY);
    if(res!=-1) fail("could open file in root");
    res=stat("/this_file_should_not_exist",&statbuf);
    if(res!=-1) fail("could stat file in root");
    res=open("/non_existent_directory/non_existent_file",O_RDONLY);
    if(res!=-1) fail("could open file in subdirectory in root");
    res=stat("/non_existent_directory/non_existent_file",&statbuf);
    if(res!=-1) fail("could stat file in subdirectory in root");
    pass();
}

//
// Filesystem test 8
//
/*
tests:
chmod
chown
access
open
mkdir
rmdir
diropen
search access
*/


// #define WITH_POSIX_PERMISSIONS
// #error remove the define
#if defined(WITH_POSIX_PERMISSIONS) || defined(IN_PROCESS)

/**
 * \note note that these tests will fail in fat32 and exfat since they
 * do not support posix permissions
 */
static void checkDirPermissions(const std::string& d, uid_t expectedUid, gid_t expectedGid, bool expectToFail=false)
{
    const char testPermissionsDir[]="test-permissions";

    int result=mkdir((d+testPermissionsDir).c_str(), 0500);
    if(result==0 && expectToFail) fail("mkdir 0500"); // dr-x------
    if(result!=0 && !expectToFail) fail("mkdir 0500"); // dr-x------
    
    // checDirContent fails when returns false, so that's why it must be equal expectToFail
    if(checkDirContent(d,testPermissionsDir,0)==expectToFail) fail("mkdir 0500 2");
    struct stat dirstat;

    // tests from now on do not make any sense since we didn't manage to create the directory
    if (expectToFail) return;
 
    lstat((d+testPermissionsDir).c_str(), &dirstat);
    if (dirstat.st_uid!=expectedUid && !expectToFail) fail("mkdir uid");
    if (dirstat.st_gid!=expectedGid) fail("mkdir gid");
    if ((dirstat.st_mode & S_IFDIR)==0) fail("mkdir ifdir mode");
    if ((dirstat.st_mode & 0777)!=0500) fail("mkdir permission mode");

    if(chmod((d+testPermissionsDir).c_str(), 0400)!=0) fail("chmod 0400"); // we want only read access
    DIR *dir=opendir((d+testPermissionsDir).c_str());
    if (dir==NULL) 
    {
        if (errno==EACCES) fail("Failed to access dir");
        fail("chmod opendir read permissions");
    }
    closedir(dir);


    lstat((d+testPermissionsDir).c_str(), &dirstat);
    if (dirstat.st_uid!=expectedUid) fail("mkdir uid 2");
    if (dirstat.st_gid!=expectedGid) fail("mkdir gid 2");
    if ((dirstat.st_mode & S_IFDIR)==0) fail("mkdir ifdir mode");
    if ((dirstat.st_mode & 0777)!=0400) fail("mkdir permission mode");

    // this is done to allow for rmdir the directory at the end
    // otherwise if the process euid is not 0 and we change the owner of the
    // directory we will not be able to delete it and the test will fail
    if(chmod((d+testPermissionsDir).c_str(), 0777)!=0) fail("chmod 0777");

    if(chown((d+testPermissionsDir).c_str(), expectedUid+1, expectedGid+1)!=0) fail("chown dir 1000 1000");
    dir=opendir((d+testPermissionsDir).c_str());
    if (dir==NULL) fail("chown opendir read permissions");
    closedir(dir);
    lstat((d+testPermissionsDir).c_str(), &dirstat);
    if (dirstat.st_uid!=expectedUid+1) fail("chown dir uid");
    if (dirstat.st_gid!=expectedGid+1) fail("chown dir gid");
    if ((dirstat.st_mode & S_IFDIR)==0) fail("chown dir ifdir mode");
    if ((dirstat.st_mode & 0777)!=0777) fail("chown dir permission mode");

    // we should be able to remove the directory since this action
    // depends only on the parent directory permissions
    // but we are root so we can do anything we want
    if(rmdir((d+testPermissionsDir).c_str())) fail("rmdir test-permissions");    
}

/**
 * All this tests fail if the FS is not littlefs for now
 * The reason is that littlefs is the only one that supports 
 * permissions for files.
 */
static void checkFilePermissions(const std::string d, uid_t expectedUid, gid_t expectedGid)
{
    const char* filename="test-file.txt";
    const char* filename2="testfile2.txt";

    
    // if we fail here we don't care, we just want to be sure the file does not exist
    // if unlink fails because something wrong is happening fopen will fail too
    unlink((d+filename).c_str());

    FILE* fd=fopen((d+filename).c_str(), "w+"); // rw-rw-rw- by default
    if (fd==NULL) fail("perm fopen");
    constexpr const char* teststr="test";
    constexpr size_t testlen=sizeof teststr;
    if(fwrite(teststr, 1, testlen, fd)!=testlen) fail("perm fwrite");
    if (fclose(fd)!=0) fail("perm fclose");

    struct stat filstat;
    if(lstat((d+filename).c_str(), &filstat)!=0) fail("perm lstat");
    printf("inode=%llu dev=%d mode=%lo uid=%d gid=%d %s\n",filstat.st_ino,
            filstat.st_dev,filstat.st_mode,filstat.st_uid,filstat.st_gid,(d+filename).c_str());
    if(!(filstat.st_mode & S_IFREG)) fail("perm lstat regular");
    if((filstat.st_mode&0777) != 0666) fail("perm lstat fopen");
    if(filstat.st_uid!=expectedUid) fail("perm lstat uid");
    if(filstat.st_gid!=expectedGid) fail("perm lstat gid");
    
    if(chmod((d+filename).c_str(), 0444)!=0) fail("perm chmod-lstat");
    if(lstat((d+filename).c_str(), &filstat)!=0) fail("perm lstat 2");
    if((filstat.st_mode&0777) != 0444) fail("perm lstat fopen 2");

    if(chmod((d+filename).c_str(), 0666)!=0) fail("perm chmod-lstat 2");
    
    if(access((d+filename).c_str(), F_OK)!=0) fail("perm access 1");
    if(access((d+filename).c_str(), R_OK)!=0) fail("perm access 2");
    if(access((d+filename).c_str(), W_OK)!=0) fail("perm access 3");

    if(access((d+filename).c_str(), X_OK)!=-1)
    {
        if (errno!=EACCES) fail("perm access 4");
    } 

    if(chmod((d+filename).c_str(), 0667)!=0)  fail("perm chmod");
    if(access((d+filename).c_str(), X_OK)!=0) fail("perm access 5");
    if(chmod((d+filename).c_str(), 0676)!=0)  fail("perm chmod 2");
    if(access((d+filename).c_str(), X_OK)!=0) fail("perm access 6");
    if(chmod((d+filename).c_str(), 0766)!=0)  fail("perm chmod 3");
    if(access((d+filename).c_str(), X_OK)!=0) fail("perm access 7");

    fd=fopen((d+filename).c_str(),"r");
    if(fd==NULL) fail("perm fopen 2");
    char buff[255];
    if (fread(buff, 1, testlen, fd)!=testlen) fail("perm fread");
    buff[testlen]=0;
    if(strcmp(teststr,buff)!=0) fail("perm fread 2");
    if (fclose(fd)!=0) fail("perm fclose 2");

    if(chmod((d+filename).c_str(), 0000)!=0)  fail("perm chmod 4");
    // we should be able to unlink the file even if we don't have permissions on it
    // since unlinking only refers to the parent directory and sticky bit is not
    // yet implemented
    if(unlink((d+filename).c_str())!=0) fail("perm cleanup 1");

    // root can always bypass directory permission checking which by now have already
    // been checked for root, many times
    if(expectedUid==0) return;

    // test directory read, search and edit (edit needs both search and write)
    const char* testDir = "test-dir/";
    
    if(mkdir((d+testDir).c_str(), 0700)!=0) //;fail("perm dir mkdir 0700");
    {
        if(errno==EACCES) fail("perm dir mkdir 0700 eacces");
        // recovery
        if (errno==EEXIST)
        {
            if(lstat((d+testDir).c_str(), &filstat)!=0) fail("Could not stat dir");
            printf("inode=%llu dev=%d mode=%lo uid=%d gid=%d %s\n",filstat.st_ino,
            filstat.st_dev,filstat.st_mode,filstat.st_uid,filstat.st_gid,(d+testDir).c_str());
    
        }
        chmod((d+testDir).c_str(), 0777);

        if(access((d+testDir+filename).c_str(), F_OK)==0)
        {
            iprintf("dir not empty, removing file 1\n");
            unlink((d+testDir+filename).c_str());
        }
        if(access((d+testDir+filename2).c_str(), F_OK)==0)
        {
            iprintf("dir not empty, removing file 2\n");
            unlink((d+testDir+filename2).c_str());
        }
        rmdir((d+testDir).c_str());
        if(mkdir((d+testDir).c_str(), 0700)!=0) fail("perm dir mkdir 0700");    
    }
    if(access((d+testDir).c_str(), F_OK)!=0) fail("perm dir access F_OK");
    
    fd=fopen((d+testDir+filename).c_str(),"w+");
    if (fd==NULL) fail("perm dir fopen");
    if(fwrite(teststr, 1, testlen, fd)!=testlen) fail("perm dir fwrite");
    if (fclose(fd)!=0) fail("perm dir fclose");

    if(lstat((d+testDir+filename).c_str(), &filstat)!=0) fail("perm dir lstat");
    printf("inode=%llu dev=%d mode=%lo uid=%d gid=%d %s\n",filstat.st_ino,
            filstat.st_dev,filstat.st_mode,filstat.st_uid,filstat.st_gid,(d+testDir+filename).c_str());
    if(!(filstat.st_mode & S_IFREG)) fail("perm  dir lstat regular");
    if((filstat.st_mode&0777) != 0666) fail("perm dir lstat fopen");
    if(filstat.st_uid!=expectedUid) fail("perm dir lstat uid");
    if(filstat.st_gid!=expectedGid) fail("perm dir lstat gid");

    if(chmod((d+testDir).c_str(), 0400)!=0) fail("perm dir chmod 2");
    if(access((d+testDir).c_str(), R_OK)!=0) fail("perm dir access R_OK");
    if(access((d+testDir).c_str(), W_OK)!=-1) fail("perm dir access W_OK");
    if(errno!=EACCES) fail("perm dir access W_OK 2");
    if(access((d+testDir).c_str(), X_OK)!=-1) fail("perm dir access X_OK");
    if(errno!=EACCES) fail("perm dir access X_OK 2");
    
    // we should able to read the directory contents and check that the file exists
    // but we should not be able to stat the file
    if(!checkDirContent(d+testDir, filename, 0)) fail("perm dir read access");
    if(lstat((d+testDir+filename).c_str(), &filstat)==0) fail("perm dir lstat 2");
    if(errno!=EACCES) fail("perm dir lstat 3");

    // now lets try search access but not read access
    if(chmod((d+testDir).c_str(), 0100)!=0) fail("perm dir chmod 2.1");
    if(checkDirContent(d+testDir, filename, 0)) fail("perm dir read access 2");
    if(errno!=EACCES) fail("perm dir read access 3");
    if(lstat((d+testDir+filename).c_str(), &filstat)!=0) fail("perm dir lstat 4");

    // now lets try editing the directory entries with rw- permissions
    if(chmod((d+testDir).c_str(), 0600)!=0) fail("perm dir chmod 3");
    int fdOpen=open((d+testDir+filename2).c_str(), O_WRONLY|O_CREAT, 0644);
    if(fdOpen>=0) fail("perm dir rw-");
    if(errno!=EACCES) fail("perm dir rw- 2");
    // here we want to test with filename since the previously open failed
    // we shouldnt be able to remove the filename since we need search access too
    if(unlink((d+testDir+filename).c_str())!=-1) fail("perm dir unlink rw-");

    // now r-x 
    unlink((d+testDir+filename2).c_str());
    if(chmod((d+testDir).c_str(), 0500)!=0) fail("perm dir chmod 3");
    fdOpen=open((d+testDir+filename2).c_str(), O_WRONLY|O_CREAT, 0644);
    if(fdOpen>=0) fail("perm dir r-x");
    if(errno!=EACCES) fail("perm dir r-x 2");
    if(access((d+testDir+filename2).c_str(), F_OK)==0) fail("perm dir r-x 3");
    // here we want to test with filename since the previously open failed
    // we shouldnt be able to remove the filename since we need write access
    if(unlink((d+testDir+filename).c_str())!=-1) fail("perm dir unlink r-x");

    // cleanup

    if(chmod((d+testDir).c_str(), 0700)!=0) fail("perm dir chmod 4");
    if(unlink((d+testDir+filename).c_str())!=0) fail("perm dir cleanup 1");
    // do unlink but ignore the result since this is useful only if the open test failed
    unlink((d+testDir+filename2).c_str());
    if(rmdir((d+testDir).c_str())!=0) fail("perm dir cleanup 2");
}

static void fs_test_8()
{
    test_name("permissions");
    checkDirPermissions("/", 0, 0);
    checkDirPermissions("/sd/", 0, 0);
    checkFilePermissions("/sd/", 0, 0);
    pass();

    #ifdef IN_PROCESS

    constexpr int PROCESS_NEW_UID=1000;
    constexpr int PROCESS_NEW_GID=1000;

    // we must create a space where the test can happen
    // otherwise evetything will fail
    mkdir("/sd/testproc", 0755);
    chown("/sd/testproc", PROCESS_NEW_UID, PROCESS_NEW_GID);
    if(access("/sd/testproc",F_OK)!=0) fail("perm setup test");
    chmod("/sd/testproc", 0755);

    setgid(PROCESS_NEW_GID);
    setuid(PROCESS_NEW_UID);
    if(access("/sd/testproc",R_OK|W_OK|X_OK)!=0) fail("perm setup test 2");
    
    // since mounpointfs has 555 access rights only the root user can effectively
    // create directories or files in the root directory
    // even if /sd is less strict we have 755 access that means still only the owner
    // can edit the directory entry. And the owner is root.
    // so all tests must be done in /sd/testproc that is owned by the process
    // we can do failure tests but that's all
    
    test_name("permissions drop directories / ");
    checkDirPermissions("/", PROCESS_NEW_UID, PROCESS_NEW_GID, true);
    pass();

    test_name("permissions drop directories /sd ");
    checkDirPermissions("/sd/", PROCESS_NEW_UID, PROCESS_NEW_GID, true);
    pass();

    test_name("permissions drop files /sd/testproc ");
    checkFilePermissions("/sd/testproc/", PROCESS_NEW_UID, PROCESS_NEW_GID);
    pass();
    #endif
}
#endif

//
// Pipe test
//
/*
tests:
dup
dup2
pipe
*/

static void sys_test_pipe_tryReadAndWrite(int readFd, int writeFd, char c)
{
    if(write(writeFd,&c,1)!=1) fail("write");
    char d;
    if(read(readFd,&d,1)!=1) fail("read");
    if(c!=d) fail("read output");
}

#ifndef IN_PROCESS
static void sys_test_pipe_largeWrite_thread(int wrFd, size_t wrSz, size_t rdSz)
{
    unsigned char *buf=new unsigned char[wrSz];
    if(buf==nullptr) fail("buffer allocation");
    for(size_t i=0; i<wrSz; i++) buf[i]=static_cast<unsigned char>(i&0xFF);
    for(size_t i=0; i<wrSz;)
    {
        ssize_t wr=write(wrFd,buf+i,wrSz-i);
        if(wr<=0 && rdSz>=wrSz) fail("write (large) unexpected eof");
        if(wr<=0) 
        {
            if(errno!=EPIPE) fail("EPIPE");
            break;
        }
        i+=wr;
    }
    delete[] buf;
    if(close(wrFd)!=0) fail("close (write end)");
}

static void sys_test_pipe_tryLargeReadAndWrite(size_t rdSz, size_t wrSz)
{
    int fds[2];
    if(pipe(fds)!=0) fail("pipe");

    std::thread t(sys_test_pipe_largeWrite_thread, fds[1], wrSz, rdSz);

    unsigned char *buf=new unsigned char[rdSz];
    if(buf==nullptr) fail("buffer allocation");
    size_t i;
    for(i=0; i<rdSz;)
    {
        ssize_t rd=read(fds[0],buf+i,rdSz-i);
        if(rd<0) fail("read (large)");
        if(rd==0 && wrSz>=rdSz) fail("read (large) unexpected eof");
        if(rd==0) break;
        i+=rd;
    }
    for(size_t j=0; j<i; j++)
    {
        unsigned char expected=static_cast<unsigned char>(j&0xFF);
        if(buf[j]!=expected) fail("read data not matching");
    }
    delete[] buf;

    if(close(fds[0])!=0) fail("close (read end)");
    t.join();
}
#endif

static void sys_test_pipe()
{
    test_name("pipes");
    int pipeFds[2];
    if(pipe(pipeFds)!=0) fail("pipe");

    sys_test_pipe_tryReadAndWrite(pipeFds[0], pipeFds[1], '1');

    int dupWriteFd=dup(pipeFds[1]);
    if(dupWriteFd<0) fail("dup (1)");
    sys_test_pipe_tryReadAndWrite(pipeFds[0], dupWriteFd, '2');

    int dupReadFd=dup(pipeFds[0]);
    if(dupReadFd<0) fail("dup (2)");
    sys_test_pipe_tryReadAndWrite(dupReadFd, dupWriteFd, '3');
    sys_test_pipe_tryReadAndWrite(dupReadFd, pipeFds[1], '4');

    // We need a dummy file descriptor. Open the root directory as it is
    // guaranteed to exist
    int dupReadFd2=open("/",O_SEARCH);
    if(dupReadFd2==-1) fail("open of /");
    if(dupReadFd2!=dup2(dupReadFd,dupReadFd2)) fail("dup2");
    sys_test_pipe_tryReadAndWrite(dupReadFd2, dupWriteFd, '5');
    sys_test_pipe_tryReadAndWrite(dupReadFd2, pipeFds[1], '6');
    if(close(dupReadFd2)!=0) fail("close (1)");

    if(close(pipeFds[0])!=0) fail("close (2)");
    sys_test_pipe_tryReadAndWrite(dupReadFd, dupWriteFd, '7');
    sys_test_pipe_tryReadAndWrite(dupReadFd, pipeFds[1], '8');

    if(close(pipeFds[1])!=0) fail("close (3)");
    sys_test_pipe_tryReadAndWrite(dupReadFd, dupWriteFd, '9');

    if(close(dupReadFd)!=0) fail("close (4)");
    if(close(dupWriteFd)!=0) fail("close (5)");

    #ifndef IN_PROCESS
    sys_test_pipe_tryLargeReadAndWrite(100, 512);
    sys_test_pipe_tryLargeReadAndWrite(512, 512);
    sys_test_pipe_tryLargeReadAndWrite(512, 100);
    #endif

    pass();
}

#endif //WITH_FILESYSTEM

//
// Time and sleep syscalls
//
/*
tests:
miosix::getTime
clock_gettime
times
gettimeofday
miosix::nanoSleepUntil
clock_nanosleep/nanosleep
usleep
*/

#ifdef IN_PROCESS
namespace miosix {
extern long long getTime();
extern void nanoSleepUntil(long long t);
}
#else
namespace miosix {
static inline void nanoSleepUntil(long long t)
{
    Thread::nanoSleepUntil(t);
}
}
#endif

void sys_test_time()
{
    long long t0,dt;
    int res;
    test_name("Time and sleep syscalls");

    //First, test lower level APIs
    t0 = miosix::getTime();
    miosix::nanoSleepUntil(t0+100000000);
    dt = miosix::getTime()-t0;
    //iprintf("miosix::getTime dt=%lld ns\n",dt);
    if (!(90000000<=dt&&dt<=110000000))
        fail("nanoSleepUntil and getTime do not agree");
    
    //clock_nanosleep
    //In Miosix this syscall never fails
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000;
    t0 = miosix::getTime();
    if(nanosleep(&ts,nullptr)!=0)
        fail("nanosleep");
    dt = miosix::getTime()-t0;
    //iprintf("miosix::getTime dt=%lld ns\n",dt);
    if (!(90000000<=dt&&dt<=110000000))
        fail("nanosleep and getTime do not agree");
    
    //usleep
    t0 = miosix::getTime();
    usleep(100000);
    dt = miosix::getTime()-t0;
    //iprintf("miosix::getTime dt=%lld ns\n",dt);
    if (!(90000000<=dt&&dt<=110000000))
        fail("usleep and getTime do not agree");

    //gettimeofday
    struct timeval tv0,tv1;
    res=gettimeofday(&tv0,nullptr);
    if(res) fail("gettimeofday");
    usleep(1100000);  //Slightly over 1 second to test rollover to tv_sec
    res=gettimeofday(&tv1,nullptr);
    if(res) fail("gettimeofday");
    t0=(long long)tv0.tv_sec*1000000LL+(long long)tv0.tv_usec;
    dt=((long long)tv1.tv_sec*1000000LL+(long long)tv1.tv_usec)-t0;
    //iprintf("gettimeofday dt=%lld us\n",dt);
    if (!(1000000<=dt&&dt<=1200000))
        fail("usleep and gettimeofday do not agree");

    //times
    struct tms tms0,tms1;
    clock_t clk0=times(&tms0);
    usleep(1000000);
    clock_t clk1=times(&tms1);
    if(clk0==static_cast<clock_t>(-1)||clk1==static_cast<clock_t>(-1))
        fail("times");
    if(tms0.tms_utime!=static_cast<clock_t>(-1) && tms0.tms_utime!=clk0)
        fail("times result does not agree with struct (1)");
    if(tms1.tms_utime!=static_cast<clock_t>(-1) && tms1.tms_utime!=clk1)
        fail("times result does not agree with struct (2)");
    dt=(unsigned long long)(tms1.tms_utime-tms0.tms_utime);
    //iprintf("times dt=%lld CLOCKS_PER_SEC\n",dt);
    if(!(CLOCKS_PER_SEC*9/10<=dt&&dt<=CLOCKS_PER_SEC*11/10))
        fail("usleep and times do not agree");
    
    //clock_gettime
    struct timespec ts0,ts1;
    res=clock_gettime(CLOCK_MONOTONIC,&ts0);
    if(res) fail("gettimeofday");
    usleep(1000000);
    res=clock_gettime(CLOCK_MONOTONIC,&ts1);
    if(res) fail("gettimeofday");
    t0=(long long)ts0.tv_sec*1000000000LL+(long long)ts0.tv_nsec;
    dt=((long long)ts1.tv_sec*1000000000LL+(long long)ts1.tv_nsec)-t0;
    //iprintf("clock_gettime dt=%lld ns\n",dt);
    if (!(900000000<=dt&&dt<=1100000000))
        fail("usleep and clock_gettime do not agree");

    pass();
}

//
// PID test
//
/*
tests:
getpid
getppid
*/

#ifdef IN_PROCESS
int sys_test_getpid_child(int argc, char *argv[])
{
    printf("%d %d\n",getppid(),getpid());
    fflush(stdout);
    return 0;
}
#endif

void sys_test_getpid()
{
    test_name("getpid/getppid");
    pid_t myPid=getpid();
    #ifndef IN_PROCESS
    if(myPid!=0) fail("getpid");
    #endif
    if(getppid()!=0) fail("getppid (kernel parent)");
    #ifdef WITH_PROCESSES
    const char *args[]={"/bin/test_process","sys_test_getpid_child",nullptr};
    int readFd;
    pid_t childPid=spawnWithPipe(args,readFd);
    FILE *fp=fdopen(readFd,"r");
    if(!fp) {printf("errno=%d\n", errno); fail("fdopen");}
    int ppidFromChild,pidFromChild;
    int res=fscanf(fp,"%d%d\n",&ppidFromChild,&pidFromChild);
    if(res!=2) fail("fscanf from process");
    res=fclose(fp);
    if(res!=0) fail("fclose");
    int ec;
    pid_t childPid2=wait(&ec);
    if(childPid!=childPid2) fail("wrong pid from wait");
    if(!WIFEXITED(ec)) fail("process returned due to error");
    if(WEXITSTATUS(ec)!=0) fail("process return value not zero");
    if(ppidFromChild!=myPid) fail("wrong parent pid in child");
    if(pidFromChild!=childPid) fail("wrong child pid in parent");
    #endif
    pass();
}

//
// isatty test
//
/*
tests:
isatty
*/

static void sys_test_isatty()
{
    test_name("isatty");
    if(isatty(STDIN_FILENO)!=1) fail("default stdin not a tty");
    if(isatty(STDOUT_FILENO)!=1) fail("default stdout not a tty");
    if(isatty(STDERR_FILENO)!=1) fail("default stderr not a tty");
    if(isatty(0x7FFFFFFF)!=0) fail("isatty of non-existent fd (return value)");
    if(errno!=EBADF) fail("isatty of non-existent fd (errno)");
    #ifdef WITH_FILESYSTEM
    int fd=open("/",O_SEARCH); // the root directory is guaranteed to exist
    if(fd==-1) fail("open of /");
    if(isatty(fd)!=0) fail("isatty of a directory (return value)");
    if(errno!=ENOTTY) fail("isatty of a directory (errno)");
    int res=close(fd);
    if(res!=0) fail("close of /");
    #endif
    pass();
}

#ifdef WITH_PROCESSES

//
// Process spawning test
//
/*
tests:
posix_spawn
exit
wait
waitpid
execve
*/

static void sys_test_spawn()
{
    test_name("process spawning/exiting");
    const char *arg0[] = { "/non_existing_file", nullptr };
    const char *env[] = { nullptr };
    pid_t pid,pid2;
    int ec;
    ec=posix_spawn(&pid,arg0[0],NULL,NULL,(char* const*)arg0,(char* const*)env);
    if(ec!=EACCES && ec!=ENOENT) fail("posix_spawn of non-existing file (return value)");

    const char *arg1[] = { "/bin/test_process", "exit_123", nullptr };
    ec=posix_spawn(nullptr,arg1[0],NULL,NULL,(char* const*)arg1,(char* const*)env);
    if(ec!=0) fail("posix_spawn (1)");
    int pstat;
    pid=wait(&pstat);
    if(pid<=0) fail("wait");
    if(!WIFEXITED(pstat)) fail("WIFEXITED (1)");
    if(WIFSIGNALED(pstat)) fail("WIFSIGNALED (1)");
    if(WIFSTOPPED(pstat)) fail("WIFSTOPPED (1)");
    if(WEXITSTATUS(pstat) != 123) fail("WEXITSTATUS (1)");

    ec=posix_spawn(&pid,arg1[0],NULL,NULL,(char* const*)arg1,(char* const*)env);
    if(ec!=0) fail("posix_spawn (2)");
    const char *arg2[] = { "/bin/test_process", "sleep_and_exit_234", nullptr };
    ec=posix_spawn(&pid2,arg1[0],NULL,NULL,(char* const*)arg2,(char* const*)env);
    if(ec!=0) fail("posix_spawn (3)");
    // The second process we launched should still be in its sleep(1) here
    pid_t pid3=waitpid(pid2,&pstat,WNOHANG);
    if(pid3!=0) fail("waitpid WNOHANG");
    pid3=waitpid(pid,&pstat,0);
    if(pid3!=pid) fail("waitpid (1)");
    if(!WIFEXITED(pstat)) fail("WIFEXITED (2)");
    if(WIFSIGNALED(pstat)) fail("WIFSIGNALED (2)");
    if(WIFSTOPPED(pstat)) fail("WIFSTOPPED (2)");
    if(WEXITSTATUS(pstat) != 123) fail("WEXITSTATUS (2)");
    pid3=waitpid(pid2,&pstat,0);
    if(pid3!=pid2) fail("waitpid (2)");
    if(!WIFEXITED(pstat)) fail("WIFEXITED (3)");
    if(WIFSIGNALED(pstat)) fail("WIFSIGNALED (3)");
    if(WIFSTOPPED(pstat)) fail("WIFSTOPPED (3)");
    if(WEXITSTATUS(pstat) != 234) fail("WEXITSTATUS (3)");

    const char *arg3[] = { "/bin/test_process", "environ_check", nullptr };
    const char *env1[] = { "TEST=test", nullptr };
    ec=posix_spawn(&pid,arg3[0],NULL,NULL,(char* const*)arg3,(char* const*)env1);
    if(ec!=0) fail("posix_spawn (4)");
    wait(&pstat);
    if(!WIFEXITED(pstat)) fail("WIFEXITED (4)");
    if(WIFSIGNALED(pstat)) fail("WIFSIGNALED (4)");
    if(WIFSTOPPED(pstat)) fail("WIFSTOPPED (4)");
    if(WEXITSTATUS(pstat)!=0) fail("environ");

    const char *arg5[] = { "/bin/test_process", "execve", nullptr };
    ec=posix_spawn(nullptr,arg5[0],NULL,NULL,(char* const*)arg5,(char* const*)env);
    if(ec!=0) fail("posix_spawn (6)");
    wait(&pstat);
    if(!WIFEXITED(pstat)) fail("WIFEXITED (6)");
    if(WIFSIGNALED(pstat)) fail("WIFSIGNALED (6)");
    if(WIFSTOPPED(pstat)) fail("WIFSTOPPED (6)");
    if(WEXITSTATUS(pstat)!=66 && WEXITSTATUS(pstat)!=77) fail("execve");
    if(WEXITSTATUS(pstat)==66) fail("execve'd process argc/argv/environ");
    
    ec=wait(&pstat);
    if(ec!=-1) fail("wait with no children (return value)");
    if(errno!=ECHILD) fail("wait with no children (errno)");

    pass();
}

#ifdef IN_PROCESS

//
// Process global constructor/destructor
//

static void proc_test_global_ctor_dtor()
{
    test_name("Global constructors/destructors in processes");

    const char *args[]={"/bin/test_global_dtor_ctor",nullptr};
    int readFd;
    pid_t pid=spawnWithPipe(args,readFd);
    FILE *fp=fdopen(readFd,"r");
    if(!fp) {printf("errno=%d\n", errno); fail("fdopen");}

    char buffer[64];
    if(fiscanf(fp,"%s",buffer)!=1) fail("fiscanf (1)");
    if(strcmp(buffer,"ctor")!=0) fail("global constructor not called");
    if(fiscanf(fp,"%s",buffer)!=1) fail("fiscanf (2)");
    if(strcmp(buffer,"member_ok")!=0) fail("member variables not correctly initialized");
    if(fiscanf(fp,"%s",buffer)!=1) fail("global destructor not called");
    if(strcmp(buffer,"dtor")!=0) fail("global destructor not called");
    if(fiscanf(fp,"%s",buffer)==1)
    {
        if(strcmp(buffer,"dtor")==0) fail("global destructors called twice");
        fail("fiscanf (4)");
    }
    fclose(fp);

    int pstat;
    pid_t exitPid=waitpid(pid,&pstat,0);
    if(exitPid!=pid) fail("waitpid");
    if(!WIFEXITED(pstat)) fail("test process not exited correctly");

    pass();
}

#endif // IN_PROCESS

#endif // WITH_PROCESSES
