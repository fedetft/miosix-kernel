/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013 by Terraneo Federico *
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

/************************************************************************
* Part of the Miosix Embedded OS. This is the testsuite used during Miosix
* development to check for regressions.
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <set>
#include <cassert>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <dirent.h>
#include <tr1/functional>
#include <ext/atomicity.h>

#include "miosix.h"
#include "config/miosix_settings.h"
#include "interfaces/atomic_ops.h"
#include "board_settings.h"
#include "interfaces/endianness.h"
#include "e20/e20.h"
#include "kernel/intrusive.h"
#include "util/crc16.h"

#ifdef WITH_PROCESSES
#include "kernel/elf_program.h"
#include "kernel/process.h"
#include "kernel/process_pool.h"
#include "kernel/SystemMap.h"

#include "testsuite/syscall_testsuite/includes.h"
#include "testsuite/elf_testsuite/includes.h"
#include "testsuite/mpu_testsuite/includes.h"
#endif //WITH_PROCESSES

using namespace std::tr1;
using namespace miosix;

// A reasonably small stack value for spawning threads during the test.
// Using this instead of STACK_MIN because STACK_MIN is too low for some tests
// and caused stack overflows when compiling with -O0
// Note: can be reduced down to STACK_MIN if only testing with -O2
const unsigned int STACK_SMALL=512;

//Functions common to all tests
static void test_name(const char *name);
static void pass();
static void fail(const char *cause);
//Kernel test functions
static void test_1();
static void test_2();
static void test_3();
static void test_4();
static void test_5();
static void test_6();
static void test_7();
static void test_8();
static void test_9();
#ifndef __NO_EXCEPTIONS
static void test_10();
#endif //__NO_EXCEPTIONS
static void test_11();
static void test_12();
static void test_13();
static void test_14();
static void test_15();
static void test_16();
static void test_17();
static void test_18();
static void test_19();
static void test_20();
static void test_21();
static void test_22();
static void test_23();
static void test_24();
//Filesystem test functions
#ifdef WITH_FILESYSTEM
static void fs_test_1();
static void fs_test_2();
static void fs_test_3();
static void fs_test_4();
#endif //WITH_FILESYSTEM
//Benchmark functions
static void benchmark_1();
static void benchmark_2();
static void benchmark_3();
static void benchmark_4();
//Exception thread safety test
#ifndef __NO_EXCEPTIONS
static void exception_test();
#endif //__NO_EXCEPTIONS

#ifdef WITH_PROCESSES
void syscall_test_sleep();
void process_test_process_ret();
void syscall_test_system();
#ifdef WITH_FILESYSTEM
void syscall_test_files();
void process_test_file_concurrency();
void syscall_test_mpu_open();
void syscall_test_mpu_read();
void syscall_test_mpu_write();
#endif //WITH_FILESYSTEM

unsigned int* memAllocation(unsigned int size);
bool memCheck(unsigned int *base, unsigned int size);
void runElfTest(const char *name, const unsigned char *filename, unsigned int file_length);

int runProgram(const unsigned char *filename, unsigned int file_length);
bool isSignaled(int exit_code);
void mpuTest1();
void mpuTest2();
void mpuTest3();
void mpuTest4();
void mpuTest5();
void mpuTest6();
void mpuTest7();
void mpuTest8();
void mpuTest9();
void mpuTest10();
#endif //WITH_PROCESSES


//main(), calls all tests
int main()
{
    srand(0);
    //The priority of the test thread must be 0
    Thread::setPriority(0);
    for(;;)
    {
        iprintf("Type:\n"
                " 't' for kernel test\n"
                " 'f' for filesystem test\n"
                " 'x' for exception test\n"
                " 'b' for benchmarks\n"
                " 'p' for process test\n"
                " 'y' for syscall test\n"
                " 'm' for elf and mpu test\n"
                " 's' for shutdown\n");
        char c;
        for(;;)
        {
            c=getchar();
            if(c!='\n') break;
        }
        //For testing mpu
        unsigned int *m;
        switch(c)
        {
            case 't':
                //for(;;){ //Testing
                ledOn();
                test_1();
                test_2();
                test_3();
                test_4();
                test_5();
                test_6();
                test_7();
                test_8();
                test_9();
                #ifndef __NO_EXCEPTIONS
                test_10();
                #endif //__NO_EXCEPTIONS
                test_11();
                test_12();
                test_13();
                test_14();
                test_15();
                test_16();
                test_17();
                test_18();
                test_19();
                test_20();
                test_21();
                test_22();
                test_23();
                test_24();
                
                ledOff();
                Thread::sleep(500);//Ensure all threads are deleted.
                iprintf("\n*** All tests were successful\n\n");
                //} //Testing
                break;
            case 'f':
                ledOn();
                #ifdef WITH_FILESYSTEM
                fs_test_1();
                fs_test_2();
                fs_test_3();
                fs_test_4();
                #else //WITH_FILESYSTEM
                iprintf("Error, filesystem support is disabled\n");
                #endif //WITH_FILESYSTEM
                ledOff();
                Thread::sleep(500);//Ensure all threads are deleted.
                iprintf("\n*** All tests were successful\n\n");
                break;
            case 'x':
                #ifndef __NO_EXCEPTIONS
                exception_test();
                #else //__NO_EXCEPTIONS
                iprintf("Error, exception support is disabled\n");
                #endif //__NO_EXCEPTIONS
                break;
            case 'b':
                ledOn();
                benchmark_1();
                benchmark_2();
                benchmark_3();
                benchmark_4();

                ledOff();
                Thread::sleep(500);//Ensure all threads are deleted.
                break;
            case 'p':
                #ifdef WITH_PROCESSES
                ledOn();
                process_test_process_ret();
                process_test_file_concurrency();
                ledOff();
                #else //#ifdef WITH_PROCESSES
                iprintf("Error, process support is disabled\n");
                #endif //#ifdef WITH_PROCESSES
                break;
            case 'y':
                ledOn();
                #ifdef WITH_PROCESSES
                #ifdef WITH_FILESYSTEM
                syscall_test_files();
                syscall_test_mpu_open();
                syscall_test_mpu_read();
                syscall_test_mpu_write();
                #else //WITH_FILESYSTEM
                iprintf("Error, filesystem support is disabled\n");
                #endif //WITH_FILESYSTEM

                syscall_test_sleep();
                syscall_test_system();
                #else //WITH_PROCESSES
                iprintf("Error, process support is disabled\n");
                #endif //WITH_PROCESSES
                ledOff();
                break;
            case 'm':
                //The priority of the test thread must be 1
                Thread::setPriority(1);
                ledOn();
                #ifdef WITH_PROCESSES
                //Note by TFT: these addresses are only valid for the stm3220g-eval.
                //FIXME: look into it
                // ProcessPool allocates 4096 bytes starting from address 0x64100000
                // Range : 0x64100000 - 0x64101000

                // First process memory layout
                // Code region : 0x64101000 - 0x64101400
                // Data region : 0x64104000 - 0x64108000

                // Second process memory layout
                // Code region : 0x64101400 - 0x64101800
                // Data region : 0x64108000 - 0x6410c000

                // Third process memory layout
                // Code region : 0x64101800 - 0x64101c00
                // Data region : 0x6410c000 - 0x64110000

                //Altered elfs tests
                iprintf("\nExecuting ELF tests.\n");
                iprintf("--------------------\n");
                runElfTest("Elf Test1", aelf1, aelf1_len);
                runElfTest("Elf Test2", aelf2, aelf2_len);
                runElfTest("Elf Test3", aelf3, aelf3_len);
                runElfTest("Elf Test4", aelf4, aelf4_len);
                runElfTest("Elf Test5", aelf5, aelf5_len);
                runElfTest("Elf Test6", aelf6, aelf6_len);
                runElfTest("Elf Test7", aelf7, aelf7_len);

                //Mpu tests
                iprintf("\n\nExecuting MPU tests.\n");
                iprintf("---------------------\n");
                m = memAllocation(4096);
                mpuTest1();
                mpuTest2();
                mpuTest3();
                mpuTest4();
                mpuTest5();
                mpuTest6();
                mpuTest7();
                mpuTest8();
                mpuTest9();
                mpuTest10();
                ProcessPool::instance().deallocate(m);
                #else //#ifdef WITH_PROCESSES
                iprintf("Error, process support is disabled\n");
                #endif //#ifdef WITH_PROCESSES
                ledOff();
                Thread::setPriority(0);
            break;
            case 's':
                iprintf("Shutting down\n");
                shutdown();
            default:
                iprintf("Unrecognized option\n");
        }
    }
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
    write(STDOUT_FILENO,"Failed:\r\n",9);
    write(STDOUT_FILENO,cause,strlen(cause));
    write(STDOUT_FILENO,"\r\n",2);
    reboot();
}

#ifdef WITH_PROCESSES

void process_test_file_concurrency()
{
    test_name("Process file concurrency");

    remove("/file1.bin");
    remove("/file2.bin");

    ElfProgram prog1(reinterpret_cast<const unsigned int*>(testsuite_file1_elf), testsuite_file1_elf_len);
    ElfProgram prog2(reinterpret_cast<const unsigned int*>(testsuite_file2_elf), testsuite_file2_elf_len);

    pid_t p1 = Process::create(prog1);
    pid_t p2 = Process::create(prog2);

    int res1 = 0, res2 = 0;

    Process::waitpid(p1, &res1, 0);
    Process::waitpid(p2, &res2, 0);

    FILE *f1 = fopen("/file1.bin", "rb");
    FILE *f2 = fopen("/file2.bin", "rb");

    if(!f1) fail("Unable to open first file");
    if(!f2) fail("Unable to open second file");

    char buffer[10];

    for(int i = 0; i < 1000; i++)
    {
        fread(buffer, 1, 9, f1);
        if(strncmp(buffer, "file1.bin", 9) != 0) fail("Wrong data from file 1");
    }

    for(int i = 0; i < 1000; i++)
    {
        fread(buffer, 1, 9, f2);
        if(strncmp(buffer, "file2.bin", 9) != 0) fail("Wrong data from file 2");
    }

    fclose(f1);
    fclose(f2);

    pass();
}

void process_test_process_ret()
{
    test_name("Process return value");
    ElfProgram prog(reinterpret_cast<const unsigned int*>(testsuite_simple_elf),testsuite_simple_elf_len);
    int ret = 0;
    pid_t p = Process::create(prog);
    Process::waitpid(p, &ret, 0);
    //iprintf("Returned value is %d\n", WEXITSTATUS(ret));
    if(WEXITSTATUS(ret) != 42) fail("Wrong returned value");
    pass();
}

void syscall_test_mpu_open()
{
    test_name("open and MPU");
    ElfProgram prog(reinterpret_cast<const unsigned int*>(testsuite_syscall_mpu_open_elf), testsuite_syscall_mpu_open_elf_len);
    int ret = 0;
    pid_t p = Process::create(prog);
    Process::waitpid(p, &ret, 0);
    //iprintf("Returned value is %d\n", ret);
    if(WTERMSIG(ret) != SIGSYS) fail("0x00000000 is not a valid address!");
    pass();
}

void syscall_test_mpu_read()
{
    test_name("read calls and MPU");
    ElfProgram prog(reinterpret_cast<const unsigned int*>(testsuite_syscall_mpu_read_elf), testsuite_syscall_mpu_read_elf_len);
    int ret = 0;
    pid_t p = Process::create(prog);
    Process::waitpid(p, &ret, 0);
    //iprintf("Returned value is %d\n", ret);
    if(WTERMSIG(ret) != SIGSYS) fail("0x00000000 is not a valid address!");
    pass();
}

void syscall_test_mpu_write()
{
    test_name("write and MPU");
    ElfProgram prog(reinterpret_cast<const unsigned int*>(testsuite_syscall_mpu_write_elf), testsuite_syscall_mpu_write_elf_len);
    int ret = 0;
    pid_t p = Process::create(prog);
    Process::waitpid(p, &ret, 0);
    iprintf("Returned value is %d\n", ret);
    if(WTERMSIG(ret) != SIGSYS) fail("0x00000000 is not a valid address!");
    pass();
}

void syscall_test_system()
{
    test_name("system");
	if(SystemMap::instance().getElfCount() != 0)
		fail("The system lookup table should be empty");
	else
		iprintf("The system lookup table is empty. Correct.\n");
	
	SystemMap::instance().addElfProgram("test", reinterpret_cast<const unsigned int*>(testsuite_simple_elf), testsuite_simple_elf_len);
	
	if(SystemMap::instance().getElfCount() != 1)
		fail("Now the system lookup table should contain 1 program");
	else
		iprintf("The system lookup table contain one program. Correct.\n");
	
	std::pair<const unsigned int*, unsigned int> sysret = SystemMap::instance().getElfProgram("test");
	
	if(sysret.first == 0 || sysret.second == 0)
		fail("The system lookup table has returned an invalid process size or an invalid elf pointer for the process");
	
	ElfProgram prog(reinterpret_cast<const unsigned int*>(testsuite_system_elf), testsuite_system_elf_len);
	
	int ret = 0;
	pid_t p = Process::create(prog);
	Process::waitpid(p, &ret, 0);
	
	if(WEXITSTATUS(ret) != 42){
		iprintf("Host process returned: %d\n", WEXITSTATUS(ret));
		fail("The system inside a process has failed");
	}

	SystemMap::instance().removeElfProgram("test");
	
	if(SystemMap::instance().getElfCount() != 0)
		fail("The system lookup table now should be empty.\n");
	
	pass();
}

void syscall_test_sleep(){
	test_name("System Call: sleep");
	ElfProgram prog(reinterpret_cast<const unsigned int*>(testsuite_sleep_elf),testsuite_sleep_elf_len);
	
//	iprintf("diff was: %d\n", (unsigned int)getTick());
	
	int ret = 0;
	long long time = getTick();
	pid_t p = Process::create(prog);
	Process::waitpid(p, &ret, 0);
	
	long long diff = llabs((getTick() - time));
	
	if (llabs(diff - 5*TICK_FREQ) > static_cast<long long>(TICK_FREQ * 0.02))
		fail("The sleep should have only a little more than 5 seconds.");
	
	pass();
}

void syscall_test_files(){
	test_name("System Call: open, read, write, seek, close, system");
	
	ElfProgram prog(reinterpret_cast<const unsigned int*>(testsuite_syscall_elf),testsuite_syscall_elf_len);
	int ret = 0;
	
	remove("/foo.bin");
	
	pid_t child = Process::create(prog);
	Process::waitpid(child, &ret, 0);
	
	iprintf("Returned value %d\n", WEXITSTATUS(ret));
	switch(WEXITSTATUS(ret)){
		case 0:
			pass();
			break;
		case 1:
			fail("Open with O_RDWR should have failed, the file doesn't exist");
			break;
		case 2:
			fail("Cannot craete new file");
			break;
		case 3:
			fail("file descriptor not valid");
			break;
		case 4:
			fail("write has written the wrong amount of data");
			break;
		case 5:
			fail("read has read the wrong amount of data");
			break;
		case 6:
			fail("readed data doesn't match the written one");
			break;
		case 7:
			fail("close hasn't returned 0");
			break;
		case 8:
			fail("open with O_RDWR failed, but the file exists");
			break;
		case 9:
			fail("read has return the wrogn amount of data");
			break;
		case 10:
			fail("readed data doesn't match the written one");
			break;
		case 11:
			fail("close hasn't returned 0");
			break;
	}
}

#endif //WITH_PROCESSES

//
// Test 1
//
/*
tests:
Thread::create
Thread::yield
Thread::terminate
Thread::testTerminate
Thread::exists
Thread::getStackBottom
also test thread termination by exiting the entry point
and test argv pointer passing
*/

static volatile bool t1_v1;

static void t1_p1(void *argv)
{
    for(;;)
    {
        if(Thread::testTerminate()) break;
        t1_v1=true;
        #ifdef SCHED_TYPE_EDF
        Thread::sleep(5);
        #endif //SCHED_TYPE_EDF
    }
}

static void t1_p3(void *argv)
{
    if(reinterpret_cast<unsigned int>(argv)!=0xdeadbeef) fail("argv passing");
}

static void t1_f1(Thread *p)
{
    // This test is very strict and sometimes might fail. To see why consider
    // this: yield() moves control flow to the second thread, but a preemption
    // happens immediately before it has the chance to set t1_v1 to true.
    // In this case t1_v1 will remain false and the test will fail.
    // The Thread::sleep(5) is added to make this possibility as unlikely as
    //possible.

    //If the thread exists, it should modify t1_v1, and exist() must return true
    for(int i=0;i<10;i++) //testing 10 times
    {
        Thread::sleep(5);
        t1_v1=false;
        #ifndef SCHED_TYPE_EDF
        Thread::yield();//run t1_p1
        #else //SCHED_TYPE_EDF
        Thread::sleep(15);
        #endif //SCHED_TYPE_EDF
        if(t1_v1==false) fail("thread not created");
        if(Thread::exists(p)==false) fail("Thread::exists (1)");
    }
    p->terminate();
    #ifndef SCHED_TYPE_EDF
    Thread::yield();//Give time to the second thread to terminate
    #else //SCHED_TYPE_EDF
    Thread::sleep(15);
    #endif //SCHED_TYPE_EDF
    //If the thread doesn't exist, it can't modify t1_v1, and exist() must
    //return false
    for(int i=0;i<10;i++) //testing 10 times
    {
        t1_v1=false;
        #ifndef SCHED_TYPE_EDF
        Thread::yield();//run t1_p1
        Thread::yield();//Double yield to be extra sure.
        #else //SCHED_TYPE_EDF
        Thread::sleep(15);
        #endif //SCHED_TYPE_EDF
        if(t1_v1==true) fail("thread not deleted");
        if(Thread::exists(p)==true) fail("Thread::exists (2)");
    }
}

