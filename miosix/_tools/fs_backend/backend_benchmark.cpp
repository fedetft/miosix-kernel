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
#include <miosix.h>
#include <interfaces/disk.h>

using namespace std;
using namespace miosix;

bool randomAccess=false; ///< Random or sequential access?
bool writeAccess=false;  ///< Read or write access?

void testThread(void *)
{
    const int sizek=32;                ///< Block write size in KByte
    const int sizeb=sizek*1024;        ///< Block write size in Byte
    const int sizei=sizeb/sizeof(int); ///< Block write size in integers
    const int sizes=sizeb/512;         ///< Block write size in blocks
    int *data=new int[sizei];
    memset(data,0xaa,sizei);
    Timer timer;
    const int startAddr=10240; ///< Start address
    const int endAddr=2000000; ///< End address ~1GB card
    int addr=startAddr;
    for(;;)
    {
        if(randomAccess) addr=rand()%(endAddr-startAddr-sizeb)+startAddr;
        else {
            addr+=sizes;
            if(addr>endAddr-sizeb) addr=startAddr;
        }
        timer.start();
        ledOn();
        if(writeAccess) Disk::write(reinterpret_cast<unsigned char*>(data),addr,sizes);
        else Disk::read(reinterpret_cast<unsigned char*>(data),addr,sizes);
        ledOff();
        timer.stop();
        float time=timer.interval();
        time/=1000.0f;
        timer.clear();
        float speed=sizek/static_cast<float>(time);
        printf("time:%0.3fs speed:%0.1fKB/s\n",time,speed);
        if(Thread::testTerminate()) break;
    }
    delete[] data;
}

int main()
{
    for(;;)
    {
        puts("Read or write access (r/w)?");
        char line[64];
        fgets(line,sizeof(line),stdin);
        if(line[0]=='w') writeAccess=true;
        if(line[0]=='w' || line[0]=='r') break;
        puts("Error: insert 'r' or 'w'");
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
    printf("Bye\n");
}
