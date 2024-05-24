
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

using namespace std;

// Processes implies filesystem & DevFs
#define WITH_FILESYSTEM
#define WITH_DEVFS
// Define for tests to distinguish whether we are compiling in a process
#define IN_PROCESS

//Functions common to all tests
static void test_name(const char *name);
static void pass();
static void fail(const char *cause);

// Syscalls tests (shared with kercalls)
#include "../test_syscalls.h"

int main()
{
    test_syscalls();
    return 0;
}

/**
 * Called @ the beginning of a test
 * \param name test name
 */
static void test_name(const char *name)
{
    iprintf("Testing %s...\n",name);
}

/**
 * Called @ the end of a successful test
 */
static void pass()
{
    iprintf("Ok.\n");
}

/**
 * Called if a test fails
 * \param cause cause of the failure
 */
static void fail(const char *cause)
{
    //Can't use iprintf here because fail() may be used in threads
    //with 256 bytes of stack, and iprintf may cause stack overflow
    write(STDOUT_FILENO,"Failed:\n",8);
    write(STDOUT_FILENO,cause,strlen(cause));
    write(STDOUT_FILENO,"\n",1);
    exit(1);
}

/**
 * 16 bit ccitt crc calculation
 * \param crc The first time the function is called, pass 0xffff, all the other
 * times, pass the previous value. The value returned after the last call is
 * the desired crc
 * \param data a byte of the message
 */
static inline void crc16Update(unsigned short& crc, unsigned char data)
{
    unsigned short x=((crc>>8) ^ data) & 0xff;
    x^=x>>4;
    crc=(crc<<8) ^ (x<<12) ^ (x<<5) ^ x;
}

unsigned short crc16(const void *message, unsigned int length)
{
    const unsigned char *m=reinterpret_cast<const unsigned char*>(message);
    unsigned short result=0xffff;
    for(unsigned int i=0;i<length;i++) crc16Update(result,m[i]);
    return result;
}

#include "../test_syscalls.cpp"