static void test_1()
{
    test_name("thread creation/deletion");
    //Testing getStackBottom()
    const unsigned int *y=Thread::getStackBottom();
    if(*y!=STACK_FILL) fail("getStackBottom() (1)");
    y--;//Now should point to last word of watermark
    if(*y!=WATERMARK_FILL) fail("getStackBottom() (2)");
    //Testing thread termination
    Thread *p=Thread::create(t1_p1,STACK_SMALL,0,NULL);
    t1_f1(p);
    //Testing argv passing
    p=Thread::create(t1_p3,STACK_SMALL,0,reinterpret_cast<void*>(0xdeadbeef));
    Thread::sleep(5);
    if(Thread::exists(p)) fail("thread not deleted (2)");
    pass();
}

//
// Test 2
//
/*
Tests:
pauseKernel
restartKernel
Thread::getCurrentThread
*/

static volatile bool t2_v1;
static Thread *t2_p_v1;

static void t2_p1(void *argv)
{
    //This is to fix a race condition: the immediately after the thread
    //creation a yield occurs, t2_p_v1 is not yet assigned so the check fails
    Thread::sleep(5);
    for(;;)
    {
        if(Thread::testTerminate()) break;
        if(Thread::getCurrentThread()!=t2_p_v1)
            fail("Thread::getCurrentThread()");
        t2_v1=true;
        #ifdef SCHED_TYPE_EDF
        Thread::sleep(5);
        #endif //SCHED_TYPE_EDF
    }
}

static void test_2()
{
    test_name("pause/restart kernel");
    t2_p_v1=Thread::create(t2_p1,STACK_SMALL,0,NULL);
    pauseKernel();//
    t2_v1=false;
    //If the kernel is stopped, t2_v1 must not be updated
    for(int i=0;i<10;i++)
    {
        delayMs(20);
        if(t2_v1==true) { restartKernel(); fail("pauseKernel"); }
    }
    restartKernel();//
    //If the kernel is started, t2_v1 must be updated
    for(int i=0;i<10;i++)
    {
        t2_v1=false;
        #ifndef SCHED_TYPE_EDF
        delayMs(20);
        #else //SCHED_TYPE_EDF
        Thread::sleep(15);
        #endif //SCHED_TYPE_EDF
        if(t2_v1==false) fail("restartKernel");
    }
    t2_p_v1->terminate();
    pass();
}

//
// Test 3
//
/*
tests:
Thread::sleep()
Thread::sleepUntil()
getTick()
also tests creation of multiple instances of the same thread
*/

static void t3_p1(void *argv)
{
    const int SLEEP_TIME=100;
    for(;;)
    {
        if(Thread::testTerminate()) break;
        //Test that Thread::sleep sleeps the desired number of ticks
        long long x=getTick();
        Thread::sleep(SLEEP_TIME);
        if(abs(((SLEEP_TIME*TICK_FREQ)/1000)-(getTick()-x))>5)
            fail("Thread::sleep() or getTick()");
    }
}

static volatile bool t3_v2;//Set to true by t3_p2
static volatile bool t3_deleted;//Set when an instance of t3_p2 is deleted

static void t3_p2(void *argv)
{
    const int SLEEP_TIME=15;
    for(;;)
    {
        t3_v2=true;
        Thread::sleep(SLEEP_TIME);
        if(Thread::testTerminate())
        {
            t3_deleted=true;
            break;
        }
    }
}

static void test_3()
{
    test_name("tick and sleep");
    Thread *p=Thread::create(t3_p1,STACK_SMALL,0,NULL);
    for(int i=0;i<4;i++)
    {
        //The other thread is running tick() test
        Thread::sleep((101*1000)/TICK_FREQ);
    }
    p->terminate();
    Thread::sleep(200); //make sure the other thread does terminate*/
    //Testing Thread::sleep() again
    t3_v2=false;
    p=Thread::create(t3_p2,STACK_SMALL,0,NULL);
    //t3_p2 sleeps for 15ms, then sets t3_v2. We sleep for 20ms so t3_v2 should
    //be true
    Thread::sleep(20);
    if(t3_v2==false) fail("Thread::sleep() (2)");
    t3_v2=false;
    //t3_p2 is sleeping for other 15ms and will set t3_v2 @ t=30ms
    //but we check variable @ t=25ms, so it should be false
    Thread::sleep(5);
    if(t3_v2==true) fail("Thread::sleep() (3)");
    //Giving time to t3_p2 to set t3_v2
    Thread::sleep(10);
    if(t3_v2==false) fail("Thread::sleep() (2)");
    t3_v2=false;
    //Creating another instance of t3_p2 to check what happens when 3 threads
    //are sleeping
    Thread *p2=Thread::create(t3_p2,STACK_SMALL,0,NULL);
    //p will wake @ t=45, main will wake @ t=47 and p2 @ t=50ms
    //this will test proper sorting of sleeping_list in the kernel
    Thread::sleep(12);
    if(t3_v2==false) fail("Thread::sleep() (2)");
    //Deleting the two instances of t3_p2
    t3_deleted=false;
    p->terminate();
    Thread::sleep(20);
    if(Thread::exists(p)) fail("multiple instances (1)");
    if(t3_deleted==false) fail("multiple instances (2)");
    t3_deleted=false;
    p2->terminate();
    Thread::sleep(20);
    if(Thread::exists(p)) fail("multiple instances (3)");
    if(t3_deleted==false) fail("multiple instances (4)");
    //Testing Thread::sleepUntil()
    long long tick;
    const int period=static_cast<int>(TICK_FREQ*0.01); //10ms
    {
        InterruptDisableLock lock; //Making these two operations atomic.
        tick=getTick();
        tick+=period;
    }
    for(int i=0;i<4;i++)
    {
        Thread::sleepUntil(tick);
        if(tick!=getTick()) fail("Thread::sleepUntil()");
        tick+=period;
    }
    pass();
}

//
// Test 4
//
/*
tests:
disableInterrupts
enableInterrupts
fastDisableInterrupts
fastEnableInterrupts
Thread::getPriority
Thread::setPriority
Thread::IRQgetCurrentThread
Thread::IRQgetPriority
*/

static volatile bool t4_v1;

static void t4_p1(void *argv)
{
    for(;;)
    {
        if(Thread::testTerminate()) break;
        t4_v1=true;
        #ifdef SCHED_TYPE_EDF
        Thread::sleep(5);
        #endif //SCHED_TYPE_EDF
    }
}

#ifdef SCHED_TYPE_EDF
//This takes .014/.03=47% of CPU time
static void t4_p2(void *argv)
{
    const int period=static_cast<int>(TICK_FREQ*0.03);
    long long tick=getTick();
    for(int i=0;i<10;i++)
    {
        long long prevTick=tick;
        tick+=period;
        Thread::setPriority(Priority(tick)); //Change deadline
        Thread::sleepUntil(prevTick); //Make sure the task is run periodically
        delayMs(14);
        if(getTick()>tick) fail("Deadline missed (A)\n");
    }
}
#endif //SCHED_TYPE_EDF

static void test_4()
{
    test_name("disableInterrupts and priority");
    Thread *p=Thread::create(t4_p1,STACK_SMALL,0,NULL);
    //Check getPriority()
    if(p->getPriority()!=0) fail("getPriority (0)");
    //Check that getCurrentThread() and IRQgetCurrentThread() return the
    //same value
    Thread *q=Thread::getCurrentThread();
    disableInterrupts();//
    if(q!=Thread::IRQgetCurrentThread()) fail("IRQgetCurrentThread");
    //Check IRQgetPriority
    if(p->IRQgetPriority()!=0) fail("IRQgetPriority");
    //Check that tick is not incremented and t4_v1 is not updated
    long long tick=getTick();
    t4_v1=false;
    for(int i=0;i<4;i++)
    {
        delayMs(100);
        if((t4_v1==true)||(tick!=getTick()))
        {
            enableInterrupts();//
            fail("disableInterrupts");
        }
    }
    enableInterrupts();//
    #ifndef SCHED_TYPE_EDF
    Thread::yield();
    #else //SCHED_TYPE_EDF
    Thread::sleep(15);
    #endif //SCHED_TYPE_EDF
    //Should not happen, since already tested
    if(t4_v1==false) fail("variable not updated");

    fastDisableInterrupts();//
    //Check that tick is not incremented and t4_v1 is not updated
    tick=getTick();
    t4_v1=false;
    for(int i=0;i<4;i++)
    {
        delayMs(100);
        if((t4_v1==true)||(tick!=getTick()))
        {
            fastEnableInterrupts();//
            fail("disableInterrupts");
        }
    }
    fastEnableInterrupts();//
    #ifndef SCHED_TYPE_EDF
    Thread::yield();
    #else //SCHED_TYPE_EDF
    Thread::sleep(15);
    #endif //SCHED_TYPE_EDF
    //Should not happen, since already tested
    if(t4_v1==false) fail("variable not updated");

    //Checking get_priority
    if(Thread::getCurrentThread()->getPriority()!=0)
        fail("getPriority (1)");
    //Checking setPriority
    Thread::setPriority(1);
    if(Thread::getCurrentThread()->getPriority()!=1)
        fail("setPriority (0)");
    #if !defined(SCHED_TYPE_CONTROL_BASED) && !defined(SCHED_TYPE_EDF)
    //Since priority is higher, the other thread must not run
    //Of course this is not true for the control based scheduler
    t4_v1=false;
    for(int i=0;i<4;i++)
    {
        Thread::yield();//must return immediately
        delayMs(100);
        if(t4_v1==true) fail("setPriority (1)");
    }
    #endif //SCHED_TYPE_CONTROL_BASED
    //Restoring original priority
    Thread::setPriority(0);
    if(Thread::getCurrentThread()->getPriority()!=0)
        fail("setPriority (2)");
    p->terminate();
    Thread::sleep(10);
    #ifdef SCHED_TYPE_EDF
    Thread::create(t4_p2,STACK_SMALL);
    const int period=static_cast<int>(TICK_FREQ*0.05);
    tick=getTick();
    //This takes .024/.05=48% of CPU time
    for(int i=0;i<10;i++)
    {
        long long prevTick=tick;
        tick+=period;
        Thread::setPriority(Priority(tick)); //Change deadline
        Thread::sleepUntil(prevTick); //Make sure the task is run periodically
        delayMs(24);
        if(getTick()>tick) fail("Deadline missed (B)\n");
    }
    #endif //SCHED_TYPE_EDF
    pass();
}

//
// Test 5
//
/*
tests:
Thread::wait
Thread::wakeup
Thread::IRQwait
Thread::IRQwakeup
*/

static volatile bool t5_v1;
static volatile bool t5_v2;//False=testing Thread::wait() else Thread::IRQwait()

static void t5_p1(void *argv)
{
    for(;;)
    {
        if(Thread::testTerminate()) break;
        t5_v1=true;
        if(t5_v2) Thread::wait();
        else {
            disableInterrupts();
            Thread::IRQwait();
            enableInterrupts();
            Thread::yield();
        }
    }
}

static void test_5()
{
    test_name("wait, wakeup, IRQwait, IRQwakeup");
    t5_v2=false;//Testing wait
    t5_v1=false;
    Thread *p=Thread::create(t5_p1,STACK_SMALL,0,NULL);
    //Should not happen, since already tested
    Thread::sleep(5);
    if(t5_v1==false) fail("variable not updated");
    int i;
    //Since the other thread is waiting, must not update t5_v1
    t5_v1=false;
    for(i=0;i<4;i++)
    {
        Thread::sleep(100);
        if(t5_v1==true) fail("Thread::wait");
    }
    t5_v1=false;
    t5_v2=true;//Test IRQwait()
    p->wakeup();
    //Now that is still running, must update the variable
    Thread::sleep(5);
    if(t5_v1==false) fail("Thread::wakeup");
    //Since the other thread is waiting, must not update t5_v1
    t5_v1=false;
    for(i=0;i<4;i++)
    {
        Thread::sleep(100);
        if(t5_v1==true) fail("Thread::IRQwait");
    }
    disableInterrupts();
    p->IRQwakeup();
    t5_v1=false;
    enableInterrupts();
    //Now that is still running, must update the variable
    Thread::sleep(5);
    if(t5_v1==false) fail("Thread::IRQwakeup");
    p->terminate();
    p->wakeup();//Wake thread so that it can terminate
    pass();
}

//
// Test 6
//
/*
tests:
Mutex::lock
Mutex::unlock
Mutex::tryLock
FastMutex::lock
FastMutex::unlock
FastMutex::tryLock
Lock
*/

Priority priorityAdapter(int x)
{
    #ifndef SCHED_TYPE_EDF
    return Priority(x);
    #else //SCHED_TYPE_EDF
    //The EDF scheduler uses deadlines as priorities, so the ordering is
    //reversed, smaller number = close deadline = higher priority
    //the value of 5 is arbitrary, it should just avoid going below zero
    return Priority(5-x);
    #endif //SCHED_TYPE_EDF
}

class Sequence
{
    public:
    Sequence()
    {
        s[0]='\0';
    }

    void add(const char c)
    {
        if(strlen(s)>=3) fail("error in Sequence class");
        char t[2];
        t[0]=c; t[1]='\0';
        strcat(s,t);
    }

    char *read()
    {
        return s;
    }

    void clear()
    {
        s[0]='\0';
    }

    private:
    char s[4];
};

static Sequence seq;
static Mutex t6_m1;
static FastMutex t6_m1a;

static void t6_p1(void *argv)
{
    t6_m1.lock();
    seq.add('1');
    Thread::sleep(100);
    //Testing priority inheritance. Priority is 2 because inherits priority
    //from t6_p3
    if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(2))
        fail("priority inheritance (1)");
    t6_m1.unlock();
}

static void t6_p2(void *argv)
{
    t6_m1.lock();
    seq.add('2');
    Thread::sleep(100);
    //Testing priority inheritance. Priority is 1 because enters after t6_p3
    if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(1))
        fail("priority inheritance (2)");
    t6_m1.unlock();
}

static void t6_p3(void *argv)
{
    t6_m1.lock();
    seq.add('3');
    Thread::sleep(100);
    //Testing priority inheritance. Original priority 2 must not change
    if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(2))
        fail("priority inheritance (3)");
    t6_m1.unlock();
}

static void t6_p4(void *argv)
{
    t6_m1.lock();
    for(;;)
    {
        if(Thread::testTerminate())
        {
            t6_m1.unlock();
            break;
        }
        #ifndef SCHED_TYPE_EDF
        Thread::yield();
        #else //SCHED_TYPE_EDF
        Thread::sleep(5);
        #endif //SCHED_TYPE_EDF
    }
}

static void t6_p1a(void *argv)
{
    t6_m1a.lock();
    seq.add('1');
    Thread::sleep(100);
    t6_m1a.unlock();
}

static void t6_p2a(void *argv)
{
    t6_m1a.lock();
    seq.add('2');
    Thread::sleep(100);
    t6_m1a.unlock();
}

static void t6_p3a(void *argv)
{
    t6_m1a.lock();
    seq.add('3');
    Thread::sleep(100);
    t6_m1a.unlock();
}

static void t6_p4a(void *argv)
{
    t6_m1a.lock();
    for(;;)
    {
        if(Thread::testTerminate())
        {
            t6_m1a.unlock();
            break;
        }
        #ifndef SCHED_TYPE_EDF
        Thread::yield();
        #else //SCHED_TYPE_EDF
        Thread::sleep(5);
        #endif //SCHED_TYPE_EDF
    }
}

static volatile bool t6_v1;
static Mutex t6_m2;
static FastMutex t6_m2a;

static void t6_p5(void *argv)
{
    for(;;)
    {
        if(Thread::testTerminate()) break;

        {
            Lock<Mutex> l(t6_m2);
            t6_v1=true;
        }

        Thread::sleep(10);
    }
}

static void t6_p5a(void *argv)
{
    for(;;)
    {
        if(Thread::testTerminate()) break;

        {
            Lock<FastMutex> l(t6_m2a);
            t6_v1=true;
        }

        Thread::sleep(10);
    }
}

static Mutex t6_m3;
static Mutex t6_m4;

static void t6_p6(void *argv)
{
    Mutex *m=reinterpret_cast<Mutex*>(argv);
    {
        Lock<Mutex> l(*m);
        Thread::sleep(1);
    }
}

