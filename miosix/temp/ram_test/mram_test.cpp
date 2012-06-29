
// To test the MRAM for hibernation

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "sha1.h"
#include <interfaces/suspend_support.h>

using namespace std;

bool shaCmp(unsigned int a[5], unsigned int b[5])
{
    if(memcmp(a,b,20)==0) return false;
    iprintf("Mismatch\n");
    for(int i=0;i<5;i++) iprintf("%04x",a[i]); iprintf("\n");
    for(int i=0;i<5;i++) iprintf("%04x",b[i]); iprintf("\n");
    return true;
}

bool mramTest(int writeSize, int readSize)
{
    char *writeBuffer=new char[writeSize];
    char *readBuffer=new char[readSize];
    Mram& mram=Mram::instance();
    mram.exitSleepMode();
    
    SHA1 sha;
    sha.Reset();
    //Just to shuffle initialization between tests
    srand(0x7ad3c099+0xad45fce0*writeSize+0x658da2fb*readSize);
    for(unsigned int i=0;i<mram.size();i+=writeSize)
    {
        for(int j=0;j<writeSize;j++) writeBuffer[j]=rand() & 0xff;
        mram.write(i,writeBuffer,writeSize);
        sha.Input(writeBuffer,writeSize);
    }
    unsigned int a[5];
    sha.Result(a);
    
    sha.Reset();
    for(unsigned int i=0;i<mram.size();i+=readSize)
    {
        mram.read(i,readBuffer,readSize);
        sha.Input(readBuffer,readSize);
    }
    unsigned int b[5];
    sha.Result(b);
    
    delete[] writeBuffer;
    delete[] readBuffer;
    
    return shaCmp(a,b);
}

int main()
{  
    getchar();
    puts("MRAM test");
    puts("Testing w=1024, r=1024");
    if(mramTest(1024,1024)) return 1;
    puts("Testing w=512, r=1024");
    if(mramTest(512,1024)) return 1;
    puts("Testing w=1024, r=512");
    if(mramTest(1024,512)) return 1;
    puts("Ok");
}
