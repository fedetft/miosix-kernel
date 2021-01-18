/**
 * This program can be used to test the SD card backend speed, i.e the
 * low level sector read/write functions.
 * 
 * !!!WARNING!!! If you select to test the write speed this program will
 * write at random in the SD card, corrupting everything in it (i.e: all files
 * and even the filesystem itself). After testing write speed you WILL need to
 * FORMAT your SD card before you can use it again, losing everythin in it.
 * 
 * NOTE: this program assumes the SD is larger than 1GByte, and you have
 * 32KByte available in your microcontroller for the disk buffer.
 */

#include <cstdio>
#include <cstring>
#include <cassert>
#include <chrono>
#include <fcntl.h>
#include <miosix.h>

using namespace std;
using namespace std::chrono;
using namespace miosix;

static bool randomAccess; ///< Random or sequential access?
static bool writeAccess;  ///< Read or write access?

void testThread(void *)
{
    const int sizek=32;                ///< Block write size in KByte
    const int sizeb=sizek*1024;        ///< Block write size in Byte
    const int sizei=sizeb/sizeof(int); ///< Block write size in integers
    int *data=new int[sizei];
    memset(data,0xaa,sizeb);
    const int startAddr=10240; ///< Start address, skip first sectors
    const int endAddr=2000000; ///< End address ~1GB card
    int addr=startAddr;
    int fd=open("/dev/sda",O_RDWR,0);
    if(fd<0)
    {
        perror("open");
        return;
    }
    for(;;)
    {
        if(randomAccess)
            addr=512*((rand() % (endAddr-startAddr-sizeb/512))+startAddr);
        else {
            addr+=sizeb;
            if(addr>endAddr-sizeb) addr=startAddr;
        }
        lseek(fd,addr,SEEK_SET);
        auto t=system_clock::now();
        ledOn();
        if(writeAccess) if(write(fd,data,sizeb)!=sizeb) assert(false);
        else; else if(read(fd,data,sizeb)!=sizeb) assert(false);
        ledOff();
        duration<float> d=system_clock::now()-t;
        float time=d.count();
        float speed=sizek/time;
        printf("time:%0.3fs speed:%0.1fKB/s\n",time,speed);
        if(Thread::testTerminate()) break;
    }
    close(fd);
    delete[] data;
}

int main()
{
    puts("\n====================");
    puts("Warning, the write test will destroy the formatting of the SD.");
    puts("After running that test, format your SD card!");
    for(;;)
    {
        writeAccess=false;
        randomAccess=false;
        for(;;)
        {
            puts("Read or write access, or quit (r/w/q)?");
            char line[64];
            fgets(line,sizeof(line),stdin);
            if(line[0]=='q') goto quit;
            if(line[0]=='w') writeAccess=true;
            if(line[0]=='w' || line[0]=='r') break;
            puts("Error: insert 'r' or 'w' or 'q'");
        }
        for(;;)
        {
            puts("Random or sequential access (r/s)?");
            char line[64];
            fgets(line,sizeof(line),stdin);
            if(line[0]=='r') randomAccess=true;
            if(line[0]=='r' || line[0]=='s') break;
            puts("Error: insert 'r' or 's'");
        }
        Thread *t=Thread::create(testThread,4096,1,0,Thread::JOINABLE);
        printf("Type enter to stop\n");
        getchar();
        t->terminate();
        t->join();
    }
    quit:
    printf("Bye\n");
}