static void t6_p6a(void *argv)
{
    FastMutex *m=reinterpret_cast<FastMutex*>(argv);
    {
        Lock<FastMutex> l(*m);
        Thread::sleep(1);
    }
}

static Mutex t6_m5(Mutex::RECURSIVE);
static FastMutex t6_m5a(FastMutex::RECURSIVE);

static void *t6_p7(void *argv)
{
    if(t6_m5.tryLock()==false) return reinterpret_cast<void*>(1); // 1 = locked
    t6_m5.unlock();
    return reinterpret_cast<void*>(0); // 0 = unlocked
}

bool checkIft6_m5IsLocked()
{
    Thread *t=Thread::create(t6_p7,STACK_SMALL,0,0,Thread::JOINABLE);
    void *result;
    t->join(&result);
    return reinterpret_cast<int>(result)==0 ? false : true;
}

static void *t6_p7a(void *argv)
{
    if(t6_m5a.tryLock()==false) return reinterpret_cast<void*>(1); // 1 = locked
    t6_m5a.unlock();
    return reinterpret_cast<void*>(0); // 0 = unlocked
}

bool checkIft6_m5aIsLocked()
{
    Thread *t=Thread::create(t6_p7a,STACK_SMALL,0,0,Thread::JOINABLE);
    void *result;
    t->join(&result);
    return reinterpret_cast<int>(result)==0 ? false : true;
}

static void test_6()
{
    test_name("Mutex class");
    #ifdef SCHED_TYPE_EDF
    Thread::setPriority(priorityAdapter(0));
    #endif //SCHED_TYPE_EDF
    seq.clear();
    //Create first thread
    Thread::create(t6_p1,STACK_SMALL,priorityAdapter(0),NULL);
    Thread::sleep(20);
    //Create second thread
    Thread::create(t6_p2,STACK_SMALL,priorityAdapter(1),NULL);
    Thread::sleep(20);
    //Create third thread
    Thread::create(t6_p3,STACK_SMALL,priorityAdapter(2),NULL);
    Thread::sleep(20);
    t6_m1.lock();
    /*
    Now there will be 4 threads on the Mutex, the first one, with priority 0,
    waiting 100ms the second with priority 1, the third with priority 2 and the
    fourth (this) with priority 0. The first will go first (because it entered
    first), and will inherit priority 1 when the second enters the waiting list,
    and priority 2 when the third enters the waiting list. When the first
    leaves, the third enters (list is ordered by priority), then the second and
    last the fourth.
    */
    //Testing priority inheritance. Original priority 0 must not change
    if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(0))
        fail("priority inheritance (4)");
    if(strcmp(seq.read(),"132")!=0)
    {
        //iprintf("%s\n",seq.read());
        fail("incorrect sequence");
    }
    t6_m1.unlock();

    Thread::sleep(350);//Ensure all threads are deleted
    //
    // Testing tryLock
    //

    //This thread will hold the lock until we terminate it
    Thread *t=Thread::create(t6_p4,STACK_SMALL,0);
    Thread::yield();
    if(t6_m1.tryLock()==true) fail("Mutex::tryLock() (1)");
    Thread::sleep(10);
    if(t6_m1.tryLock()==true) fail("Mutex::tryLock() (2)");
    t->terminate();
    #ifndef SCHED_TYPE_EDF
    Thread::yield();
    Thread::yield();//Ensuring the other thread is deleted
    #else //SCHED_TYPE_EDF
    Thread::sleep(15);
    #endif //SCHED_TYPE_EDF
    if(t6_m1.tryLock()==false) fail("Mutex::tryLock() (3)");
    t6_m1.unlock();
    t6_v1=false;
    Thread *t2=Thread::create(t6_p5,STACK_SMALL,1);
    Thread::sleep(30);
    if(t6_v1==false) fail("Thread not created");
    {
        Lock<Mutex> l(t6_m2);
        t6_v1=false;
        Thread::sleep(30);
        if(t6_v1==true) fail("Lock (1)");
    }
    Thread::sleep(30);
    if(t6_v1==false) fail("Lock (2)");
    t2->terminate();
    Thread::sleep(10);
    //
    // Testing full priority inheritance algorithm
    //
    Thread *t3;
    Thread *t4;
    {
        Lock<Mutex> l1(t6_m3);
        {
            Lock<Mutex> l2(t6_m4);
            //Check initial priority
            if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(0))
                fail("priority inheritance (5)");
            //This thread will increase priority to 1
            t=Thread::create(t6_p6,STACK_SMALL,priorityAdapter(1),
                    reinterpret_cast<void*>(&t6_m4),Thread::JOINABLE);
            Thread::sleep(10);
            if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(1))
                fail("priority inheritance (6)");
            //This thread will do nothing
            t2=Thread::create(t6_p6,STACK_SMALL,priorityAdapter(1),
                    reinterpret_cast<void*>(&t6_m3),Thread::JOINABLE);
            Thread::sleep(10);
            if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(1))
                fail("priority inheritance (7)");
            //This thread will increase priority to 2
            t3=Thread::create(t6_p6,STACK_SMALL,priorityAdapter(2),
                    reinterpret_cast<void*>(&t6_m4),Thread::JOINABLE);
            Thread::sleep(10);
            if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(2))
                fail("priority inheritance (8)");
            //This will do nothing
            Thread::setPriority(priorityAdapter(1));
            if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(2))
                fail("priority inheritance (9)");
            //This will increase priority
            Thread::setPriority(priorityAdapter(3));
            if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(3))
                fail("priority inheritance (10)");
            //This will reduce priority to 2, not 0
            Thread::setPriority(priorityAdapter(0));
            if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(2))
                fail("priority inheritance (11)");
        }
        //Unlocking t6_m4, only the thread waiting on t6_m3 will cause
        //priority inheritance, which is only t2 which has priority 1
        if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(1))
                fail("priority inheritance (12)");
        //This will make the priority of 1 hold even when unlocking the mutex
        Thread::setPriority(priorityAdapter(1));
        //This thread will increase priority to 2
        t4=Thread::create(t6_p6,STACK_SMALL,priorityAdapter(2),
                reinterpret_cast<void*>(&t6_m3),Thread::JOINABLE);
        Thread::sleep(10);
        if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(2))
            fail("priority inheritance (13)");

    }
    if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(1))
        fail("priority inheritance (14)");
    //Restore original priority
    Thread::setPriority(0);
    if(Thread::getCurrentThread()->getPriority()!=0)
        fail("priority inheritance (14)");
    t->join();
    t2->join();
    t3->join();
    t4->join();
    Thread::sleep(10);
    
    //
    // Testing recursive mutexes
    //
    {
        Lock<Mutex> l(t6_m5);
        {
            Lock<Mutex> l(t6_m5);
            {
                Lock<Mutex> l(t6_m5);
                t=Thread::create(t6_p6,STACK_SMALL,0,reinterpret_cast<void*>(
                    &t6_m5));
                Thread::sleep(10);
                //If thread does not exist it means it has locked the mutex
                //even if we locked it first
                if(Thread::exists(t)==false) fail("recursive mutex (1)");
            }
            Thread::sleep(10);
            //If thread does not exist the mutex was unlocked too early
            if(Thread::exists(t)==false) fail("recursive mutex (2)");
        }
        Thread::sleep(10);
        //If thread does not exist the mutex was unlocked too early
        if(Thread::exists(t)==false) fail("recursive mutex (3)");
    }
    Thread::sleep(10);
    //If thread exists the mutex was not unlocked
    if(Thread::exists(t)==true) fail("recursive mutex (4)");

    //Checking if tryLock on recursive mutex returns true when called by the
    //thread that already owns the lock
    {
        Lock<Mutex> l(t6_m5);
        {
            if(t6_m5.tryLock()==false) fail("Mutex::tryLock (4)");
            if(checkIft6_m5IsLocked()==false) fail("unexpected");
            t6_m5.unlock();
        }
        if(checkIft6_m5IsLocked()==false) fail("unexpected");
    }
    if(checkIft6_m5IsLocked()==true) fail("unexpected");

    if(t6_m5.tryLock()==false) fail("Mutex::tryLock (5)");
    if(t6_m5.tryLock()==false) fail("Mutex::tryLock (6)");
    if(checkIft6_m5IsLocked()==false) fail("unexpected");
    t6_m5.unlock();
    if(checkIft6_m5IsLocked()==false) fail("unexpected");
    t6_m5.unlock();
    if(checkIft6_m5IsLocked()==true) fail("unexpected");

    //
    // Testing FastMutex
    //
    
    seq.clear();
    //Create first thread
    Thread::create(t6_p1a,STACK_SMALL,priorityAdapter(0),NULL);
    Thread::sleep(20);
    //Create second thread
    Thread::create(t6_p2a,STACK_SMALL,priorityAdapter(1),NULL);
    Thread::sleep(20);
    //Create third thread
    Thread::create(t6_p3a,STACK_SMALL,priorityAdapter(2),NULL);
    Thread::sleep(20);
    t6_m1a.lock();
    /*
    Now there will be 4 threads on the Mutex, the first one, with priority 0,
    waiting 100ms the second with priority 1, the third with priority 2 and the
    fourth (this) with priority 0. Given that FastMutex does not implement
    priority inheritance, they will lock the mutex in fifo order.
    */
    if(strcmp(seq.read(),"123")!=0)
    {
        //iprintf("%s\n",seq.read());
        fail("incorrect sequence a");
    }
    t6_m1a.unlock();

    Thread::sleep(350);//Ensure all threads are deleted
    //
    // Testing tryLock
    //

    //This thread will hold the lock until we terminate it
    t=Thread::create(t6_p4a,STACK_SMALL,0);
    Thread::yield();
    if(t6_m1a.tryLock()==true) fail("Mutex::tryLock() (1a)");
    Thread::sleep(10);
    if(t6_m1a.tryLock()==true) fail("Mutex::tryLock() (2a)");
    t->terminate();
    #ifndef SCHED_TYPE_EDF
    Thread::yield();
    Thread::yield();//Ensuring the other thread is deleted
    #else //SCHED_TYPE_EDF
    Thread::sleep(15);
    #endif //SCHED_TYPE_EDF
    if(t6_m1a.tryLock()==false) fail("Mutex::tryLock() (3a)");
    t6_m1a.unlock();
    t6_v1=false;
    t2=Thread::create(t6_p5a,STACK_SMALL,1);
    Thread::sleep(30);
    if(t6_v1==false) fail("Thread not created");
    {
        Lock<FastMutex> l(t6_m2a);
        t6_v1=false;
        Thread::sleep(30);
        if(t6_v1==true) fail("Lock (1a)");
    }
    Thread::sleep(30);
    if(t6_v1==false) fail("Lock (2a)");
    t2->terminate();
    Thread::sleep(10);

    //
    // Testing recursive mutexes
    //
    {
        Lock<FastMutex> l(t6_m5a);
        {
            Lock<FastMutex> l(t6_m5a);
            {
                Lock<FastMutex> l(t6_m5a);
                t=Thread::create(t6_p6a,STACK_SMALL,0,reinterpret_cast<void*>(
                    &t6_m5a));
                Thread::sleep(10);
                //If thread does not exist it means it has locked the mutex
                //even if we locked it first
                if(Thread::exists(t)==false) fail("recursive mutex (1)");
            }
            Thread::sleep(10);
            //If thread does not exist the mutex was unlocked too early
            if(Thread::exists(t)==false) fail("recursive mutex (2)");
        }
        Thread::sleep(10);
        //If thread does not exist the mutex was unlocked too early
        if(Thread::exists(t)==false) fail("recursive mutex (3)");
    }
    Thread::sleep(10);
    //If thread exists the mutex was not unlocked
    if(Thread::exists(t)==true) fail("recursive mutex (4)");

    //Checking if tryLock on recursive mutex returns true when called by the
    //thread that already owns the lock
    {
        Lock<FastMutex> l(t6_m5a);
        {
            if(t6_m5a.tryLock()==false) fail("Mutex::tryLock (4)");
            if(checkIft6_m5aIsLocked()==false) fail("unexpected");
            t6_m5a.unlock();
        }
        if(checkIft6_m5aIsLocked()==false) fail("unexpected");
    }
    if(checkIft6_m5aIsLocked()==true) fail("unexpected");

    if(t6_m5a.tryLock()==false) fail("Mutex::tryLock (5)");
    if(t6_m5a.tryLock()==false) fail("Mutex::tryLock (6)");
    if(checkIft6_m5aIsLocked()==false) fail("unexpected");
    t6_m5a.unlock();
    if(checkIft6_m5aIsLocked()==false) fail("unexpected");
    t6_m5a.unlock();
    if(checkIft6_m5aIsLocked()==true) fail("unexpected");
    
    pass();
}

//
// Test 7
//
/*
tests:
Timer::start
Timer::stop
Timer::isRunning
Timer::interval
Timer::clear
*/

static void test_timer(Timer *t)
{
    //Testing interval behaviour when timer never started
    if(t->interval()!=-1) fail("interval (1)");
    //Testing isRunning
    if(t->isRunning()==true) fail("isRunning (1)");
    t->start();
    //Testing interval behaviour when timer not stopped
    if(t->interval()!=-1) fail("interval (2)");
    //Testing isRunning
    if(t->isRunning()==false) fail("isRunning (2)");
    Thread::sleep(100);
    t->stop();
    //Testing interval precision
    if(t->interval()==-1) fail("interval (3)");
    if(abs(t->interval()-((100*TICK_FREQ)/1000))>4) fail("not precise");
    //Testing isRunning
    if(t->isRunning()==true) fail("isRunning (1)");
}

static void test_7()
{
    test_name("Timer class");
    Timer t;
    //Testing a new timer
    test_timer(&t);
    t.clear();
    //Testing after clear
    test_timer(&t);
    //Testing copy constructor and operator = with a stopped timer
    t.clear();
    t.start();
    Thread::sleep(100);
    t.stop();
    Timer w(t);
    if(w.interval()==-1) fail("interval (copy 1)");
    if(abs(w.interval()-((100*TICK_FREQ)/1000))>4) fail("not precise (copy 1)");
    if(w.isRunning()==true) fail("isRunning (copy 1)");
    Timer q;
    q=t;
    if(q.interval()==-1) fail("interval (= 1)");
    if(abs(q.interval()-((100*TICK_FREQ)/1000))>4) fail("not precise (= 1)");
    if(q.isRunning()==true) fail("isRunning (= 1)");
    //Testing copy constructor and operator = with a running timer
    t.clear();
    t.start();
    Thread::sleep(100);
    Timer x(t);//copy constructor called when running
    x.stop();
    if(x.interval()==-1) fail("interval (copy 2)");
    if(abs(x.interval()-((100*TICK_FREQ)/1000))>4) fail("not precise (copy 2)");
    if(x.isRunning()==true) fail("isRunning (copy 2)");
    Timer y;
    y=t;//Operator = called when running
    y.stop();
    if(y.interval()==-1) fail("interval (= 2)");
    if(abs(y.interval()-((100*TICK_FREQ)/1000))>4) fail("not precise (= 2)");
    if(y.isRunning()==true) fail("isRunning (= 2)");
    //Testing concatenating time intervals
    t.clear();//Calling clear without calling stop. done on purpose
    t.start();
    Thread::sleep(100);
    t.stop();
    t.start();
    Thread::sleep(150);
    t.stop();
    if(t.interval()==-1) fail("interval (= 2)");
    if(abs(t.interval()-((250*TICK_FREQ)/1000))>4) fail("not precise (= 2)");
    pass();
}

//
// Test 8
//
/*
tests:
Queue::isFull()
Queue::isEmpty()
Queue::put()
Queue::get()
Queue::reset()
Queue::IRQput()
Queue::waitUntilNotFull()
Queue::IRQget()
Queue::waitUntilNotEmpty()
FIXME: The overloaded versions of IRQput and IRQget are not tested
*/

static Queue<char,4> t8_q1;
static Queue<char,4> t8_q2;

static void t8_p1(void *argv)
{
    for(;;)
    {
        if(Thread::testTerminate()) break;
        char c;
        t8_q1.get(c);
        t8_q2.put(c);
    }
}

