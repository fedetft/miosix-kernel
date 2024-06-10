/***************************************************************************
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

using namespace std;

extern char **environ;

// Processes implies filesystem & DevFs
#define WITH_FILESYSTEM
#define WITH_DEVFS
#define WITH_PROCESSES
// Define for tests to distinguish whether we are compiling in a process
#define IN_PROCESS

//Functions common to all tests
static void test_name(const char *name);
static void pass();
static void fail(const char *cause);

// Syscalls tests (shared with kercalls)
#include "../test_syscalls.h"

static int sys_test_getpid_child(int argc, char *argv[]);

int main(int argc, char *argv[], char *envp[])
{
    if(argc == 0) return 1;
    else if(argc == 1) test_syscalls();
    else if(argc == 2)
    {
        if(strcmp("sys_test_getpid_child", argv[1])==0)
            return sys_test_getpid_child(argc, argv);
        if(strcmp("exit_123", argv[1])==0)
            exit(123);
        if(strcmp("sleep_and_exit_234", argv[1])==0)
        {
            sleep(1);
            exit(234);
        }
        if(strcmp("environ_check", argv[1])==0)
        {
            if(environ!=envp) return 1;
            if(environ[0]==nullptr) return 1;
            if(environ[1]!=nullptr) return 1;
            if(strcmp(environ[0],"TEST=test")!=0) return 1;
            return 0;
        }
        if(strcmp("execve", argv[1])==0)
        {
            // For some weird reason, execve signature allows the pointers to
            // be constant but not the strings.
            char arg_path[]="/bin/test_execve";
            char env_entry[]="TEST_ENVP=test";
            char *arg[] = { arg_path, nullptr };
            char *env[] = { env_entry, nullptr };
            execve(arg[0],arg,env);
            return 99;
        }
        if(strcmp("sigsegv", argv[1])==0)
        {
            uint32_t *ptr=nullptr;
            // Fool the compiler in believing the ptr may not be null
            // If we don't do this, the compiler is allowed to consider this
            // code to always have undefined behavior and transform it to
            // something we don't want (clang is known to pull this kind of
            // tricks)
            asm volatile("":"+r"(ptr)::);
            *ptr=12345;
        }
    }
    else return 1;
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