static void test_8()
{
    test_name("Queue class");
    //A newly created queue must be empty
    if(t8_q1.isEmpty()==false) fail("isEmpty (1)");
    if(t8_q1.isFull()==true) fail("isFull (1)");
    //Adding elements
    t8_q1.put('0');
    if(t8_q1.isEmpty()==true) fail("isEmpty (2)");
    if(t8_q1.isFull()==true) fail("isFull (2)");
    t8_q1.put('1');
    if(t8_q1.isEmpty()==true) fail("isEmpty (2)");
    if(t8_q1.isFull()==true) fail("isFull (2)");
    t8_q1.put('2');
    if(t8_q1.isEmpty()==true) fail("isEmpty (3)");
    if(t8_q1.isFull()==true) fail("isFull (3)");
    t8_q1.put('3');
    if(t8_q1.isEmpty()==true) fail("isEmpty (4)");
    if(t8_q1.isFull()==false) fail("isFull (4)");
    //Removing elements
    char c;
    t8_q1.get(c);
    if(c!='0') fail("get (1)");
    if(t8_q1.isEmpty()==true) fail("isEmpty (5)");
    if(t8_q1.isFull()==true) fail("isFull (5)");
    t8_q1.get(c);
    if(c!='1') fail("get (2)");
    if(t8_q1.isEmpty()==true) fail("isEmpty (6)");
    if(t8_q1.isFull()==true) fail("isFull (6)");
    t8_q1.get(c);
    if(c!='2') fail("get (3)");
    if(t8_q1.isEmpty()==true) fail("isEmpty (7)");
    if(t8_q1.isFull()==true) fail("isFull (7)");
    t8_q1.get(c);
    if(c!='3') fail("get (4)");
    if(t8_q1.isEmpty()==false) fail("isEmpty (8)");
    if(t8_q1.isFull()==true) fail("isFull (8)");
    //Testing reset
    t8_q1.put('a');
    t8_q1.put('b');
    t8_q1.reset();
    if(t8_q1.isEmpty()==false) fail("reset (isEmpty)");
    if(t8_q1.isFull()==true) fail("reset (isFull)");
    //Test queue between threads
    t8_q1.reset();
    t8_q2.reset();
    Thread *p=Thread::create(t8_p1,STACK_SMALL,0,NULL);
    int i,j;
    char write='A', read='A';
    for(i=1;i<=8;i++)
    {
        for(j=0;j<i;j++)
        {
            t8_q1.put(write);
            write++;//Advance to next char, to check order
        }
        for(j=0;j<i;j++)
        {
            char c;
            t8_q2.get(c);
            if(c!=read) fail("put or get (1)");
            read++;
        }
    }
    //Test waitUntilNotFull and IRQput
    t8_q1.reset();
    t8_q2.reset();
    write='A';
    read='A';
    disableInterrupts();//
    for(j=0;j<8;j++)
    {
        if(t8_q1.isFull())
        {
            enableInterrupts();//
            t8_q1.waitUntilNotFull();
            disableInterrupts();//
        }
        if(t8_q1.isFull()) fail("waitUntilNotFull()");
        if(t8_q1.IRQput(write)==false) fail("IRQput (1)");
        write++;//Advance to next char, to check order
    }
    enableInterrupts();//
    for(j=0;j<8;j++)
    {
        char c;
        t8_q2.get(c);
        if(c!=read) fail("IRQput (2)");
        read++;
    }
    //Test waitUntilNotEmpty and IRQget
    t8_q1.reset();
    t8_q2.reset();
    write='A';
    read='A';
    for(j=0;j<8;j++)
    {
        disableInterrupts();//
        t8_q1.IRQput(write);
        write++;
        if(t8_q2.isEmpty()==false) fail("Unexpected");
        enableInterrupts();//
        t8_q2.waitUntilNotEmpty();
        disableInterrupts();//
        if(t8_q2.isEmpty()==true) fail("waitUntilNotEmpty()");
        char c='\0';
        if(t8_q2.IRQget(c)==false) fail("IRQget (1)");
        if(c!=read) fail("IRQget (2)");
        read++;
        enableInterrupts();//
    }
    p->terminate();
    t8_q1.put('0');
    //Make sure the queue is empty in case the testsuite is run again
    Thread::sleep(5);
    t8_q1.reset();
    t8_q2.reset();
    pass();
}

//
// Test 9
//
/*
tests:
isKernelRunning()
areInterruptsEnabled()
*/

static void test_9()
{
    test_name("isKernelRunning and save/restore interrupts");
    //Testing kernel_running with nested pause_kernel()
    if(isKernelRunning()==false) fail("isKernelRunning() (1)");
    pauseKernel();//1
    if(isKernelRunning()==true)
    {
        restartKernel();
        fail("isKernelRunning() (2)");
    }
    pauseKernel();//2
    if(isKernelRunning()==true)
    {
        restartKernel();
        restartKernel();
        fail("isKernelRunning() (3)");
    }
    restartKernel();//2
    if(isKernelRunning()==true)
    {
        restartKernel();
        fail("isKernelRunning() (4)");
    }
    restartKernel();//1
    if(isKernelRunning()==false) fail("isKernelRunning() (5)");
    //Testing nesting of disableInterrupts()
    long long i;
    if(areInterruptsEnabled()==false) fail("areInterruptsEnabled() (1)");
    disableInterrupts();//Now interrupts should be disabled
    i=getTick();
    delayMs(100);
    if(i!=getTick())
    {
        enableInterrupts();
        fail("disableInterrups() nesting (1)");
    }
    if(areInterruptsEnabled()==true)
    {
        enableInterrupts();
        fail("areInterruptsEnabled() (2)");
    }
    disableInterrupts();//Interrupts already disabled
    i=getTick();
    delayMs(100);
    if(i!=getTick())
    {
        enableInterrupts();
        fail("disableInterrups() nesting (2)");
    }
    if(areInterruptsEnabled()==true)
    {
        enableInterrupts();
        fail("areInterruptsEnabled() (3)");
    }
    enableInterrupts();//Now interrupts should remain disabled
    i=getTick();
    delayMs(100);
    if(i!=getTick())
    {
        enableInterrupts();
        fail("enableInterrupts() nesting (1)");
    }
    if(areInterruptsEnabled()==true)
    {
        enableInterrupts();
        fail("areInterruptsEnabled() (4)");
    }
    enableInterrupts();//Now interrupts should be enabled
    i=getTick();
    delayMs(100);
    if(i==getTick())
    {
        enableInterrupts();
        fail("enableInterrupts() nesting (2)");
    }
    if(areInterruptsEnabled()==false) fail("areInterruptsEnabled() (5)");
    pass();
}

//
// Test 10
//
/*
tests exception propagation throug entry point of a thread
*/

#ifndef __NO_EXCEPTIONS
void t10_f1()
{
    throw(std::logic_error("Ok, exception test worked"));
}

void t10_f2()
{
    try {
        t10_f1();
        fail("Exception not thrown (1)");
    } catch(std::logic_error& e)
    {
        throw;
    }
}

void t10_p1(void *argv)
{
    t10_f2();
    fail("Exception not thrown");
}

void test_10()
{
    test_name("Exception handling");
    Thread::sleep(10);
    Thread *t=Thread::create(t10_p1,1024+512,0,NULL);
    Thread::sleep(40);
    if(Thread::exists(t)) fail("Thread not deleted");
    pass();
}
#endif //__NO_EXCEPTIONS

//
// Test 11
//
/*
tests class MemoryProfiling
*/

static volatile unsigned int t11_v1;//Free heap after spawning thread

void t11_p1(void *argv)
{
    if(MemoryProfiling::getStackSize()!=STACK_SMALL) fail("getStackSize (2)");
    //Check that getCurrentFreeHeap returns the same value from different
    //threads if no heap allocation happened in between
    Thread::sleep(10);//Give time to other thread to set variable
    if(MemoryProfiling::getCurrentFreeHeap()!=t11_v1)
        fail("getCurrentFreeHeap (2)");
}

void test_11()
{
    test_name("MemoryProfiling class");
    MemoryProfiling::print();
    //Stack size
    unsigned int stackSize=MemoryProfiling::getStackSize();
    if(stackSize!=MAIN_STACK_SIZE) fail("getStackSize (1)");
    unsigned int curFreeStack=MemoryProfiling::getCurrentFreeStack();
    if(curFreeStack>stackSize) fail("getCurrentFreeStack (1)");
    if(MemoryProfiling::getAbsoluteFreeStack()>stackSize)
        fail("getAbsoluteFreeStack (1)");
    if(MemoryProfiling::getAbsoluteFreeStack()>curFreeStack-4)
        fail("getAbsoluteFreeStack (2)");
        unsigned int heapSize=MemoryProfiling::getHeapSize();
    if(MemoryProfiling::getCurrentFreeHeap()>heapSize)
        fail("getCurrentFreeHeap");
    if(MemoryProfiling::getAbsoluteFreeHeap()>heapSize)
        fail("getAbsoluteFreeHeap");
    //Multithread test
    Thread *t=Thread::create(t11_p1,STACK_SMALL,0,0,Thread::JOINABLE);
    t11_v1=MemoryProfiling::getCurrentFreeHeap();
    t->join(0);
    Thread::sleep(10); //Give time to the idle thread for deallocating resources
    pass();
}

//
// Test 12
//
/*
Additional test for priority inheritance
*/

Mutex t12_m1;
Mutex t12_m2;

void t12_p1(void *argv)
{
    Lock<Mutex> l1(t12_m1);
    Lock<Mutex> l2(t12_m2);
}

void t12_p2(void *argv)
{
    Lock<Mutex> l(t12_m1);
}

void test_12()
{
    test_name("Priority inheritance 2");
    Thread::setPriority(priorityAdapter(0)); //For EDF
    Thread *t1;
    Thread *t2;
    {
        //First, we lock the second Mutex
        Lock<Mutex> l(t12_m2);
        //Then we create the first thread that will lock successfully the first
        //mutex, but will block while locking the second
        t1=Thread::create(t12_p1,STACK_SMALL,priorityAdapter(0),0,
                Thread::JOINABLE);
        Thread::sleep(5);
        //Last, we create the third thread that will block at the first mutex,
        //causing the priority of the first thread to be increased.
        //But since the first thread is itself waiting, the priority increase
        //should transitively pass to the thread who locked the second mutex,
        //which is main.
        t2=Thread::create(t12_p2,STACK_SMALL,priorityAdapter(1),0,
                Thread::JOINABLE);
        Thread::sleep(5);
        if(Thread::getCurrentThread()->getPriority()!=priorityAdapter(1))
            fail("Priority inheritance not transferred transitively");
    }
    t1->join();
    t2->join();
    Thread::setPriority(0);
    pass();
}

//
// Test 13
//
/*
Tests:
printing through stderr
*/

void test_13()
{
    test_name("stderr");
    //Test stderr, as it used to fail
    int res=fiprintf(stderr,"This string is printed through stderr. %d\n",0);
    if(res<=0) fail("stderr");
    pass();
}

//
// Test 14
//
/*
Tests:
- Thread::create with void * entry point
- Thread::create with joinable threads
- Thread::isDetached
- Thread::join
- Thread::detach
*/

void *t14_p1(void *argv)
{
    for(;;)
    {
        if(Thread::testTerminate()) break;
        #ifndef SCHED_TYPE_EDF
        Thread::yield();
        #else //SCHED_TYPE_EDF
        Thread::sleep(5);
        #endif //SCHED_TYPE_EDF
    }
    return argv;
}

void *t14_p2(void *argv)
{
    Thread::sleep(50);
    return argv;
}

void t14_p3(void *argv)
{
    Thread::sleep(20);
    Thread *t=reinterpret_cast<Thread *>(argv);
    t->detach();
}

void test_14()
{
    test_name("joinable threads");
    //
    // Memory leak tests. These tests make sure that memory of joinable threads
    // is always deallocated. Since during tests MemoryStatistics.print() is
    // called, it is possible to run the test repeatedly and check for memory
    // leaks.
    //

    //Test 1: detached thread that returns void*
    Thread *t=Thread::create(t14_p1,STACK_SMALL,0,0);
    t->terminate();
    Thread::sleep(10);
    if(Thread::exists(t)) fail("detached thread");
    //Test 2: joining a not deleted thread
    t=Thread::create(t14_p2,STACK_SMALL,0,0,Thread::JOINABLE);
    t->join();
    Thread::sleep(10);
    //Test 3: detaching a thread while not deleted
    t=Thread::create(t14_p2,STACK_SMALL,0,0,Thread::JOINABLE);
    Thread::yield();
    t->detach();
    Thread::sleep(10);
    //Test 4: detaching a deleted thread
    t=Thread::create(t14_p1,STACK_SMALL,0,0,Thread::JOINABLE);
    t->terminate();
    Thread::sleep(10);
    t->detach();
    Thread::sleep(10);
    //Test 5: detaching a thread on which some other thread called join
    t=Thread::create(t14_p2,STACK_SMALL,0,0,Thread::JOINABLE);
    Thread::create(t14_p3,STACK_SMALL,0,reinterpret_cast<void*>(t));
    if(t->join()==true) fail("thread not detached (1)");

    //
    // Consistency tests, these make sure that joinable threads are correctly
    // implemented.
    //

    //Test 1: join on joinable, not already deleted
    void *result=0;
    t=Thread::create(t14_p2,STACK_SMALL,0,(void*)0xdeadbeef,Thread::JOINABLE);
    Thread::yield();
    if(t->join(&result)==false) fail("Thread::join (1)");
    if(Thread::exists(t)) fail("Therad::exists (1)");
    if(reinterpret_cast<unsigned>(result)!=0xdeadbeef) fail("join result (1)");
    Thread::sleep(10);

    //Test 2: join on joinable, but detach called before
    t=Thread::create(t14_p2,STACK_SMALL,0,0,Thread::JOINABLE);
    Thread::yield();
    t->detach();
    if(t->join()==true) fail("Thread::join (2)");
    Thread::sleep(60);

    //Test 3: join on joinable, but detach called after
    t=Thread::create(t14_p1,STACK_SMALL,0,0,Thread::JOINABLE);
    Thread::create(t14_p3,STACK_SMALL,0,reinterpret_cast<void*>(t));
    if(t->join()==true) fail("Thread::join (3)");
    t->terminate();
    Thread::sleep(10);

    //Test 4: join on detached, not already deleted
    t=Thread::create(t14_p1,STACK_SMALL,0,0);
    if(t->join()==true) fail("Thread::join (4)");
    t->terminate();
    Thread::sleep(10);

    //Test 5: join on self
    if(Thread::getCurrentThread()->join()==true)
        fail("Thread:: join (5)");

    //Test 6: join on already deleted
    result=0;
    t=Thread::create(t14_p1,STACK_SMALL,0,(void*)0xdeadbeef,Thread::JOINABLE);
    t->terminate();
    Thread::sleep(10);
    if(Thread::exists(t)==false) fail("Therad::exists (2)");
    if(t->join(&result)==false) fail("Thread::join (6)");
    if(Thread::exists(t)) fail("Therad::exists (3)");
    if(reinterpret_cast<unsigned>(result)!=0xdeadbeef) fail("join result (2)");
    Thread::sleep(10);

    //Test 7: join on already detached and deleted
    t=Thread::create(t14_p1,STACK_SMALL,0,0,Thread::JOINABLE);
    t->detach();
    t->terminate();
    Thread::sleep(10);
    if(Thread::exists(t)) fail("Therad::exists (4)");
    if(t->join()==true) fail("Thread::join (7)");
    Thread::sleep(10);
    
    pass();
}

//
// Test 15
//
/*
Tests:
- Condition variables
*/

static volatile bool t15_v1;
static volatile bool t15_v2;
static int t15_v3;
static ConditionVariable t15_c1;
static Mutex t15_m1;

void t15_p1(void *argv)
{
    for(int i=0;i<10;i++)
    {
        Lock<Mutex> l(t15_m1);
        t15_c1.wait(l);
        t15_v1=true;
        t15_v3++;
    }
}

void t15_p2(void *argv)
{
    for(int i=0;i<10;i++)
    {
        Lock<Mutex> l(t15_m1);
        t15_c1.wait(l);
        t15_v2=true;
        t15_v3++;
    }
}

static void test_15()
{
    test_name("Condition variables");
    //Test signal
    t15_v1=false;
    t15_v2=false;
    t15_v3=0;
    Thread *p1=Thread::create(t15_p1,STACK_SMALL,0);
    Thread *p2=Thread::create(t15_p2,STACK_SMALL,0);
    for(int i=0;i<20;i++)
    {
        Thread::sleep(10);
        if(t15_v1 || t15_v2) fail("Spurious wakeup (1)");
        t15_c1.signal();
        Thread::sleep(10);
        {
            Lock<Mutex> l(t15_m1);
            if(t15_v1 && t15_v2) fail("Both threads wake up");
            if(t15_v3!=i+1) fail("Mutple wakeup (1)");
        }
        t15_v1=false;
        t15_v2=false;
    }
    Thread::sleep(10);
    if(Thread::exists(p1) || Thread::exists(p2))
        fail("Threads not deleted (1)");
    //Test broadcast
    t15_v1=false;
    t15_v2=false;
    t15_v3=0;
    p1=Thread::create(t15_p1,STACK_SMALL,0);
    p2=Thread::create(t15_p2,STACK_SMALL,0);
    for(int i=0;i<10;i++)
    {
        Thread::sleep(10);
        if(t15_v1 || t15_v2) fail("Spurious wakeup (2)");
        t15_c1.broadcast();
        Thread::sleep(10);
        {
            Lock<Mutex> l(t15_m1);
            if((t15_v1 && t15_v2)==false) fail("Not all thread woken up");
            if(t15_v3!=2*(i+1)) fail("Mutple wakeup (2)");
        }
        t15_v1=false;
        t15_v2=false;
    }
    Thread::sleep(10);
    if(Thread::exists(p1) || Thread::exists(p2))
        fail("Threads not deleted (2)");
    pass();
}

//
// Test 16
//
/*
tests:
posix threads API
 pthread_attr_init
 pthread_attr_destroy
 pthread_attr_getstacksize
 pthread_attr_setstacksize
 pthread_attr_getdetachstate
 pthread_atthr_setdetachstate
 pthread_detach
 pthread_create
 pthread_join
 pthread_self
 pthread_equal
 pthread_mutex_init
 pthread_mutex_destroy
 pthread_mutex_lock
 pthread_mutex_unlock
 pthread_mutex_trylock
 pthread_cond_init
 pthread_cond_destroy
 pthread_cond_wait
 pthread_cond_signal
 pthread_cond_broadcast
 pthread_once
*/

inline Thread *toThread(pthread_t t)
{
    return reinterpret_cast<Thread*>(t);
}

void *t16_p1(void *argv)
{
    toThread(pthread_self())->wait();
    return argv;
}

volatile bool t16_v1;
pthread_mutex_t t16_m1=PTHREAD_MUTEX_INITIALIZER;

void *t16_p2(void *argv)
{
    pthread_mutex_t *mutex=(pthread_mutex_t*)argv;
    if(pthread_mutex_lock(mutex)!=0) fail("mutex_lock (2)");
    t16_v1=true;
    Thread::sleep(50);
    if(pthread_mutex_unlock(mutex)!=0) fail("mutex unlock (2)");
    return NULL;
}

pthread_cond_t t16_c1=PTHREAD_COND_INITIALIZER;

void *t16_p3(void *argv)
{
    Thread::sleep(30);
    if(pthread_mutex_trylock(&t16_m1)!=0)
        fail("cond_wait did not release mutex"); //<---
    t16_v1=true;
    if(pthread_cond_signal(&t16_c1)!=0) fail("signal");
    if(pthread_mutex_unlock(&t16_m1)!=0) fail("cond mutex unlock"); //<---
    return NULL;
}

pthread_cond_t t16_c2;

void *t16_p4(void *argv)
{
    Thread::sleep(30);
    if(pthread_mutex_trylock(&t16_m1)!=0)
        fail("cond_wait did not release mutex (2)"); //<---
    t16_v1=true;
    //testing pthread_cond_destroy while a thread is waiting
    if(pthread_cond_destroy(&t16_c2)==0) fail("cond destroy while waiting");
    //testing broadcast
    if(pthread_cond_broadcast(&t16_c2)!=0) fail("broadcast");
    if(pthread_mutex_unlock(&t16_m1)!=0) fail("cond mutex unlock (2)"); //<---
    return NULL;
}

int a=0;

void t16_f1()
{
    a++;
}

pthread_once_t t16_o1=PTHREAD_ONCE_INIT;

static void test_16()
{
    test_name("posix threads");
    //
    // Test pthread_attr_*
    //
    pthread_attr_t attr;
    if(pthread_attr_init(&attr)!=0) fail("attr_init return value");
    int temp;
    size_t temp2;
    if(pthread_attr_getstacksize(&attr,&temp2)!=0)
        fail("attr_getstacksize return value (1)");
    if(temp2!=STACK_DEFAULT_FOR_PTHREAD) fail("attr_getstacksize (1)");
    if(pthread_attr_getdetachstate(&attr,&temp)!=0)
        fail("attr_getdetachstate (1)");
    if(temp!=PTHREAD_CREATE_JOINABLE) fail("attr_getdetahstate (1)");
    //setstacksize should fail for anything below STACK_SMALL
    if(pthread_attr_setstacksize(&attr,STACK_MIN-4)!=EINVAL)
        fail("stacks < STACK_MIN allowed");
    if(pthread_attr_getstacksize(&attr,&temp2)!=0)
        fail("attr_getstacksize return value (2)");
    if(temp2!=STACK_DEFAULT_FOR_PTHREAD) fail("attr_getstacksize (2)");
    //set a valid stack size
    if(pthread_attr_setstacksize(&attr,STACK_MIN)!=0)
        fail("attr_setstacksize return value");
    if(pthread_attr_getstacksize(&attr,&temp2)!=0)
        fail("attr_getstacksize return value (3)");
    if(temp2!=STACK_MIN) fail("attr_getstacksize (3)");
    //now try setting detach state
    if(pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED)!=0)
        fail("attr_stedetachstate return value (1)");
    if(pthread_attr_getdetachstate(&attr,&temp)!=0)
        fail("attr_getdetachstate (2)");
    if(temp!=PTHREAD_CREATE_DETACHED) fail("attr_getdetahstate (2)");
    //
    // Test pthread_create, pthread_self, pthread_equal, pthread_attr_destroy
    //
    pthread_t thread;
    if(pthread_create(&thread,&attr,t16_p1,NULL)!=0)
        fail("pthread_create (1)");
    Thread *t=toThread(thread);
    if(Thread::exists(t)==false) fail("thread not created");
    if(t->isDetached()==false) fail("attr_setdetachstate not considered");
    pthread_t self=pthread_self();
    if(Thread::getCurrentThread()!=toThread(self)) fail("pthread_self");
    if(pthread_equal(thread,self)) fail("pthread_equal (1)");
    if(!pthread_equal(thread,thread)) fail("pthread_equal (2)");
    if(pthread_attr_destroy(&attr)!=0) fail("attr_destroy return value");
    Thread::sleep(10); //Give the other thread time to call wait()
    t->wakeup();
    Thread::sleep(10);
    if(Thread::exists(t)) fail("unexpected");
    //
    // testing pthread_join and pthread_detach
    //
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr,STACK_SMALL);
    if(pthread_create(&thread,&attr,t16_p1,(void*)0xdeadbeef)!=0)
        fail("pthread_create (2)");
    t=toThread(thread);
    if(Thread::exists(t)==false) fail("thread not created (2)");
    if(t->isDetached()==true) fail("detached by mistake (1)");
    Thread::sleep(10); //Give the other thread time to call wait()
    t->wakeup();
    void *res;
    if(pthread_join(thread,&res)!=0) fail("join return value");
    if(((unsigned)res)!=0xdeadbeef) fail("entry point return value");
    if(Thread::exists(t)) fail("not joined");
    Thread::sleep(10);
    //Testing create with no pthread_attr_t
    if(pthread_create(&thread,NULL,t16_p1,NULL)!=0) fail("pthread_create (3)");
    t=toThread(thread);
    if(Thread::exists(t)==false) fail("thread not created (3)");
    if(t->isDetached()==true) fail("detached by mistake (2)");
    if(pthread_detach(thread)!=0) fail("detach return value");
    if(t->isDetached()==false) fail("detach");
    Thread::sleep(10); //Give the other thread time to call wait()
    t->wakeup();
    Thread::sleep(10);
    if(Thread::exists(t)) fail("unexpected (2)");
    //
    // testing pthread_mutex_*
    //
    if(pthread_mutex_lock(&t16_m1)!=0) fail("mutex lock"); //<---
    t16_v1=false;
    if(pthread_create(&thread,NULL,t16_p2,(void*)&t16_m1)!=0)
        fail("pthread_create (4)");
    t=toThread(thread);
    if(Thread::exists(t)==false) fail("thread not created (4)");
    if(t->isDetached()==true) fail("detached by mistake (3)");
    Thread::sleep(10);
    if(t16_v1==true) fail("mutex fail");
    if(pthread_mutex_unlock(&t16_m1)!=0) fail("mutex unlock"); //<---
    Thread::sleep(10);
    if(t16_v1==false) fail("mutex fail (2)");
    pthread_join(thread,NULL);
    Thread::sleep(10);
    //testing trylock, after the thread is created it will lock the mutex
    //for 50ms
    if(pthread_create(&thread,NULL,t16_p2,(void*)&t16_m1)!=0)
        fail("pthread_create (5)");
    Thread::sleep(10);
    if(pthread_mutex_trylock(&t16_m1)==0) fail("trylock");
    pthread_join(thread,NULL);
    if(pthread_mutex_trylock(&t16_m1)!=0) fail("trylock (2)"); //<---
    if(pthread_mutex_unlock(&t16_m1)!=0) fail("mutex unlock (3)"); //<---
    Thread::sleep(10);
    //now testing pthread_mutex_init
    pthread_mutex_t mutex;
    if(pthread_mutex_init(&mutex,NULL)!=0) fail("mutex init");
    if(pthread_mutex_lock(&mutex)!=0) fail("mutex lock (4)"); //<---
    t16_v1=false;
    if(pthread_create(&thread,NULL,t16_p2,(void*)&mutex)!=0)
        fail("pthread_create (6)");
    t=toThread(thread);
    if(Thread::exists(t)==false) fail("thread not created (5)");
    if(t->isDetached()==true) fail("detached by mistake (4)");
    Thread::sleep(10);
    if(t16_v1==true) fail("mutex fail (3)");
    if(pthread_mutex_unlock(&mutex)!=0) fail("mutex unlock (4)"); //<---
    Thread::sleep(10);
    if(t16_v1==false) fail("mutex fail (4)");
    pthread_join(thread,NULL);
    Thread::sleep(10);
    //testing pthread_mutex_destroy
    if(pthread_create(&thread,NULL,t16_p2,(void*)&mutex)!=0)
        fail("pthread_create (7)");
    t=toThread(thread);
    if(Thread::exists(t)==false) fail("thread not created (6)");
    if(t->isDetached()==true) fail("detached by mistake (5)");
    Thread::sleep(10);
    if(pthread_mutex_destroy(&mutex)==0) fail("mutex destroy");
    pthread_join(thread,NULL);
    Thread::sleep(10);
    if(pthread_mutex_destroy(&mutex)!=0) fail("mutex destroy (2)");
    //
    // Testing pthread_cond_*
    //
    if(pthread_create(&thread,NULL,t16_p3,NULL)!=0) fail("pthread_create (8)");
    t=toThread(thread);
    if(Thread::exists(t)==false) fail("thread not created (7)");
    if(t->isDetached()==true) fail("detached by mistake (6)");
    if(pthread_mutex_lock(&t16_m1)!=0) fail("mutex lock (5)"); //<---
    t16_v1=false;
    if(pthread_cond_wait(&t16_c1,&t16_m1)!=0) fail("wait");
    if(t16_v1==false) fail("did not really wait");
    if(pthread_mutex_unlock(&t16_m1)!=0) fail("mutex unlock (5)"); //<---
    pthread_join(thread,NULL);
    Thread::sleep(10);
    //now testing pthread_cond_init
    if(pthread_cond_init(&t16_c2,NULL)!=0) fail("cond_init");
    if(pthread_create(&thread,NULL,t16_p4,NULL)!=0) fail("pthread_create (9)");
    t=toThread(thread);
    if(Thread::exists(t)==false) fail("thread not created (8)");
    if(t->isDetached()==true) fail("detached by mistake (7)");
    if(pthread_mutex_lock(&t16_m1)!=0) fail("mutex lock (6)"); //<---
    t16_v1=false;
    if(pthread_cond_wait(&t16_c2,&t16_m1)!=0) fail("wait (2)");
    if(t16_v1==false) fail("did not really wait (2)");
    if(pthread_mutex_unlock(&t16_m1)!=0) fail("mutex unlock (6)"); //<---
    pthread_join(thread,NULL);
    Thread::sleep(10);
    if(pthread_cond_destroy(&t16_c2)!=0) fail("cond destroy");
    //
    // Testing pthread_once
    //
    //Note: implementation detail since otherwise by the very nature of
    //pthread_once, it wouldn't be possible to run the test more than once ;)
    if(t16_o1.init_executed==1) t16_o1.init_executed=0;
    a=0;
    if(pthread_once(&t16_o1,t16_f1)!=0) fail("pthread_once 1");
    if(a!=1) fail("pthread_once 2");
    if(pthread_once(&t16_o1,t16_f1)!=0) fail("pthread_once 2");
    if(a!=1) fail("pthread_once 3");
    if(sizeof(pthread_once_t)!=2) fail("pthread_once 4");
    pass();
}

//
// Test 17
//
/*
tests:
C++ static constructors
*/

static unsigned char t17_v1=0;

class TestStaticConstructor
{
public:
    TestStaticConstructor(): a(0x1234567), b(0x89abcdef) {}

    bool isOk()
    {
        return a==0x1234567 && b==0x89abcdef;
    }

    unsigned int a,b;
};

static TestStaticConstructor& instance()
{
    static TestStaticConstructor singleton;
    return singleton;
}

class TestOneCallToStaticConstructor
{
public:
    TestOneCallToStaticConstructor() { t17_v1++; }
};

TestOneCallToStaticConstructor t17_v2;

static void test_17()
{
    test_name("static constructors");
    if(instance().isOk()==false)
    {
        iprintf("a=0x%x, b=0x%x\n",instance().a, instance().b);
        fail("constructor fail");
    }
    // Fails both if never called or if called multiple times 
    if(t17_v1!=1) fail("Constructor error");
    pass();
}

//
// Test 18
//
/*
tests:
endianness API
*/

void __attribute__((noinline)) check16(unsigned short a, unsigned short b)
{
    if(swapBytes16(a)!=b) fail("swapBytes16");
}

void __attribute__((noinline)) check32(unsigned int a, unsigned int b)
{
    if(swapBytes32(a)!=b) fail("swapBytes32");
}

void __attribute__((noinline)) check64(unsigned long long a,
        unsigned long long b)
{
    if(swapBytes64(a)!=b) fail("swapBytes64");
}

static void test_18()
{
    test_name("endianness");
    if(swapBytes16(0x1234)!=0x3412 ||
       swapBytes16(0x55aa)!=0xaa55) fail("swapBytes16");
    if(swapBytes32(0x12345678)!=0x78563412 ||
       swapBytes32(0x55aa00ff)!=0xff00aa55) fail("swapBytes32");
    if(swapBytes64(0x0123456789abcdefull)!=0xefcdab8967452301ull ||
       swapBytes64(0x55aa00ffcc33ab56ull)!=0x56ab33ccff00aa55ull)
        fail("swapBytes64");
    check16(0x1234,0x3412);
    check16(0x55aa,0xaa55);
    check32(0x12345678,0x78563412);
    check32(0x55aa00ff,0xff00aa55);
    check64(0x0123456789abcdefull,0xefcdab8967452301ull);
    check64(0x55aa00ffcc33ab56ull,0x56ab33ccff00aa55ull);
    union {
        short a;
        unsigned char b[2];
    } test;
    test.a=0x1234;
    if(test.b[0]==0x12)
    {
        //Runtime check says our CPU is big endian
        if(toBigEndian16(0x0123)!=0x0123 ||
           toBigEndian32(0x01234567)!=0x01234567 ||
           toBigEndian64(0x0123456789abcdef)!=0x0123456789abcdef)
            fail("toBigEndian");
        if(fromBigEndian16(0x0123)!=0x0123 ||
           fromBigEndian32(0x01234567)!=0x01234567 ||
           fromBigEndian64(0x0123456789abcdef)!=0x0123456789abcdef)
            fail("fromBigEndian");
        if(toLittleEndian16(0x0123)!=0x2301 ||
           toLittleEndian32(0x01234567)!=0x67452301 ||
           toLittleEndian64(0x0123456789abcdef)!=0xefcdab8967452301)
            fail("toLittleEndian");
        if(fromLittleEndian16(0x0123)!=0x2301 ||
           fromLittleEndian32(0x01234567)!=0x67452301 ||
           fromLittleEndian64(0x0123456789abcdef)!=0xefcdab8967452301)
            fail("fromLittleEndian");
    } else {
        //Runtime check says our CPU is little endian
        if(toLittleEndian16(0x0123)!=0x0123 ||
           toLittleEndian32(0x01234567)!=0x01234567 ||
           toLittleEndian64(0x0123456789abcdef)!=0x0123456789abcdef)
            fail("toLittleEndian");
        if(fromLittleEndian16(0x0123)!=0x0123 ||
           fromLittleEndian32(0x01234567)!=0x01234567 ||
           fromLittleEndian64(0x0123456789abcdef)!=0x0123456789abcdef)
            fail("fromLittleEndian");
        if(toBigEndian16(0x0123)!=0x2301 ||
           toBigEndian32(0x01234567)!=0x67452301 ||
           toBigEndian64(0x0123456789abcdef)!=0xefcdab8967452301)
            fail("toBigEndian");
        if(fromBigEndian16(0x0123)!=0x2301 ||
           fromBigEndian32(0x01234567)!=0x67452301 ||
           fromBigEndian64(0x0123456789abcdef)!=0xefcdab8967452301)
            fail("fromBigEndian");
    }
    pass();
}

//
// Test 19
//
/*
tests:
class BufferQueue
*/

BufferQueue<char,10,3> bq;
Thread *t19_v1;

static const char b1c[]="b1c----";
static const char b2c[]="b2c----x";
static const char b3c[]="b3c----xx";
static const char b4c[]="";

static char *IRQgbw(FastInterruptDisableLock& dLock)
{
    char *buffer=0;
    if(bq.tryGetWritableBuffer(buffer)==false)
    {
        FastInterruptEnableLock eLock(dLock);
        fail("BufferQueue::get");
    }
    return buffer;
}

static void gbr(const char *&buffer, unsigned int& size)
{
    FastInterruptDisableLock dLock;
    while(bq.tryGetReadableBuffer(buffer,size)==false)
    {
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
}

static void be()
{
    FastInterruptDisableLock dLock;
    bq.bufferEmptied();
}

static void t19_p1(void *argv)
{
    Thread::sleep(50);
    {
        FastInterruptDisableLock dLock;
        char *buffer=IRQgbw(dLock);
        strcpy(buffer,b1c);
        bq.bufferFilled(strlen(b1c));
        t19_v1->IRQwakeup();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::sleep(10);
        }
        buffer=IRQgbw(dLock);
        strcpy(buffer,b2c);
        bq.bufferFilled(strlen(b2c));
        t19_v1->IRQwakeup();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::sleep(10);
        }
        buffer=IRQgbw(dLock);
        strcpy(buffer,b3c);
        bq.bufferFilled(strlen(b3c));
        t19_v1->IRQwakeup();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::sleep(10);
        }
        buffer=IRQgbw(dLock);
        strcpy(buffer,b4c);
        bq.bufferFilled(strlen(b4c));
        t19_v1->IRQwakeup();
    }
}

static void test_19()
{
    test_name("BufferQueue");
    if(bq.bufferMaxSize()!=10) fail("bufferMaxSize");
    if(bq.numberOfBuffers()!=3) fail("numberOfBuffers");
    //NOTE: in theory we should disable interrupts before calling these, but
    //since we are accessing it from one thread only, for now it isn't required
    if(bq.isEmpty()==false) fail("IRQisEmpty");
    if(bq.isFull()==true) fail("IRQisFull");

    //Test filling only one slot
    char *buf=0;
    const char *buffer=0;
    unsigned int size;
    if(bq.tryGetReadableBuffer(buffer,size)==true) fail("IRQgetReadableBuffer");
    if(bq.tryGetWritableBuffer(buf)==false) fail("IRQgetWritableBuffer");
    const char b1a[]="b1a";
    strcpy(buf,b1a);
    bq.bufferFilled(strlen(b1a));
    buf=0;
    if(bq.isEmpty()==true) fail("IRQisEmpty");
    if(bq.isFull()==true) fail("IRQisFull");
    if(bq.tryGetReadableBuffer(buffer,size)==false) fail("IRQgetReadableBuffer");
    if(size!=strlen(b1a)) fail("returned size");
    if(strcmp(buffer,b1a)!=0) fail("returned buffer");
    bq.bufferEmptied();
    if(bq.isEmpty()==false) fail("IRQisEmpty");
    if(bq.isFull()==true) fail("IRQisFull");
    if(bq.tryGetReadableBuffer(buffer,size)==true) fail("IRQgetReadableBuffer");
    if(bq.tryGetWritableBuffer(buf)==false) fail("IRQgetWritableBuffer");

    //Test filling all three slots
    const char b1b[]="b1b0";
    strcpy(buf,b1b);
    bq.bufferFilled(strlen(b1b));
    buf=0;
    if(bq.isEmpty()==true) fail("IRQisEmpty");
    if(bq.isFull()==true) fail("IRQisFull");
    if(bq.tryGetWritableBuffer(buf)==false) fail("IRQgetWritableBuffer");
    const char b2b[]="b2b01";
    strcpy(buf,b2b);
    bq.bufferFilled(strlen(b2b));
    buf=0;
    if(bq.isEmpty()==true) fail("IRQisEmpty");
    if(bq.isFull()==true) fail("IRQisFull");
    if(bq.tryGetWritableBuffer(buf)==false) fail("IRQgetWritableBuffer");
    const char b3b[]="b2b012";
    strcpy(buf,b3b);
    bq.bufferFilled(strlen(b3b));
    buf=0;
    if(bq.isEmpty()==true) fail("IRQisEmpty");
    if(bq.isFull()==false) fail("IRQisFull");
    if(bq.tryGetWritableBuffer(buf)==true) fail("IRQgetWritableBuffer");
    buf=0;
    //Filled entirely, now emptying
    if(bq.tryGetReadableBuffer(buffer,size)==false) fail("IRQgetReadableBuffer");
    if(size!=strlen(b1b)) fail("returned size");
    if(strcmp(buffer,b1b)!=0) fail("returned buffer");
    bq.bufferEmptied();
    if(bq.isEmpty()==true) fail("IRQisEmpty");
    if(bq.isFull()==true) fail("IRQisFull");
    if(bq.tryGetReadableBuffer(buffer,size)==false) fail("IRQgetReadableBuffer");
    if(size!=strlen(b2b)) fail("returned size");
    if(strcmp(buffer,b2b)!=0) fail("returned buffer");
    bq.bufferEmptied();
    if(bq.isEmpty()==true) fail("IRQisEmpty");
    if(bq.isFull()==true) fail("IRQisFull");
    if(bq.tryGetReadableBuffer(buffer,size)==false) fail("IRQgetReadableBuffer");
    if(size!=strlen(b3b)) fail("returned size");
    if(strcmp(buffer,b3b)!=0) fail("returned buffer");
    bq.bufferEmptied();
    if(bq.tryGetReadableBuffer(buffer,size)==true) fail("IRQgetReadableBuffer");
    if(bq.isEmpty()==false) fail("IRQisEmpty");
    if(bq.isFull()==true) fail("IRQisFull");

    //Now real multithreaded test
    t19_v1=Thread::getCurrentThread();
    Thread *t=Thread::create(t19_p1,STACK_MIN,0,0,Thread::JOINABLE);
    gbr(buffer,size);
    if(size!=strlen(b1c)) fail("returned size");
    if(strcmp(buffer,b1c)!=0) fail("returned buffer");
    be();
    gbr(buffer,size);
    if(size!=strlen(b2c)) fail("returned size");
    if(strcmp(buffer,b2c)!=0) fail("returned buffer");
    be();
    gbr(buffer,size);
    if(size!=strlen(b3c)) fail("returned size");
    if(strcmp(buffer,b3c)!=0) fail("returned buffer");
    be();
    gbr(buffer,size);
    if(size!=strlen(b4c)) fail("returned size");
    if(strcmp(buffer,b4c)!=0) fail("returned buffer");
    be();
    t->join();
    if(bq.isEmpty()==false) fail("IRQisEmpty");
    if(bq.isFull()==true) fail("IRQisFull");

    //Last, check Reset (again, single thread mode)
    if(bq.tryGetWritableBuffer(buf)==false) fail("IRQgetWritableBuffer");
    strcpy(buf,b1a);
    bq.bufferFilled(strlen(b1a));
    bq.reset();
    if(bq.isEmpty()==false) fail("IRQisEmpty");
    if(bq.isFull()==true) fail("IRQisFull");
    pass();
}

//
// Test 20
//
/*
tests:
class Callback
class EventQueue
class FixedEventQueue
*/

int t20_v1;

void t20_f1()
{
    t20_v1=1234;
}

void t20_f2(int a, int b)
{
    t20_v1=a+b;
}

class T20_c1
{
public:
    T20_c1() : x(0) {}
    
	void h()
    {
        x=4321;
    }
	
    void k(int a, int b)
    {
        x=a*b;
    }
    
    int get() const { return x; }
private:
    int x;
};

#ifndef __NO_EXCEPTIONS

void thrower()
{
    throw 5;
}

void t20_t1(void* arg)
{
    EventQueue *eq=reinterpret_cast<EventQueue*>(arg);
    t20_v1=0;
    eq->post(t20_f1);
    Thread::sleep(10);
    if(t20_v1!=1234) fail("Not called");
    
    t20_v1=0;
    eq->post(t20_f1);
    eq->post(bind(t20_f2,5,5)); //Checking event ordering
    Thread::sleep(10);
    if(t20_v1!=10) fail("Not called");
    
    eq->post(thrower);
}

void t20_t2(void* arg)
{
    FixedEventQueue<2> *eq=reinterpret_cast<FixedEventQueue<2>*>(arg);
    t20_v1=0;
    eq->post(t20_f1);
    eq->post(t20_f1);
    unsigned long long t1=getTick();
    eq->post(bind(t20_f2,10,4)); //This should block
    unsigned long long t2=getTick();
    //The other thread sleep for 50ms before calling run()
    if((t2-t1)<static_cast<unsigned long long>(TICK_FREQ*0.04))
        fail("Not blocked");
    Thread::sleep(10);
    if(t20_v1!=14) fail("Not called");
    
    Thread::sleep(10);
    eq->post(thrower);
}
#endif //__NO_EXCEPTIONS

static void test_20()
{
    test_name("Event system");
    //
    // Testing Callback
    //
    Callback<20> cb;

    t20_v1=0;
	cb=t20_f1; //4 bytes
	Callback<20> cb2(cb);
	cb2();
    if(t20_v1!=1234) fail("Callback");

	cb=bind(t20_f2,12,2); //12 bytes
	cb2=cb;
	cb2();
    if(t20_v1!=14) fail("Callback");
    
	T20_c1 c;
	cb=bind(&T20_c1::h,&c); //12 bytes
	cb2=cb;
	cb2();
    if(c.get()!=4321) fail("Callback");

	cb=bind(&T20_c1::k,&c,10,15); //20 bytes
	cb2=cb;
	cb2();
    if(c.get()!=150) fail("Callback");

	cb=bind(&T20_c1::k,ref(c),12,12); //20 bytes
	cb2=cb;
	cb2();
    if(c.get()!=144) fail("Callback");

	cb.clear();
	cb2=cb;
	if(cb2) fail("Empty callback");
    
    //
    // Testing EventQueue
    //
    EventQueue eq;
    if(eq.empty()==false || eq.size()!=0) fail("Empty EventQueue");
    
    eq.runOne(); //This tests that runOne() does not block
    
    t20_v1=0;
    eq.post(t20_f1);
    if(t20_v1!=0) fail("Too early");
    if(eq.empty() || eq.size()!=1) fail("Not empty EventQueue");
    eq.runOne();
    if(t20_v1!=1234) fail("Not called");
    if(eq.empty()==false || eq.size()!=0) fail("Empty EventQueue");
    
    t20_v1=0;
    eq.post(t20_f1);
    eq.post(bind(t20_f2,2,3));
    if(t20_v1!=0) fail("Too early");
    if(eq.empty() || eq.size()!=2) fail("Not empty EventQueue");
    eq.runOne();
    if(t20_v1!=1234) fail("Not called");
    if(eq.empty() || eq.size()!=1) fail("Not empty EventQueue");
    eq.runOne();
    if(t20_v1!=5) fail("Not called");
    if(eq.empty()==false || eq.size()!=0) fail("Empty EventQueue");
    
    #ifndef __NO_EXCEPTIONS
    Thread *t=Thread::create(t20_t1,STACK_SMALL,0,&eq,Thread::JOINABLE);
    try {
        eq.run();
        fail("run() returned");
    } catch(int i) {
        if(i!=5) fail("Wrong");
    }
    t->join();
    if(eq.empty()==false || eq.size()!=0) fail("Empty EventQueue");
    #endif //__NO_EXCEPTIONS
    
    //
    // Testing EventQueue
    //
    FixedEventQueue<2> feq;
    if(feq.empty()==false || feq.size()!=0) fail("Empty EventQueue");
    
    feq.runOne(); //This tests that runOne() does not block
    
    t20_v1=0;
    feq.post(t20_f1);
    if(t20_v1!=0) fail("Too early");
    if(feq.empty() || feq.size()!=1) fail("Not empty EventQueue");
    feq.runOne();
    if(t20_v1!=1234) fail("Not called");
    if(feq.empty()==false || feq.size()!=0) fail("Empty EventQueue");
    
    t20_v1=0;
    feq.post(t20_f1);
    feq.post(bind(t20_f2,2,3));
    if(t20_v1!=0) fail("Too early");
    if(feq.empty() || feq.size()!=2) fail("Not empty EventQueue");
    feq.runOne();
    if(t20_v1!=1234) fail("Not called");
    if(feq.empty() || feq.size()!=1) fail("Not empty EventQueue");
    feq.runOne();
    if(t20_v1!=5) fail("Not called");
    if(feq.empty()==false || feq.size()!=0) fail("Empty EventQueue");
    
    t20_v1=0;
    feq.post(t20_f1);
    if(feq.postNonBlocking(bind(t20_f2,2,3))==false) fail("PostNonBlocking 1");
    if(feq.postNonBlocking(t20_f1)==true) fail("PostNonBlocking 2");
    if(t20_v1!=0) fail("Too early");
    if(feq.empty() || feq.size()!=2) fail("Not empty EventQueue");
    feq.runOne();
    if(t20_v1!=1234) fail("Not called");
    if(feq.empty() || feq.size()!=1) fail("Not empty EventQueue");
    feq.runOne();
    if(t20_v1!=5) fail("Not called");
    if(feq.empty()==false || feq.size()!=0) fail("Empty EventQueue");
    
    #ifndef __NO_EXCEPTIONS
    t=Thread::create(t20_t2,STACK_SMALL,0,&feq,Thread::JOINABLE);
    Thread::sleep(50);
    try {
        feq.run();
        fail("run() returned");
    } catch(int i) {
        if(i!=5) fail("Wrong");
    }
    t->join();
    if(feq.empty()==false || feq.size()!=0) fail("Empty EventQueue");
    #endif //__NO_EXCEPTIONS
    
    
    pass();
}

//
// Test 21
//
/*
tests:
floating point access from multiple threads (mostly of interest for
architectures with hardware floating point whose state has to be preserved
among context switches) 
*/

static float t21_f1()
{
    static volatile float f1=3.0f; //Volatile to prevent compiler optimizations
    float result=f1;
    for(int i=0;i<10000;i++) result=(result+f1/result)/2.0f;
    return result;
}

static float t21_f2()
{
    static volatile float f2=2.0f; //Volatile to prevent compiler optimizations
    float result=f2;
    for(int i=0;i<10000;i++) result=(result+f2/result)/2.0f;
    return result;
}

static void *t21_t1(void*)
{
    for(int i=0;i<5;i++)
    {
        volatile float value=t21_f1();
        if(fabsf(value-sqrt(3.0f))>0.00001f) fail("thread1");
    }
    return 0;
}

static void test_21()
{
    test_name("Floating point");
    pthread_t t;
    pthread_create(&t,0,t21_t1,0);
    for(int i=0;i<5;i++)
    {
        volatile float value=t21_f2();
        if(fabsf(value-sqrt(2.0f))>0.00001f) fail("main");
    }
    pthread_join(t,0);
    pass();
}

//
// Test 22
//
/*
tests:
__atomic_add()
__exchange_and_add()
These are not actually in the kernel but in the patches to gcc
also tests atomic operations provided by miosix (interfaces/atomic_ops.h)
*/

static int t22_v1;
static int t22_v2;
static int t22_v3;
static int t22_v4;

static bool t22_v5;

struct t22_s1
{
    int a;
    int b;
};

static void t22_t2(void *argv)
{
    while(Thread::testTerminate()==false)
    {
        t22_v5=true;
        Thread::yield();
    }
}

static void *t22_t1(void*)
{
	for(int i=0;i<100000;i++)
	{
		__gnu_cxx::__atomic_add(&t22_v1,1);
		__gnu_cxx::__exchange_and_add(&t22_v2,-1);
        atomicAdd(&t22_v3,1);
        atomicAddExchange(&t22_v4,1);
	}
	return 0;
}

static void test_22()
{
    test_name("Atomic ops");
    
    //Check thread safety for atomic ops (both GCC provided and the miosix ones)
    t22_v1=t22_v2=t22_v3=t22_v4=0;
    pthread_t t;
    pthread_create(&t,0,t22_t1,0);
    for(int i=0;i<100000;i++)
    {
        __gnu_cxx::__atomic_add(&t22_v1,-1);
        __gnu_cxx::__exchange_and_add(&t22_v2,1);
        atomicAdd(&t22_v3,-1);
        atomicAddExchange(&t22_v4,-1);
    }
    pthread_join(t,0);
    if(t22_v1!=0 || t22_v2!=0 || t22_v3!=0 || t22_v4!=0)
        fail("not thread safe");
    
    //Functional test for miosix atomic ops
    int x=10;
    if(atomicSwap(&x,20)!=10) fail("atomicSwap 1");
    if(x!=20) fail("atomicSwap 2");
    
    x=10;
    atomicAdd(&x,-5);
    if(x!=5) fail("atomicAdd");
    
    x=10;
    if(atomicAddExchange(&x,5)!=10) fail("atomicAddExchange 1");
    if(x!=15) fail("atomicAddExchange 2");
    
    x=10;
    if(atomicCompareAndSwap(&x,11,12)!=10) fail("atomicCompareAndSwap 1");
    if(x!=10) fail("atomicCompareAndSwap 2");
    if(atomicCompareAndSwap(&x,10,13)!=10) fail("atomicCompareAndSwap 3");
    if(x!=13) fail("atomicCompareAndSwap 4");
    
    t22_s1 data;
    t22_s1 *dataPtr=&data;
    void * const volatile *ptr=
        reinterpret_cast<void * const volatile *>(&dataPtr);
    data.a=0;
    data.b=10;
    
    if(atomicFetchAndIncrement(ptr,0,2)!=dataPtr)
        fail("atomicFetchAndIncrement 1");
    if(data.a!=2) fail("atomicFetchAndIncrement 2");
    if(atomicFetchAndIncrement(ptr,1,-2)!=dataPtr)
        fail("atomicFetchAndIncrement 3");
    if(data.b!=8) fail("atomicFetchAndIncrement 4");
    
    //Check that the implementation works with interrupts disabled too
    bool error=false;
    Thread *t2=Thread::create(t22_t2,STACK_MIN,0,0,Thread::JOINABLE);
    {
        FastInterruptDisableLock dLock;
        t22_v5=false;
        
        int x=10;
        if(atomicSwap(&x,20)!=10) error=true;
        if(x!=20) error=true;
        
        delayMs(5); //Wait to check that interrupts are disabled

        x=10;
        atomicAdd(&x,-5);
        if(x!=5) error=true;
        
        delayMs(5); //Wait to check that interrupts are disabled

        x=10;
        if(atomicAddExchange(&x,5)!=10) error=true;
        if(x!=15) error=true;
        
        delayMs(5); //Wait to check that interrupts are disabled

        x=10;
        if(atomicCompareAndSwap(&x,11,12)!=10) error=true;
        if(x!=10) error=true;
        if(atomicCompareAndSwap(&x,10,13)!=10) error=true;
        if(x!=13) error=true;
        
        delayMs(5); //Wait to check that interrupts are disabled

        t22_s1 data;
        t22_s1 *dataPtr=&data;
        void * const volatile *ptr=
            reinterpret_cast<void * const volatile *>(&dataPtr);
        data.a=0;
        data.b=10;
        if(atomicFetchAndIncrement(ptr,0,2)!=dataPtr) error=true;
        if(data.a!=2) error=true;
        if(atomicFetchAndIncrement(ptr,1,-2)!=dataPtr) error=true;
        if(data.b!=8) error=true;
        
        delayMs(5); //Wait to check that interrupts are disabled
        if(t22_v5) error=true;
    }
    if(error) fail("Interrupt test not passed");
    
    t2->terminate();
    t2->join();
    
    pass();
}

//
// Test 23
//

/*
tests:
interaction between condition variables and recursive mutexes
*/
static Mutex t23_m1(Mutex::RECURSIVE);
static ConditionVariable t23_c1;

static FastMutex t23_m2(FastMutex::RECURSIVE);
static ConditionVariable t23_c2;

static pthread_mutex_t t23_m3=PTHREAD_MUTEX_RECURSIVE_INITIALIZER_NP;
static pthread_cond_t t23_c3=PTHREAD_COND_INITIALIZER;

static void t23_f1(void*)
{
    Lock<Mutex> l(t23_m1);
    t23_c1.signal();
}

static void t23_f2(void*)
{
    Lock<FastMutex> l(t23_m2);
    t23_c2.signal();
}

static void t23_f3(void*)
{
    pthread_mutex_lock(&t23_m3);
    pthread_cond_signal(&t23_c3);
    pthread_mutex_unlock(&t23_m3);
}

static void test_23()
{
    test_name("Condition variable and recursive mutexes");
    
    {
        Lock<Mutex> l(t23_m1);
        Lock<Mutex> l2(t23_m1);
        Thread *t=Thread::create(t23_f1,2*STACK_MIN,1,0,Thread::JOINABLE);
        t23_c1.wait(l2);
        t->join();
    }
    {
        Lock<FastMutex> l(t23_m2);
        Lock<FastMutex> l2(t23_m2);
        Thread *t=Thread::create(t23_f2,2*STACK_MIN,1,0,Thread::JOINABLE);
        t23_c2.wait(l2);
        t->join();
    }
    pthread_mutex_lock(&t23_m3);
    pthread_mutex_lock(&t23_m3);
    Thread *t=Thread::create(t23_f3,2*STACK_MIN,1,0,Thread::JOINABLE);
    pthread_cond_wait(&t23_c3,&t23_m3);
    t->join();
    pthread_mutex_unlock(&t23_m3);
    pthread_mutex_unlock(&t23_m3);
    
    pass();
}

//
// Test 24
//

/*
tests:
intrusive_ref_ptr
*/

static bool dtorCalled; // Set to true by class destructors

class Base1
{
public:
    Base1() : a(0) {}
    virtual void check() { assert(a==0); }
    virtual ~Base1() { dtorCalled=true; a=-1; }
private:
    int a;
};

class Middle1 : public Base1, public IntrusiveRefCounted
{
public:
    Middle1() : b(0) {}
    virtual void check() { Base1::check(); assert(b==0); }
    virtual ~Middle1() { b=-1; }
private:
    int b;
};

class Other
{
public:
    Other() : c(0) {}
    virtual void check() { assert(c==0); }
    virtual ~Other() { c=-1; }
private:
    int c;
};

class Derived1 : public Middle1, public Other
{
public:
    Derived1() : d(0) {}
    virtual void check() { Middle1::check(); Other::check(); assert(d==0); }
    virtual ~Derived1() { d=-1; }
private:
    int d;
};

class Derived2 : public Other, public Middle1
{
public:
    Derived2() : e(0) {}
    virtual void check() { Other::check(); Middle1::check(); assert(e==0); }
    virtual ~Derived2() { e=-1; }
private:
    int e;
};

class Base0 : public IntrusiveRefCounted
{
public:
    Base0() : f(0) {}
    virtual void check() { assert(f==0); }
    virtual ~Base0() { dtorCalled=true; f=-1; }
private:
    int f;
};

class Derived0 : public Base0
{
public:
    Derived0() : g(0) {}
    virtual void check() { Base0::check(); assert(g==0); }
    virtual ~Derived0() { g=-1; }
private:
    int g;
};

template<typename T, typename U>
void checkAtomicOps()
{
    dtorCalled=false;
    {
        intrusive_ref_ptr<T> ptr1(new U);
        intrusive_ref_ptr<T> ptr2;
        
        // Check atomic_load()
        ptr2=atomic_load(&ptr1);
        ptr2->check();
        assert(ptr1==ptr2);
        assert(dtorCalled==false);
        
        // Check atomic_store() on an empty intrusive_ref_ptr
        ptr1.reset();
        atomic_store(&ptr1,ptr2);
        ptr1->check();
        assert(ptr1==ptr2);
        assert(dtorCalled==false);
    }
    assert(dtorCalled);
    
    // Check atomic_store() on an intrusive_ref_ptr containing an object
    dtorCalled=false;
    {
        intrusive_ref_ptr<T> ptr1(new U);
        intrusive_ref_ptr<T> ptr2(new U);
        atomic_store(&ptr1,ptr2);
        assert(dtorCalled);
        dtorCalled=false;
        ptr1->check();
        assert(ptr1==ptr2);
    }
    assert(dtorCalled);
}

static intrusive_ref_ptr<Base0> threadShared; // Shared among threads

//static void *thread(void*)
//{
//    for(;;)
//    {
//        intrusive_ref_ptr<Base0> empty;
//        atomic_store(&threadShared,empty);
//        
//        intrusive_ref_ptr<Base0> result;
//        result=atomic_load(&threadShared);
//        if(result) result->check();
//        
//        intrusive_ref_ptr<Base0> full(new Base0);
//        atomic_store(&threadShared,full);
//        
//        result=atomic_load(&threadShared);
//        if(result) result->check();
//    }
//    return 0;
//}

static void test_24()
{
    test_name("intrusive_ref_ptr");
    
    // Default constructor
    dtorCalled=false;
    {
        intrusive_ref_ptr<Base0> ptr1;
        assert(ptr1==0);
    }
    assert(dtorCalled==false);
    
    // Simple use
    dtorCalled=false;
    {
        intrusive_ref_ptr<Base0> ptr1(new Base0);
        ptr1->check();
        
        Base0 *p=ptr1.get();
        p->check();
        
        Base0& r=*ptr1;
        r.check();
    }
    assert(dtorCalled);
    
    // Copy construction, nested
    dtorCalled=false;
    {
        intrusive_ref_ptr<Base0> ptr1(new Base0);
        {
            intrusive_ref_ptr<Base0> ptr2(ptr1);
            ptr2->check();
        }
        assert(dtorCalled==false);
    }
    assert(dtorCalled);
    
    // Copy construction, interleaved
    dtorCalled=false;
    {
        intrusive_ref_ptr<Base0> *ptr1=new intrusive_ref_ptr<Base0>(new Base0);
        intrusive_ref_ptr<Base0> *ptr2=new intrusive_ref_ptr<Base0>(*ptr1);
        (*ptr2)->check();
        delete ptr1;
        assert(dtorCalled==false);
        delete ptr2;
    }
    assert(dtorCalled);
    
    // Operator=
    dtorCalled=false;
    {
        intrusive_ref_ptr<Base0> ptr1(new Base0);
        {
            intrusive_ref_ptr<Base0> ptr2(new Base0);
            ptr2=ptr1; // Replaces the instance, deletes it
            assert(dtorCalled);
            dtorCalled=false;
            ptr2->check();
        }
        assert(dtorCalled==false);
    }
    assert(dtorCalled);
    
    // Operator=, with raw pointer
    dtorCalled=false;
    {
        intrusive_ref_ptr<Base0> ptr1(new Base0);
        ptr1=new Base0; // Replaces the instance, deletes it
        assert(dtorCalled);
        dtorCalled=false;
        ptr1->check();
    }
    assert(dtorCalled);
    
    // Upcasting, with copy constructor
    dtorCalled=false;
    {
        intrusive_ref_ptr<Derived0> ptr1(new Derived0);
        {
            intrusive_ref_ptr<Base0> ptr2(ptr1);
            ptr2->check();
        }
        assert(dtorCalled==false);
    }
    assert(dtorCalled);
    
    // Upcasting, with operator=
    dtorCalled=false;
    {
        intrusive_ref_ptr<Derived0> ptr1(new Derived0);
        {
            intrusive_ref_ptr<Base0> ptr2(new Base0);
            ptr2=ptr1; // Replaces the instance, deletes it
            assert(dtorCalled);
            dtorCalled=false;
            ptr2->check();
        }
        assert(dtorCalled==false);
    }
    assert(dtorCalled);
    
    //dynamic_pointer_cast requires RTTI, so disable testing if no exceptions
    #ifndef __NO_EXCEPTIONS
    // Successful downcasting
    dtorCalled=false;
    {
        intrusive_ref_ptr<Middle1> ptr1(new Derived1);
        ptr1->check();
        {
            intrusive_ref_ptr<Derived1> ptr2=
                dynamic_pointer_cast<Derived1>(ptr1);
            assert(ptr2);
            ptr2->check();
        }
        assert(dtorCalled==false);
    }
    assert(dtorCalled);
    
    // Failed downcasting
    dtorCalled=false;
    {
        intrusive_ref_ptr<Middle1> ptr1(new Derived2);
        {
            intrusive_ref_ptr<Derived1> ptr2=
                dynamic_pointer_cast<Derived1>(ptr1);
            assert(ptr2==0);
        }
        assert(dtorCalled==false);
    }
    assert(dtorCalled);
    
    // Swap
    dtorCalled=false;
    {
        intrusive_ref_ptr<Base0> ptr1(new Base0);
        {
            intrusive_ref_ptr<Base0> ptr2(new Derived0);
            ptr1.swap(ptr2);
            assert(dtorCalled==false);
            ptr1->check();
            ptr2->check();
        }
        assert(dtorCalled);
        dtorCalled=false;
        assert(dynamic_pointer_cast<Derived0>(ptr1));
    }
    assert(dtorCalled);
    #endif //__NO_EXCEPTIONS

    // Reset, on an unshared pointer
    dtorCalled=false;
    {
        intrusive_ref_ptr<Base0> ptr1(new Base0);
        ptr1.reset();
        assert(dtorCalled);
        assert(ptr1==0);
    }
    
    // Reset, on a shared pointer
    dtorCalled=false;
    {
        intrusive_ref_ptr<Base0> ptr1(new Base0);
        {
            intrusive_ref_ptr<Base0> ptr2(ptr1);
            ptr1.reset();
            assert(ptr1==0);
            assert(ptr2!=0);
            ptr2->check();
        }
        assert(dtorCalled);
    }
    
    // atomic_load(), atomic_store(), directly derived from intrusive_ref_ptr
    checkAtomicOps<Middle1,Middle1>();
    checkAtomicOps<Base0,Base0>();
    
    // atomic_load(), atomic_store(), indirectly derived from intrusive_ref_ptr
    checkAtomicOps<Derived0,Derived0>();
    checkAtomicOps<Derived1,Derived1>();
    checkAtomicOps<Derived2,Derived2>();
    
    // atomic_load(), atomic_store(), with polimorphism
    checkAtomicOps<Base0,Derived0>();
    checkAtomicOps<Middle1,Derived1>();
    checkAtomicOps<Middle1,Derived2>();
    
    // Thread safety of atomic_load() and atomic_store()
    // Actually, this test never ends, and to adequately stress the
    // synchronizations on a single core machine a delay needs to be inserted
    // between the ldrex and strex into the implementation of
    // interfaces/atomic_ops.h. This is why this part of the test is not done
//    pthread_t t1,t2,t3;
//    pthread_create(&t1,NULL,thread,NULL);
//    pthread_create(&t2,NULL,thread,NULL);
//    pthread_create(&t3,NULL,okThread,NULL);
//    pthread_join(t1,NULL);
    
    pass();
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
    int result=mkdir("/sd/testdir",0);
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
    //Test concurrent file write access
    fs_1_error=false;
    Thread *t1=Thread::create(fs_t1_p1,2048+512,1,NULL,Thread::JOINABLE);
    Thread *t2=Thread::create(fs_t1_p2,2048+512,1,NULL,Thread::JOINABLE);
    Thread *t3=Thread::create(fs_t1_p3,2048+512,1,NULL,Thread::JOINABLE);
    t1->join();
    t2->join();
    t3->join();
    if(fs_1_error) fail("Concurrent write");
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
mkdir/unlink/rename
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
    test_name("mkdir/rename/unlink");
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
opendir()/readdir()
*/

unsigned int checkInodes(const char *dir, unsigned int curInode,
        unsigned int parentInode, short curDev, short parentDev)
{
    if(chdir(dir)) fail("chdir");
    
    DIR *d=opendir(".");
    if(d==NULL) fail("opendir");
    puts(dir);
    std::set<unsigned int> inodes;
    unsigned int result=0;
    for(;;)
    {
        struct dirent *de=readdir(d);
        if(de==NULL) break;
        
        struct stat st;
        if(stat(de->d_name,&st)) fail("stat");
        
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
        
        printf("inode=%lu dev=%d %s\n",st.st_ino,st.st_dev,de->d_name);
    }
    closedir(d);
    
    if(chdir("/")) fail("chdir");
    return result;
}

static void fs_test_4()
{
    test_name("Directory listing");
    unsigned int curInode=0, parentInode=0, devFsInode=0, sdInode=0;
    short curDevice=0, devDevice=0, sdDevice=0;
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
            if(de->d_type!=DT_DIR) fail("d_type");
            devFsInode=st.st_ino;
            devDevice=st.st_dev;
        } else if(!strcmp(de->d_name,"sd")) {
            if(de->d_type!=DT_DIR) fail("d_type");
            sdInode=st.st_ino;
            sdDevice=st.st_dev;
        }
        
        printf("inode=%lu dev=%d %s\n",st.st_ino,st.st_dev,de->d_name);
    }
    closedir(d);
    
    if(curInode!=parentInode) fail("/..");
    
    #ifdef WITH_DEVFS
    if(devFsInode==0 || devDevice==0) fail("dev");
    checkInodes("/dev",devFsInode,curInode,devDevice,curDevice);
    #endif //WITH_DEVFS
    if(sdInode==0 || sdDevice==0) fail("sd");
    int testdirIno=checkInodes("/sd",sdInode,curInode,sdDevice,curDevice);
    if(testdirIno==0) fail("no testdir");
    checkInodes("/sd/testdir",testdirIno,sdInode,sdDevice,sdDevice);
    pass();
}
#endif //WITH_FILESYSTEM

//
// C++ exceptions thread safety test
//

#ifndef __NO_EXCEPTIONS
const int nThreads=8;
bool flags[nThreads];

static int throwable(std::vector<int>& v) __attribute__((noinline));
static int throwable(std::vector<int>& v)
{
	return v.at(10);
}

static void test(void *argv)
{
	const int n=reinterpret_cast<int>(argv);
	for(;;)
	{
		try {
			std::vector<int> v;
			v.push_back(10);
			throwable(v);
            fail("Exception not thrown");
		} catch(std::out_of_range& e)
		{
			flags[n]=true;
		}
	}
}

static void exception_test()
{
    test_name("C++ exception thread safety");
    iprintf("Note: test never ends. Reset the board when you are satisfied\n");
	for(int i=0;i<nThreads;i++)
        Thread::create(test,1024+512,0,reinterpret_cast<void*>(i));
	bool toggle=false;
	for(int j=0;;j++)
	{
		Thread::sleep(200);
		if(toggle) ledOn(); else ledOff();
		toggle^=1;
		bool failed=false;
		{
			PauseKernelLock dLock;
			for(int i=0;i<nThreads;i++)
			{
				if(flags[i]==false) failed=true;
				flags[i]=false;
			}
		}
        if(failed) fail("Test failed");
		int heap=MemoryProfiling::getHeapSize()-
                 MemoryProfiling::getCurrentFreeHeap();
		iprintf("iteration=%d heap_used=%d\n",j,heap);
	}
}
#endif //__NO_EXCEPTIONS

//
// Benchmark 1
//
/*
tests:
serial write speed
*/

static void benchmark_1()
{
    Timer t;
    t.start();
    extern unsigned long _data asm("_data");
    char *data=reinterpret_cast<char*>(&_data);
    #ifndef _ARCH_ARM7_LPC2000
    memDump(data,2048);
    t.stop();
    //every line dumps 16 bytes, and is 81 char long (considering \r\n)
    //so (2048/16)*81=10368
    iprintf("Time required to print 10368 char is %dms\n",(
            t.interval()*1000)/TICK_FREQ);
    unsigned int baudrate=10368*10000/((t.interval()*1000)/TICK_FREQ);
    iprintf("Effective baud rate =%u\n",baudrate);
    #else //_ARCH_ARM7_LPC2000
    memDump(data,32768);
    t.stop();
    //every line dumps 16 bytes, and is 81 char long (considering \r\n)
    //so (32768/16)*81=165888
    iprintf("Time required to print 165888 char is %dms\n",(
            t.interval()*1000)/TICK_FREQ);
    unsigned int baudrate=165888*10000/((t.interval()*1000)/TICK_FREQ);
    iprintf("Effective baud rate =%u\n",baudrate);
    #endif //_ARCH_ARM7_LPC2000
}

//
// Benchmark 2
//
/*
tests:
context switch speed
*/

static void b2_p1(void *argv)
{
    for(;;)
    {
        if(Thread::testTerminate()) break;
        Thread::yield();
    }
}

static int b2_f1()
{
    int i=0;
    Timer t;
    t.start();
    for(;;)
    {
        //Since calling interval() on a running timer is not allowed,
        //we need to make a copy of the timer and stop the copy.
        Timer k(t);
        k.stop();
        if((unsigned int)k.interval()>=TICK_FREQ) break;
        i+=2;
        Thread::yield();
    }
    t.stop();
    return i;
}

static void benchmark_2()
{
    #ifndef SCHED_TYPE_EDF
    //Test context switch time at maximum priority
    Thread::setPriority(3);//Using max priority
    Thread *p=Thread::create(b2_p1,STACK_SMALL,3,NULL);
    int i=b2_f1();
    p->terminate();
    iprintf("%d context switch per second (max priority)\n",i);
    Thread::sleep(10);
    Thread::setPriority(0);//Restoring original priority
    p=Thread::create(b2_p1,STACK_SMALL,0,NULL);
    i=b2_f1();
    p->terminate();
    iprintf("%d context switch per second (min priority)\n",i);
    Thread::sleep(10);
    #else //SCHED_TYPE_EDF
    iprintf("Context switch benchmark not possible with edf\n");
    #endif //SCHED_TYPE_EDF
}

//
// Benchmark 3
//
/*
tests:
Fisesystem write speed and latency
makes a 1MB file and measures time required to read/write it.
*/

static void benchmark_3()
{
    //Write benchmark
    const char FILENAME[]="/sd/speed.txt";
    const unsigned int BUFSIZE=1024;
    char *buf=new char[BUFSIZE];
    memset ((void*)buf,'0',BUFSIZE);
    FILE *f;
    if((f=fopen(FILENAME,"w"))==NULL)
    {
        iprintf("Filesystem write benchmark not made. Can't open file\n");
        delete[] buf;
        return;
    }
    setbuf(f,NULL);
    Timer total,part;
    int i,max=0;
    total.start();
    for(i=0;i<1024;i++)
    {
        part.start();
        if(fwrite(buf,1,BUFSIZE,f)!=BUFSIZE)
        {
            iprintf("Write error\n");
            break;
        }
        part.stop();
        if(part.interval()>max) max=part.interval();
        part.clear();
    }
    total.stop();
    if(fclose(f)!=0) iprintf("Error in fclose 1\n");
    iprintf("Filesystem write benchmark\n");
    unsigned int writeTime=(total.interval()*1000)/TICK_FREQ;
    unsigned int writeSpeed=static_cast<unsigned int>(1024000.0/writeTime);
    iprintf("Total write time = %dms (%dKB/s)\n",writeTime,writeSpeed);
    iprintf("Max filesystem latency = %dms\n",(max*1000)/TICK_FREQ);
    //Read benchmark
    max=0;
    total.clear();
    if((f=fopen(FILENAME,"r"))==NULL)
    {
        iprintf("Filesystem read benchmark not made. Can't open file\n");
        delete[] buf;
        return;
    }
    setbuf(f,NULL);
    total.start();
    for(i=0;i<1024;i++)
    {
        memset(buf,0,BUFSIZE);
        part.start();
        if(fread(buf,1,BUFSIZE,f)!=BUFSIZE)
        {
            iprintf("Read error 1\n");
            break;
        }
        part.stop();
        if(part.interval()>max) max=part.interval();
        part.clear();
        for(unsigned j=0;j<BUFSIZE;j++) if(buf[j]!='0')
        {
            iprintf("Read error 2\n");
            goto quit;
        }
    }
    quit:
    total.stop();
    if(fclose(f)!=0) iprintf("Error in fclose 2\n");
    iprintf("Filesystem read test\n");
    unsigned int readTime=(total.interval()*1000)/TICK_FREQ;
    unsigned int readSpeed=static_cast<unsigned int>(1024000.0/readTime);
    iprintf("Total read time = %dms (%dKB/s)\n",readTime,readSpeed);
    iprintf("Max filesystem latency = %dms\n",(max*1000)/TICK_FREQ);
    delete[] buf;
}

//
// Benchmark 4
//
/*
tests:
Mutex lonk/unlock time
*/

volatile bool b4_end=false;

void b4_t1(void *argv)
{
    (void)argv;
    Thread::sleep(1000);
    b4_end=true;
}

static void benchmark_4()
{
    Mutex m;
    pthread_mutex_t m1=PTHREAD_MUTEX_INITIALIZER;
    b4_end=false;
    #ifndef SCHED_TYPE_EDF
    Thread::create(b4_t1,STACK_SMALL);
    #else
    Thread::create(b4_t1,STACK_SMALL,0);
    #endif
    Thread::yield();
    int i=0;
    while(b4_end==false)
    {
        m.lock();
        m.unlock();
        i++;
    }
    iprintf("%d Mutex lock/unlock pairs per second\n",i);

    b4_end=false;
    #ifndef SCHED_TYPE_EDF
    Thread::create(b4_t1,STACK_SMALL);
    #else
    Thread::create(b4_t1,STACK_SMALL,0);
    #endif
    Thread::yield();
    i=0;
    while(b4_end==false)
    {
        pthread_mutex_lock(&m1);
        pthread_mutex_unlock(&m1);
        i++;
    }
    iprintf("%d pthread_mutex lock/unlock pairs per second\n",i);

    b4_end=false;
    #ifndef SCHED_TYPE_EDF
    Thread::create(b4_t1,STACK_SMALL);
    #else
    Thread::create(b4_t1,STACK_SMALL,0);
    #endif
    Thread::yield();
    i=0;
    while(b4_end==false)
    {
        pauseKernel();
        restartKernel();
        i++;
    }
    iprintf("%d pause/restart kernel pairs per second\n",i);

    b4_end=false;
    #ifndef SCHED_TYPE_EDF
    Thread::create(b4_t1,STACK_SMALL);
    #else
    Thread::create(b4_t1,STACK_SMALL,0);
    #endif
    Thread::yield();
    i=0;
    while(b4_end==false)
    {
        disableInterrupts();
        enableInterrupts();
        i++;
    }
    iprintf("%d disable/enable interrupts pairs per second\n",i);

    b4_end=false;
    #ifndef SCHED_TYPE_EDF
    Thread::create(b4_t1,STACK_SMALL);
    #else
    Thread::create(b4_t1,STACK_SMALL,0);
    #endif
    Thread::yield();
    i=0;
    while(b4_end==false)
    {
        fastDisableInterrupts();
        fastEnableInterrupts();
        i++;
    }
    iprintf("%d fast disable/enable interrupts pairs per second\n",i);
}

#ifdef WITH_PROCESSES

unsigned int* memAllocation(unsigned int size)
{
	unsigned int *p = ProcessPool::instance().allocate(size);
	memset(p, WATERMARK_FILL, size);
	iprintf("Allocated %d bytes. Base: %p. Size: 0x%x.\n\n", size, p, size);
	return p;
}

// Returns true if a watermark filled memory zone is not corrupted.
// 'base' must be 4-byte aligned
bool memCheck(unsigned int *base, unsigned int size)
{
	for(unsigned int i = 0; i < size / 4; i++)
	{
		if(*(base + i) != WATERMARK_FILL)
			return false;
	}
	return true;
}

void runElfTest(const char *name, const unsigned char *filename, unsigned int file_length)
{
	iprintf("Executing %s...", name);
	try
	{
		ElfProgram prog(reinterpret_cast<const unsigned int*>(filename),file_length);
		iprintf("not passed.\n");
	}
	catch (std::runtime_error &err)
	{
		iprintf("passed.\n");
	}
}

//It runs the program, waits for its exit, and returns the exit code
int runProgram(const unsigned char *filename, unsigned int file_length)
{
	int ec;
	ElfProgram prog(reinterpret_cast<const unsigned int*>(filename), file_length);
	pid_t child=Process::create(prog);
	Process::waitpid(child, &ec, 0);
	return ec;
}

//Returns true if the process has been signaled with SIGSEV
bool isSignaled(int exit_code)
{
	if(WIFSIGNALED(exit_code) && WTERMSIG(exit_code)==SIGSEGV)
	{
		return true;
	}
	return false;
}

void mpuTest1()
{
	int ec;
	unsigned int *addr = (unsigned int*) 0x64100000;
	iprintf("Executing MPU Test 1...\n");
	ec = runProgram(test1_elf, test1_elf_len);
	if(isSignaled(ec))
	{
		if(*addr == 0xbbbbbbbb)
			iprintf("...not passed! The process has written a forbidden memory location.\n\n");
		else if(*addr == WATERMARK_FILL)
			iprintf("...passed!\n\n");
		else
			iprintf("...not passed! Memory has been somehow corrupted.\n\n");
	}
	else
	{
		iprintf("...not passed! Process exited normally.\n\n");
	}
}

void mpuTest2()
{
	int ec;
	unsigned int *addr = (unsigned int*) 0x64100200;
	iprintf("Executing MPU Test 2...\n");
	ec = runProgram(test2_elf, test2_elf_len);
	if(isSignaled(ec))
	{
		if(*addr == WATERMARK_FILL)
			iprintf("...passed!\n\n");
		else
			iprintf("...not passed! Memory has been somehow corrupted.\n\n");
	}
	else
	{
		iprintf("...not passed! Process exited normally.\n\n");
	}
}

void mpuTest3()
{
	int ec;
	unsigned int *addr = (unsigned int*) 0x64100200;
	iprintf("Executing MPU Test 3...\n");
	ec = runProgram(test3_elf, test3_elf_len);
	if(isSignaled(ec))
	{
		if(*addr == 0xbbbbbbbb)
			iprintf("...not passed! The process has written a forbidden memory location.\n\n");
		else
			iprintf("...passed!\n\n");
	}
	else
	{
		iprintf("...not passed! Process exited normally.\n\n");
	}
}

void mpuTest4()
{
	int ec;
	iprintf("Executing MPU Test 4...\n");
	ec = runProgram(test4_elf, test4_elf_len);
	if(isSignaled(ec))
	{
		iprintf("...passed!.\n\n");
	}
	else
	{
		iprintf("...not passed! Process exited normally.\n\n");
	}
}

void mpuTest5()
{
	int ec;
	unsigned int *addr = (unsigned int*) 0x64101000;
	iprintf("Executing MPU Test 5...\n");
	ec = runProgram(test5_elf, test5_elf_len);
	if(isSignaled(ec))
	{
		if(*addr == 0xbbbbbbbb)
			iprintf("...not passed! The process has written a forbidden memory location.\n\n");
		else
			iprintf("...passed!.\n\n");
	}
	else
	{
		iprintf("...not passed! Process exited normally.\n\n");
	}
}

void mpuTest6()
{
	int ec;
	unsigned int *addr = (unsigned int*) 0x64101404;
	iprintf("Executing MPU Test 6...\n");
	ec = runProgram(test6_elf, test6_elf_len);
	if(isSignaled(ec))
	{
		if(*addr == 0xbbbbbbbb)
			iprintf("...not passed! The process has written a forbidden memory location.\n\n");
		else
			iprintf("...passed!.\n\n");
	}
	else
	{
		iprintf("...not passed! Process exited normally.\n\n");
	}
}

void mpuTest7()
{
	int ec;
        unsigned int memSize = 8192;
        
        iprintf("Executing MPU Test 7...\n");
        unsigned int *p = ProcessPool::instance().allocate(memSize);
        memset(p, WATERMARK_FILL, memSize);
	ElfProgram prog(reinterpret_cast<const unsigned int*>(test7_elf), test7_elf_len);
	pid_t child=Process::create(prog);
	delayMs(1000);
	Process::waitpid(child, &ec, 0);
	if(isSignaled(ec))
	{       
                if(memCheck(p, memSize) == true)
                    iprintf("...passed!.\n\n");
                else
                    iprintf("...not passed! Memory NOT sane!");
                ProcessPool::instance().deallocate(p);
	}
	else
	{
		iprintf("...not passed! Process exited normally.\n\n");
                ProcessPool::instance().deallocate(p);
	}
}

void mpuTest8()
{
	// We create two processes. The first goes to sleep for 2 seconds,
	// while the second process tries to access the data region of the
	// first.
	unsigned int *addr = (unsigned int*) 0x64104004;
	iprintf("Executing MPU Test 8...\n");
	ElfProgram prog1(reinterpret_cast<const unsigned int*>(test8_1_elf),test8_1_elf_len);
	ElfProgram prog2(reinterpret_cast<const unsigned int*>(test8_2_elf),test8_2_elf_len);
	pid_t child1=Process::create(prog1);
	pid_t child2=Process::create(prog2);
	int ec1, ec2;
	Process::waitpid(child1,&ec1,0);
	Process::waitpid(child2,&ec2,0);
	if(WIFSIGNALED(ec2) && (WTERMSIG(ec2) == SIGSEGV) && WIFEXITED(ec1))
	{
		if(*addr == 0xbbbbbbbb)
			iprintf("...not passed! The process has written a forbidden memory location.\n\n");
		else
			iprintf("...passed!.\n\n");
	}
	else
	{
		iprintf("...not passed!\n\n");
	}
}

void mpuTest9()
{
	iprintf("Executing MPU Test 9...\n");
	ElfProgram prog(reinterpret_cast<const unsigned int*>(test9_elf),test9_elf_len);
	std::vector<pid_t> pids;
	int ec;
	for(unsigned int i = 0; i < 100; i++)
	{
		pid_t pid;
		try {
			pid = Process::create(prog);
			pids.push_back(pid);
		}
		catch (std::bad_alloc &ex)
		{
			iprintf("Bad alloc raised: %s\nIteration is: %d\n", ex.what(), i);
			break;
		}
	}
	iprintf("Allocated %d processes before system ran out of memory.\n", pids.size());
	for(unsigned int i = 0; i < pids.size(); i++)
	{
		Process::waitpid(pids[i], &ec, 0);
		//iprintf("Process %d has terminated with return code: %d\n", pids[i], ec);
	}
	iprintf("...passed!.\n\n");
}

void mpuTest10()
{
	// The first process is allocated and sent to sleep. The second process statically
	// allocates a big array and calls a syscall, which will try to write in the memory
	// chunk owned by the first process.
	// The first process should end properly, the second process should fault.
	int ec1, ec2;
	iprintf("Executing MPU Test 10...\n");
	ElfProgram prog1(reinterpret_cast<const unsigned int*>(test10_1_elf), test10_1_elf_len);
	ElfProgram prog2(reinterpret_cast<const unsigned int*>(test10_2_elf), test10_2_elf_len);
	pid_t child1=Process::create(prog1);
	pid_t child2=Process::create(prog2);
	Process::waitpid(child1, &ec1, 0);
	Process::waitpid(child2, &ec2, 0);

	if(!isSignaled(ec1) && isSignaled(ec2))
	{
		iprintf("...passed!.\n\n");
	}
	else
	{
		iprintf("...not passed!\n\n");
	}
}
#endif //WITH_PROCESSES
