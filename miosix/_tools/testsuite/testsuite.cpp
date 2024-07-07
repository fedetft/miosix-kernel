/***************************************************************************
 *   Copyright (C) 2008 - 2021 by Terraneo Federico                        *
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cassert>
#include <functional>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <cerrno>
#include <ext/atomicity.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <atomic>
#include <spawn.h>

#include "miosix.h"
#include "config/miosix_settings.h"
#include "interfaces/atomic_ops.h"
#include "interfaces/endianness.h"
#include "e20/e20.h"
#include "kernel/intrusive.h"
#include "util/crc16.h"

#if defined(_ARCH_CORTEXM7_STM32F7) || defined(_ARCH_CORTEXM7_STM32H7)
#include <kernel/scheduler/scheduler.h>
#include <core/cache_cortexMx.h>
#endif //_ARCH_CORTEXM7_STM32F7/H7

#include <ctime>
static_assert(sizeof(time_t)==8,"time_t is not 64 bit");

// Kercalls tests (shared with syscalls)
#include "test_syscalls.h"

using namespace std;
using namespace miosix;

// A reasonably small stack value for spawning threads during the test.
// Using this instead of STACK_MIN because STACK_MIN is too low for some tests
// and caused stack overflows when compiling with -O0
// Note: can be reduced down to STACK_MIN if only testing with -O2
const unsigned int STACK_SMALL=768;

#ifndef _BOARD_WANDSTEM
const unsigned int MAX_TIME_IRQ_DISABLED=14000;//us
#else
//theoretical maximum for this board is 2^16/2/48e6=680us
const unsigned int MAX_TIME_IRQ_DISABLED=500;//us
#endif

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
static void test_25();
static void test_26();
static void test_27();
#if defined(_ARCH_CORTEXM7_STM32F7) || defined(_ARCH_CORTEXM7_STM32H7)
void testCacheAndDMA();
#endif //_ARCH_CORTEXM7_STM32F7/H7
#ifdef WITH_PROCESSES
void test_syscalls_process();
#endif //WITH_PROCESSES
//Benchmark functions
static void benchmark_1();
static void benchmark_2();
static void benchmark_3();
static void benchmark_4();
//Exception thread safety test
#ifndef __NO_EXCEPTIONS
static void exception_test();
#endif //__NO_EXCEPTIONS


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
                " 'k' for kercall test (includes filesystem)\n"
                " 'p' for syscall/processes test\n"
                " 'x' for exception test\n"
                " 'b' for benchmarks\n"
                " 's' for shutdown\n");
        char c;
        for(;;)
        {
            c=getchar();
            if(c!='\n') break;
        }
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
                test_25();
                test_26();
                test_27();
                #if defined(_ARCH_CORTEXM7_STM32F7) || defined(_ARCH_CORTEXM7_STM32H7)
                testCacheAndDMA();
                #endif //_ARCH_CORTEXM7_STM32F7/H7
                
                ledOff();
                Thread::sleep(500);//Ensure all threads are deleted.
                iprintf("\n*** All tests were successful\n\n");
                //} //Testing
                break;
            case 'k':
                test_syscalls(); //Actually kercalls
                break;
            case 'p':
                #ifdef WITH_PROCESSES
                test_syscalls_process();
                #else // WITH_PROCESSES
                iprintf("Error, process support is disabled\n");
                #endif // WITH_PROCESSES
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
    write(STDOUT_FILENO,"Failed:\n",8);
    write(STDOUT_FILENO,cause,strlen(cause));
    write(STDOUT_FILENO,"\n",1);
    reboot();
}

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

static void t2_p2(void *argv)
{
    #ifdef SCHED_TYPE_EDF
    do {
        Thread::getCurrentThread()->wait();
        Thread::sleep(20);
        t2_v1=true;
    } while(!Thread::testTerminate());
    #else
    while(Thread::testTerminate()==false) t2_v1=true;
    #endif
}

static void test_2()
{
    test_name("pause/restart kernel");
    t2_p_v1=Thread::create(t2_p1,STACK_SMALL,0,NULL,Thread::JOINABLE);
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
    t2_p_v1->join();
    
    t2_p_v1=Thread::create(t2_p2,STACK_SMALL,0,NULL,Thread::JOINABLE);
    for(int i=0;i<5;i++)
    {
        bool failed=false;
        #ifdef SCHED_TYPE_EDF
        t2_p_v1->wakeup();
        #endif
        {
            PauseKernelLock pk;
            t2_v1=false;
            delayMs(40); //40ms to surely cause a missed preemption
            if(t2_v1==true) failed=true;
            t2_v1=false;
        }
        //Here we check the variable immediately after restartKernel(), so this
        //test will pass only if tickSkew works
        if(t2_v1==false) fail("tickSkew in restartKernel");
        if(failed) fail("pauseKernel");
    }
    t2_p_v1->terminate();
    #ifdef SCHED_TYPE_EDF
    t2_p_v1->wakeup();
    #endif
    t2_p_v1->join();
    pass();
}

//
// Test 3
//
/*
tests:
Thread::sleep()
Thread::nanoSleepUntil()
getTime()
also tests creation of multiple instances of the same thread
*/

static void t3_p1(void *argv)
{
    const int SLEEP_TIME=100;//ms
    for(;;)
    {
        //Test that Thread::sleep sleeps the desired time
        long long x1=getTime(); //getTime returns passed time in ns
        Thread::sleep(SLEEP_TIME);
        if(Thread::testTerminate()) break;
        long long x2=getTime();
        if(llabs((x2-x1)/1000000-SLEEP_TIME)>0) //Max tolerated error is 1ms
            fail("Thread::sleep() or getTime()");
    }
}

static volatile bool t3_v2;//Set to true by t3_p2
static volatile bool t3_deleted;//Set when an instance of t3_p2 is deleted

static void t3_p2(void *argv)
{
    const int SLEEP_TIME=15;//ms
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
    test_name("time and sleep");
    long long delta;
    {
        FastInterruptDisableLock dLock;
        auto start=IRQgetTime();
        delayUs(MAX_TIME_IRQ_DISABLED);
        delta=IRQgetTime()-start;
    }
    iprintf("%lld\n",delta);
    //10% tolerance
    auto m=MAX_TIME_IRQ_DISABLED*1000;
    if(delta<(m-m/10) || delta>(m+m/10)) fail("getTime and delayUs don't agree");
    Thread *p=Thread::create(t3_p1,STACK_SMALL,0,NULL,Thread::JOINABLE);
    for(int i=0;i<4;i++)
    {
        //The other thread is running time test
        Thread::sleep(101);
    }
    p->terminate();
    p->join();
    //Testing Thread::sleep() again
    t3_v2=false;
    p=Thread::create(t3_p2,STACK_SMALL,0,NULL,Thread::JOINABLE);
    if(!p) fail("thread creation (1)");
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
    Thread *p2=Thread::create(t3_p2,STACK_SMALL,0,NULL,Thread::JOINABLE);
    if(!p2) fail("thread creation (2)");
    //p will wake @ t=45, main will wake @ t=47 and p2 @ t=50ms
    //this will test proper sorting of sleeping_list in the kernel
    Thread::sleep(12);
    if(t3_v2==false) fail("Thread::sleep() (2)");
    //Deleting the two instances of t3_p2
    t3_deleted=false;
    p->terminate();
    p->join();
    if(Thread::exists(p)) fail("multiple instances (1)");
    if(t3_deleted==false) fail("multiple instances (2)");
    t3_deleted=false;
    p2->terminate();
    p2->join();
    if(Thread::exists(p2)) fail("multiple instances (3)");
    if(t3_deleted==false) fail("multiple instances (4)");
    //Testing Thread::nanoSleepUntil()
    long long time;
        
    const int period=10000000;//10ms
    {
        InterruptDisableLock lock; //Making these two operations atomic.
        time=IRQgetTime();
        time+=period;
    }
    for(int i=0;i<4;i++)
    {
        //time is in number of ns passed, wakeup time should not differ by > 1ms
        Thread::nanoSleepUntil(time);
        long long t2 = getTime();
        if(llabs(t2-time)/1000000>0) fail("Thread::nanoSleepUntil()");
        time+=period;
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
    const int period=30000000;//ms
    long long time=getTime();
    for(int i=0;i<10;i++)
    {
        long long prevTime=time;
        time+=period;
        Thread::setPriority(Priority(time)); //Change deadline
        Thread::nanoSleepUntil(prevTime); //Make sure the task is run periodically
        delayMs(14);
        if(getTime()>time) fail("Deadline missed (A)\n");
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
    //Check that t4_v1 is not updated
    t4_v1=false;
    delayUs(MAX_TIME_IRQ_DISABLED);
    if(t4_v1)
    {
        enableInterrupts();//
        fail("disableInterrupts");
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
    //Check that t4_v1 is not updated
    t4_v1=false;
    delayUs(MAX_TIME_IRQ_DISABLED);
    if(t4_v1)
    {
        fastEnableInterrupts();//
        fail("disableInterrupts");
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
    Thread::yield();//must return immediately
    delayMs(100);
    if(t4_v1==true) fail("setPriority (1)");
    #endif //SCHED_TYPE_CONTROL_BASED
    //Restoring original priority
    Thread::setPriority(0);
    if(Thread::getCurrentThread()->getPriority()!=0)
        fail("setPriority (2)");
    p->terminate();
    Thread::sleep(10);
    #ifdef SCHED_TYPE_EDF
    Thread::create(t4_p2,STACK_SMALL);
    const int period=50000000;//ms
    long long time=getTime();
    //This takes .024/.05=48% of CPU time
    for(int i=0;i<10;i++)
    {
        long long prevTime=time;
        time+=period;
        Thread::setPriority(Priority(time)); //Change deadline
        Thread::nanoSleepUntil(prevTime); //Make sure the task is run periodically
        delayMs(24);
        if(getTime()>time) fail("Deadline missed (B)\n");
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
#ifndef SCHED_TYPE_CONTROL_BASED
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
#endif
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
    #ifndef SCHED_TYPE_CONTROL_BASED
    //Create first thread
    if (!Thread::create(t6_p1,STACK_SMALL,priorityAdapter(0),NULL))
        fail("thread creation (1)");
    Thread::sleep(20);
    //Create second thread
    if (!Thread::create(t6_p2,STACK_SMALL,priorityAdapter(1),NULL))
        fail("thread creation (2)");
    Thread::sleep(20);
    //Create third thread
    if (!Thread::create(t6_p3,STACK_SMALL,priorityAdapter(2),NULL))
        fail("thread creation (3)");
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
    #endif
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
// Test 7 removed, Timer class deprecated
//

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
Queue::IRQputBlocking()
Queue::IRQget()
Queue::IRQgetBlocking()
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
    //Test IRQputBlocking
    t8_q1.reset();
    t8_q2.reset();
    write='A';
    read='A';
    {
        FastInterruptDisableLock dLock;
        for(j=0;j<8;j++)
        {
            t8_q1.IRQputBlocking(write,dLock);
            write++;//Advance to next char, to check order
        }
    }
    for(j=0;j<8;j++)
    {
        char c;
        t8_q2.get(c);
        if(c!=read) fail("IRQputBlocking");
        read++;
    }
    //Test IRQgetBlocking
    t8_q1.reset();
    t8_q2.reset();
    write='A';
    read='A';
    for(j=0;j<8;j++)
    {
        FastInterruptDisableLock dLock;
        t8_q1.IRQput(write);
        write++;
        if(t8_q2.isEmpty()==false) fail("Unexpected");
        char c='\0';
        t8_q2.IRQgetBlocking(c,dLock);
        if(c!=read) fail("IRQgetBlocking");
        read++;
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

static volatile bool t9_v1;

void t9_p1(void*)
{
    for(;;)
    {
        if(Thread::testTerminate()) break;
        t9_v1=true;
        #ifdef SCHED_TYPE_EDF
        Thread::sleep(1);
        #endif //SCHED_TYPE_EDF
    }
}

static void test_9()
{
    test_name("isKernelRunning and save/restore interrupts");
    //Testing kernel_running with nested pause_kernel()
    Thread *p=Thread::create(t9_p1,STACK_SMALL,0,NULL,Thread::JOINABLE);
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
    if(areInterruptsEnabled()==false) fail("areInterruptsEnabled() (1)");
    disableInterrupts();//Now interrupts should be disabled
    t9_v1=false;
    delayUs(MAX_TIME_IRQ_DISABLED/3);
    if(t9_v1)
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
    delayUs(MAX_TIME_IRQ_DISABLED/3);
    if(t9_v1)
    {
        enableInterrupts();
        enableInterrupts();
        fail("disableInterrups() nesting (2)");
    }
    if(areInterruptsEnabled()==true)
    {
        enableInterrupts();
        enableInterrupts();
        fail("areInterruptsEnabled() (3)");
    }
    enableInterrupts();//Now interrupts should remain disabled
    delayUs(MAX_TIME_IRQ_DISABLED/3);
    if(t9_v1)
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
    delayMs(10);
    if(t9_v1==false)
    {
        fail("enableInterrupts() nesting (2)");
    }
    if(areInterruptsEnabled()==false) fail("areInterruptsEnabled() (5)");
    p->terminate();
    p->join();
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
    Thread *t=Thread::create(t10_p1,1024+512,0,NULL);
    //This sleep needs to be long enough to let the other thread print the
    //"An exception propagated ..." string on the serial port, otherwise the
    //test fails because the other thread still exists because it is printing.
    Thread::sleep(100);
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
    if(!t) fail("thread creation (1)");
    t->terminate();
    Thread::sleep(10);
    if(Thread::exists(t)) fail("detached thread");
    //Test 2: joining a not deleted thread
    t=Thread::create(t14_p2,STACK_SMALL,0,0,Thread::JOINABLE);
    if(!t) fail("thread creation (2)");
    t->join();
    Thread::sleep(10);
    //Test 3: detaching a thread while not deleted
    t=Thread::create(t14_p2,STACK_SMALL,0,0,Thread::JOINABLE);
    if(!t) fail("thread creation (3)");
    Thread::yield();
    t->detach();
    Thread::sleep(10);
    //Test 4: detaching a deleted thread
    t=Thread::create(t14_p1,STACK_SMALL,0,0,Thread::JOINABLE);
    if(!t) fail("thread creation (4)");
    t->terminate();
    Thread::sleep(10);
    t->detach();
    Thread::sleep(10);
    //Test 5: detaching a thread on which some other thread called join
    t=Thread::create(t14_p2,STACK_SMALL,0,0,Thread::JOINABLE);
    if(!t) fail("thread creation (5)");
    if(!Thread::create(t14_p3,STACK_SMALL,0,reinterpret_cast<void*>(t)))
        fail("thread creation (6)");
    if(t->join()==true) fail("thread not detached (1)");

    //
    // Consistency tests, these make sure that joinable threads are correctly
    // implemented.
    //

    //Test 1: join on joinable, not already deleted
    void *result=0;
    t=Thread::create(t14_p2,STACK_SMALL,0,(void*)0xdeadbeef,Thread::JOINABLE);
    if(!t) fail("thread creation (7)");
    Thread::yield();
    if(t->join(&result)==false) fail("Thread::join (1)");
    if(Thread::exists(t)) fail("Therad::exists (1)");
    if(reinterpret_cast<unsigned>(result)!=0xdeadbeef) fail("join result (1)");
    Thread::sleep(10);

    //Test 2: join on joinable, but detach called before
    t=Thread::create(t14_p2,STACK_SMALL,0,0,Thread::JOINABLE);
    if(!t) fail("thread creation (8)");
    Thread::yield();
    t->detach();
    if(t->join()==true) fail("Thread::join (2)");
    Thread::sleep(60);

    //Test 3: join on joinable, but detach called after
    t=Thread::create(t14_p1,STACK_SMALL,0,0,Thread::JOINABLE);
    if(!t) fail("thread creation (9)");
    Thread::create(t14_p3,STACK_SMALL,0,reinterpret_cast<void*>(t));
    if(t->join()==true) fail("Thread::join (3)");
    t->terminate();
    Thread::sleep(10);

    //Test 4: join on detached, not already deleted
    t=Thread::create(t14_p1,STACK_SMALL,0,0);
    if(!t) fail("thread creation (9)");
    if(t->join()==true) fail("Thread::join (4)");
    t->terminate();
    Thread::sleep(10);

    //Test 5: join on self
    if(Thread::getCurrentThread()->join()==true)
        fail("Thread:: join (5)");

    //Test 6: join on already deleted
    result=0;
    t=Thread::create(t14_p1,STACK_SMALL,0,(void*)0xdeadbeef,Thread::JOINABLE);
    if(!t) fail("thread creation (10)");
    t->terminate();
    Thread::sleep(10);
    if(Thread::exists(t)==false) fail("Therad::exists (2)");
    if(t->join(&result)==false) fail("Thread::join (6)");
    if(Thread::exists(t)) fail("Therad::exists (3)");
    if(reinterpret_cast<unsigned>(result)!=0xdeadbeef) fail("join result (2)");
    Thread::sleep(10);

    //Test 7: join on already detached and deleted
    t=Thread::create(t14_p1,STACK_SMALL,0,0,Thread::JOINABLE);
    if(!t) fail("thread creation (11)");
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

void t15_p3(void *argv)
{
    Thread::sleep(30);
    t15_c1.signal();
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

    //testing timedWait
    long long timeout1=200000;
    long long timeout2=500000;
    #ifdef WITH_RTC_AS_OS_TIMER
    timeout1+=200000;
    timeout2+=250000;
    #endif
    long long b,a=getTime()+10000000; //10ms
    {
        Lock<Mutex> l(t15_m1);
        if(t15_c1.timedWait(l,a)!=TimedWaitResult::Timeout) fail("timedwait (1)");
        b=getTime();
    }
    //iprintf("delta=%lld\n",b-a);
    if(llabs(b-a)>timeout1) fail("timedwait (2)");
    Thread::create(t15_p3,STACK_SMALL,0);
    a=getTime()+100000000; //100ms
    {
        Lock<Mutex> l(t15_m1);
        if(t15_c1.timedWait(l,a)!=TimedWaitResult::NoTimeout) fail("timedwait (1)");
        b=getTime();
    }
    //iprintf("delta=%lld\n",b-a+70000000);
    if(llabs(b-a+70000000)>timeout2) fail("timedwait (4)");
    Thread::sleep(10);
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

static constexpr int nsPerSec = 1000000000;

inline long long timespec2ll(const struct timespec *tp)
{
    return static_cast<long long>(tp->tv_sec) * nsPerSec + tp->tv_nsec;
}

void timespecAdd(struct timespec *t, long long ns)
{
    long long x=timespec2ll(t)+ns;
    t->tv_sec = x / nsPerSec;
    t->tv_nsec = static_cast<long>(x % nsPerSec);
}

long long timespecDelta(struct timespec *a, struct timespec *b)
{
    return timespec2ll(a)-timespec2ll(b);
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

int t16_v2=0;

void t16_f1()
{
    t16_v2++;
}

void t16_f2()
{
    Thread::sleep(200);
    t16_v2++;
}

pthread_once_t t16_o1=PTHREAD_ONCE_INIT;
pthread_once_t t16_o2=PTHREAD_ONCE_INIT;

void t16_p5(void*)
{
    if(pthread_once(&t16_o2,t16_f2)!=0) fail("pthread_once 6");
}

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
    //testing pthread_cond_timedwait
    long long timeout1=200000;
    long long timeout2=500000;
    #ifdef WITH_RTC_AS_OS_TIMER
    timeout1+=200000;
    timeout2+=250000;
    #endif
    timespec a,b;
    clock_gettime(CLOCK_MONOTONIC,&a);
    timespecAdd(&a,10000000); //10ms
    if(pthread_mutex_lock(&t16_m1)!=0) fail("mutex lock (7)"); //<---
    if(pthread_cond_timedwait(&t16_c1,&t16_m1,&a)!=ETIMEDOUT) fail("timedwait (1)");
    clock_gettime(CLOCK_MONOTONIC,&b);
    if(pthread_mutex_unlock(&t16_m1)!=0) fail("mutex unlock (7)"); //<---
    //iprintf("delta=%lld\n",timespecDelta(&b,&a));
    if(llabs(timespecDelta(&b,&a))>timeout1) fail("timedwait (2)");
    if(pthread_create(&thread,NULL,t16_p3,NULL)!=0) fail("pthread_create (10)");
    clock_gettime(CLOCK_MONOTONIC,&a);
    timespecAdd(&a,100000000); //100ms
    if(pthread_mutex_lock(&t16_m1)!=0) fail("mutex lock (8)"); //<---
    t16_v1=false;
    if(pthread_cond_timedwait(&t16_c1,&t16_m1,&a)!=0) fail("timedwait (3)");
    clock_gettime(CLOCK_MONOTONIC,&b);
    if(t16_v1==false) fail("did not really wait");
    if(pthread_mutex_unlock(&t16_m1)!=0) fail("mutex unlock (8)"); //<---
    //iprintf("delta=%lld\n",timespecDelta(&b,&a)+70000000);
    if(llabs(timespecDelta(&b,&a)+70000000)>timeout2) fail("timedwait (4)");
    pthread_join(thread,NULL);
    Thread::sleep(10);
    //
    // Testing pthread_once
    //
    //Note: implementation detail since otherwise by the very nature of
    //pthread_once, it wouldn't be possible to run the test more than once ;)
    if(t16_o1.init_executed) t16_o1.init_executed=0;
    if(t16_o2.init_executed) t16_o2.init_executed=0;
    t16_v2=0;
    if(pthread_once(&t16_o1,t16_f1)!=0) fail("pthread_once 1");
    if(t16_v2!=1) fail("pthread_once 2");
    if(pthread_once(&t16_o1,t16_f1)!=0) fail("pthread_once 2");
    if(t16_v2!=1) fail("pthread_once 3");
    if(sizeof(pthread_once_t)!=2) fail("pthread_once 4");
    t16_v2=0;
    Thread::create(t16_p5,STACK_MIN);
    Thread::sleep(50);
    if(pthread_once(&t16_o2,t16_f2)!=0) fail("pthread_once 5");
    if(t16_v2!=1) fail("pthread_once does not wait");
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
    unsigned long long t1=getTime();
    eq->post(bind(t20_f2,10,4)); //This should block
    unsigned long long t2=getTime();
    //The other thread sleep for 50ms before calling run()
    if((t2-t1)/1000000 < 40)
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
atomic<T>
__atomic_add()
__exchange_and_add()
These are not actually in the kernel but in the patches to gcc
also tests atomic operations provided by miosix (interfaces/atomic_ops.h)
*/

const int t22_iterations=100000;
static int t22_v1;
static int t22_v2;
static int t22_v3;
static int t22_v4;
atomic<int> t22_v6;

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
        #ifndef SCHED_TYPE_EDF
        Thread::yield();
        #else //SCHED_TYPE_EDF
        Thread::sleep(1);
        #endif //SCHED_TYPE_EDF
    }
}

static void *t22_t1(void*)
{
    for(int i=0;i<t22_iterations;i++)
    {
        __gnu_cxx::__atomic_add(&t22_v1,1);
        __gnu_cxx::__exchange_and_add(&t22_v2,-1);
        atomicAdd(&t22_v3,1);
        atomicAddExchange(&t22_v4,1);
        t22_v6++;
        #ifdef SCHED_TYPE_EDF
        if((i % (t22_iterations/10))==0) Thread::sleep(1);
        #endif //SCHED_TYPE_EDF
    }
    return 0;
}

int t22_v8;

class t22_c1
{
public:
    bool canDelete=false;
    ~t22_c1()
    {
        if(canDelete==false) fail("Deleted too soon");
        atomicAdd(&t22_v8,1);
    }
};

shared_ptr<t22_c1> t22_v7;

void t22_t3(void*)
{
    auto inst1=make_shared<t22_c1>();
    for(int i=0;i<t22_iterations;i++)
    {
        atomic_store(&t22_v7,inst1);
        #ifdef SCHED_TYPE_EDF
        if((i % (t22_iterations/10))==0) Thread::sleep(1);
        #endif //SCHED_TYPE_EDF
    }
    atomic_store(&t22_v7,shared_ptr<t22_c1>(nullptr));
    inst1->canDelete=true;
}

static void test_22()
{
    test_name("Atomic ops");
    
    //Check thread safety for atomic ops (both GCC provided and the miosix ones)
    t22_v1=t22_v2=t22_v3=t22_v4=0;
    pthread_t t;
    pthread_create(&t,0,t22_t1,0);
    for(int i=0;i<t22_iterations;i++)
    {
        __gnu_cxx::__atomic_add(&t22_v1,-1);
        __gnu_cxx::__exchange_and_add(&t22_v2,1);
        atomicAdd(&t22_v3,-1);
        atomicAddExchange(&t22_v4,-1);
        t22_v6--;
        #ifdef SCHED_TYPE_EDF
        if((i % (t22_iterations/10))==0) Thread::sleep(1);
        #endif //SCHED_TYPE_EDF
    }
    pthread_join(t,0);
    if(t22_v1!=0 || t22_v2!=0 || t22_v3!=0 || t22_v4!=0)
        fail("not thread safe");
    if(t22_v6!=0) fail("C++11 atomics not thread safe");
    
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
        #if __CORTEX_M != 0x00
        FastInterruptDisableLock dLock;
        #else
        //Cortex M0 does not have atomic ops, they are emulated by disabling IRQ
        InterruptDisableLock dLock;
        #endif
        t22_v5=false;

        int x=10;
        if(atomicSwap(&x,20)!=10) error=true;
        if(x!=20) error=true;

        x=10;
        atomicAdd(&x,-5);
        if(x!=5) error=true;

        x=10;
        if(atomicAddExchange(&x,5)!=10) error=true;
        if(x!=15) error=true;

        x=10;
        if(atomicCompareAndSwap(&x,11,12)!=10) error=true;
        if(x!=10) error=true;
        if(atomicCompareAndSwap(&x,10,13)!=10) error=true;
        if(x!=13) error=true;

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

        delayUs(MAX_TIME_IRQ_DISABLED); //Wait to check interrupts are disabled
        if(t22_v5) error=true;
    }
    if(error) fail("Interrupt test not passed");

    t2->terminate();
    t2->join();
    
    t22_v8=0;
    {
        Thread *t3=Thread::create(t22_t3,STACK_SMALL,0,0,Thread::JOINABLE);
        auto inst2=make_shared<t22_c1>();
        for(int i=0;i<t22_iterations;i++)
        {
            atomic_store(&t22_v7,inst2);
            #ifdef SCHED_TYPE_EDF
            if((i % (t22_iterations/10))==0) Thread::sleep(1);
            #endif //SCHED_TYPE_EDF
        }
        atomic_store(&t22_v7,shared_ptr<t22_c1>(nullptr));
        // NOTE: we can't check use_count to be 1 because the C++ specs say it
        // can provide inaccurate results, and it does. Apparently atomic_store
        // as of GCC 9.2.0 is implemented as a mutex lock and a swap(), see
        // include/bits/shred_ptr_atomic.h
        // Now, if the thread is interrupted after the swap but before returning
        // from atomic_store a copy of the pointer remains for a short while on
        // its stack and use_count() returns 2.
        inst2->canDelete=true;
        t3->join();
        if(atomic_load(&t22_v7)!=nullptr) fail("shared_ptr atomic_load");
    }
    if(t22_v8!=2)
    {
        iprintf("deleted %d\n",t22_v8);
        fail("Not deleted");
    }
    
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

class Middle1 : public Base1, public IntrusiveRefCounted<Middle1>
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

class Base0 : public IntrusiveRefCounted<Base0>
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
        assert(!ptr1);
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
            assert(!ptr2);
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
        assert(!ptr1);
    }
    
    // Reset, on a shared pointer
    dtorCalled=false;
    {
        intrusive_ref_ptr<Base0> ptr1(new Base0);
        {
            intrusive_ref_ptr<Base0> ptr2(ptr1);
            ptr1.reset();
            assert(!ptr1);
            assert(ptr2);
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

//
// Test 25
//
/*
tests:
C++ threads API
 thread::thread
 thread::get_id
 thread::native_handle
 thread::detach
 thread::joinable
 thread::join
 async
 future
 mutex
 lock
 recursive_mutex
 lock_guard
 unique_lock
 condition_variable
 call_once
 yield
 this_thread::sleep_for
 this_thread::sleep_until
*/

Thread *t25_v1=nullptr;

int t25_p1(int a, int b)
{
    if(a!=2 || b!=5) fail("thread::thread params");
    t25_v1=Thread::getCurrentThread();
    t25_v1->wait();
    return a+b;
}

volatile bool t25_v2;
mutex t25_m1;

void t25_p2()
{
    lock_guard<mutex> l(t25_m1);
    t25_v2=true;
    Thread::sleep(50);
}

condition_variable t25_c1;

void t25_p3(int i)
{
    Thread::sleep(30);
    if(t25_m1.try_lock()==false) fail("cond_wait did not release mutex");
    t25_v2=true;
    if(i==0) t25_c1.notify_one(); else t25_c1.notify_all();
    t25_m1.unlock();
}

static void test_25()
{
    /*
     * C++11 threads run with MAIN_PRIORITY, so to avoid deadlocks or other
     * artifacts, we restore main't priority to the default, and set it back
     * to 0 at the end of this test, as the rest of the testsuite runs with
     * lowest priority
     */
    Thread::setPriority(MAIN_PRIORITY);
    test_name("C++11 threads");
    //
    // Test thread::thread, native_handle, get_id, detach, joinable
    //
    {
        thread thr(t25_p1,2,5);
        Thread *t=reinterpret_cast<Thread*>(thr.native_handle());
        if(Thread::exists(t)==false) fail("thread::thread");
        if(t->isDetached()==true) fail("initial detachstate");
        if(thr.joinable()==false) fail("thread::joinable");
        thr.detach();
        if(t->isDetached()==false) fail("thread::detach");
        if(thr.joinable()==true) fail("thread::joinable");
        thread::id self=this_thread::get_id();
        if(self==thr.get_id()) fail("thread_id::operator== (1)");
        if(self!=this_thread::get_id()) fail("thread_id::operator== (2)");
        
        Thread::sleep(10); //Give the other thread time to call wait()
        if(t!=t25_v1) fail("thread::native_handle");
        t->wakeup();
        Thread::sleep(10);
        if(Thread::exists(t)) fail("unexpected");
    }
    //
    // Testing join
    //
    {
        thread thr(t25_p1,2,5);
        Thread *t=reinterpret_cast<Thread*>(thr.native_handle());
        if(Thread::exists(t)==false) fail("thread::thread");
        Thread::sleep(10); //Give the other thread time to call wait()
        t->wakeup();
        thr.join();
        if(Thread::exists(t)==true) fail("thread::join");
    }
    //
    // Testing async
    //
    {
        future<int> fut=async(launch::async,t25_p1,2,5);
        Thread::sleep(10); //Give the other thread time to call wait()
        t25_v1->wakeup();
        if(fut.get()!=7) fail("future");
    }
    //
    // Testing mutex
    //
    {
        t25_m1.lock();
        t25_v2=false;
        thread thr(t25_p2);
        Thread::sleep(10);
        if(t25_v2==true) fail("mutex fail");
        t25_m1.unlock();
        Thread::sleep(10);
        if(t25_v2==false) fail("mutex fail (2)");
        thr.join();
    }
    {
        thread thr(t25_p2);
        Thread::sleep(10);
        //After the thread is created it will lock the mutex for 50ms
        if(t25_m1.try_lock()==true) fail("trylock");
        thr.join();
        if(t25_m1.try_lock()==false) fail("trylock (2)");
        t25_m1.unlock();
    }
    //
    // Testing recursive_mutex
    //
    auto isLocked=[](recursive_mutex& m)->bool {
        return async(launch::async,[&]()->bool {
            if(m.try_lock()==true)
            {
                m.unlock();
                return false;
            } else return true;
        }).get();
    };
    {
        recursive_mutex rm;
        rm.lock();
        if(isLocked(rm)==false) fail("recursive_mutex::lock");
        rm.lock();
        if(isLocked(rm)==false) fail("recursive_mutex::lock");
        rm.unlock();
        if(isLocked(rm)==false) fail("recursive_mutex::unlock");
        rm.unlock();
        if(isLocked(rm)==true) fail("recursive_mutex::unlock");
    }
    //
    // Testing lock
    //
    {
        recursive_mutex rm1,rm2;
        lock(rm1,rm2);
        if(!isLocked(rm1) || !isLocked(rm2)) fail("lock");
        rm1.unlock();
        rm2.unlock();
    }
    //
    // Testing condition_variable
    //
    for(int i=0;i<2;i++)
    {
        thread thr(t25_p3,i);
        {
            unique_lock<mutex> l(t25_m1);
            t25_v2=false;
            t25_c1.wait(l);
            if(t25_v2==false) fail("did not really wait");
        }
        thr.join();
    }
    //
    // Testing call_once
    //
    {
        once_flag flag;
        int a=0;
        thread thr([&]{ call_once(flag,[&]{ Thread::sleep(100); a++; }); });
        call_once(flag,[&]{ a++; });
        thr.join();
        call_once(flag,[&]{ a++; });
        if(a!=1) fail("call_once");
    }
    //
    // Testing yield
    //
    #ifndef SCHED_TYPE_EDF
    {
        volatile bool enable=false;
        volatile bool flag=false;
        thread thr([&]{ while(enable==false) /*wait*/; flag=true; });
        Thread::sleep(10); //Get the other thread ready
        enable=true;
        this_thread::yield();
        if(flag==false) fail("this_thread::yield");
        thr.join();
    }
    #endif //SCHED_TYPE_EDF
    //
    // Testing system_clock/this_thread::sleep_for
    //
    {
        auto a=chrono::system_clock::now().time_since_epoch().count();
        this_thread::sleep_for(chrono::milliseconds(100));
        auto b=chrono::system_clock::now().time_since_epoch().count();
        if(llabs(b-a-100000000)>5000000) fail("sleep_for");
    }
    //
    // Testing steady_clock/this_thread::sleep_until
    //
    {
        auto a=chrono::steady_clock::now().time_since_epoch().count();
        this_thread::sleep_until(chrono::steady_clock::now()+chrono::milliseconds(100));
        auto b=chrono::steady_clock::now().time_since_epoch().count();
        if(llabs(b-a-100000000)>5000000) fail("sleep_until");
    }
    //
    // Testing condition_variable timed wait
    //
    #ifndef __CODE_IN_XRAM
    long long timeout1=250000;
    long long timeout2=500000;
    #else //__CODE_IN_XRAM
    long long timeout1=350000;
    long long timeout2=750000;
    #endif //__CODE_IN_XRAM
    #ifdef WITH_RTC_AS_OS_TIMER
    timeout1+=200000;
    timeout2+=250000;
    #endif
    {
        unique_lock<mutex> l(t25_m1);
        auto a=chrono::steady_clock::now().time_since_epoch().count();
        if(t25_c1.wait_for(l,10ms)!=cv_status::timeout) fail("timedwait (1)");
        auto b=chrono::steady_clock::now().time_since_epoch().count();
        //iprintf("delta=%lld\n",b-a-10000000);
        if(llabs(b-a-10000000)>timeout1) fail("timedwait (2)");
    }
    {
        unique_lock<mutex> l(t25_m1);
        auto start=chrono::steady_clock::now();
        auto a=start.time_since_epoch().count();
        if(t25_c1.wait_until(l,start+10ms)!=cv_status::timeout) fail("timedwait (3)");
        auto b=chrono::steady_clock::now().time_since_epoch().count();
        //iprintf("delta=%lld\n",b-a-10000000);
        if(llabs(b-a-10000000)>timeout1) fail("timedwait (4)");
    }
    {
        thread t([]{ this_thread::sleep_for(30ms); t25_c1.notify_one(); });
        auto a=chrono::steady_clock::now().time_since_epoch().count();
        unique_lock<mutex> l(t25_m1);
        if(t25_c1.wait_for(l,100ms)!=cv_status::no_timeout) fail("timedwait (5)");
        auto b=chrono::steady_clock::now().time_since_epoch().count();
        //iprintf("delta=%lld\n",b-a-30000000);
        if(llabs(b-a-30000000)>timeout2) fail("timedwait (6)");
        t.join();
    }
    pass();
    Thread::setPriority(0);
}

//
// Test 26
//
/*
tests:
C reentrancy data
*/

void *t26_t1(void*)
{
    for(int i=0;i<10;i++)
    {
        if(errno!=0) fail("errno");
        Thread::sleep(1);
    }
    return nullptr;
}

static void test_26()
{
    test_name("C reentrancy data");
    pthread_t t;
    pthread_create(&t,0,t26_t1,0);
    for(int i=0;i<10;i++)
    {
        errno=-i-1;
        Thread::sleep(1);
        if(errno!=-i-1) fail("errno");
    }
    pthread_join(t,0);
    pass();
}

//
// Test 27
//
/*
tests:
Semaphore class
*/

struct t27_data
{
    t27_data(): consumer(5), producer(5) {}
    Semaphore consumer;
    Semaphore producer;
};

void *t27_t1(void *xdata)
{
    t27_data *data=reinterpret_cast<t27_data *>(xdata);
    for(int i=0; i<4; i++) data->consumer.wait();
    if(data->consumer.getCount() != 1)
        fail("wait to 1 with counter >= 0 not working");
    bool res = data->consumer.tryWait();
    if(data->consumer.getCount() != 0 || res != true)
        fail("tryWait to 0 with counter = 1 not working");
    res = data->consumer.tryWait();
    if(data->consumer.getCount() != 0 || res != false)
        fail("tryWait with counter = 0 not working");
    for(int i=0; i<10; i++)
    {
        data->producer.signal();
        data->consumer.wait();
    }
    return nullptr;
}

void *t27_t2(void *xdata)
{
    t27_data *data=reinterpret_cast<t27_data *>(xdata);
    for(int i=0; i<2; i++)
    {
        Thread::sleep(100);
        data->producer.signal();
        data->consumer.wait();
    }
    return nullptr;
}

void *t27_t3(void *xdata)
{
    t27_data *data=reinterpret_cast<t27_data *>(xdata);
    data->producer.signal();
    data->consumer.wait();
    return nullptr;
}

static void test_27()
{
    test_name("Semaphores");
    t27_data data;

    Thread *thd=Thread::create(t27_t1,STACK_SMALL,MAIN_PRIORITY,reinterpret_cast<void*>(&data),Thread::JOINABLE);
    for(int i=0; i<5; i++) data.producer.wait();
    if(data.producer.getCount() != 0)
        fail("wait to zero with counter >= 0 not working");
    for(int i=0; i<10; i++)
    {
        data.producer.wait();
        data.consumer.signal();
    }
    thd->join();
    if(data.producer.getCount() != 0 && data.consumer.getCount() != 0)
        fail("wait with counter == 0 not working");
    
    Thread *thd2=Thread::create(t27_t2,STACK_SMALL,MAIN_PRIORITY,reinterpret_cast<void*>(&data),Thread::JOINABLE);
    auto res = data.producer.timedWait(getTime()+200*1000000LL);
    if(res == TimedWaitResult::Timeout)
        fail("timedWait did not return the first time without timeout, timebase incorrect?");
    data.consumer.signal();
    res = data.producer.timedWait(getTime()+8*1000000LL);
    int j = 0;
    while(res == TimedWaitResult::Timeout)
    {
        j++;
        res = data.producer.timedWait(getTime()+8*1000000LL);
    }
    if(j < 12 || j > 13)
    {
        iprintf("j=%d, should be 12 or 13\n", j);
        fail("timedWait timed out too many/few times, timebase incorrect?");
    }
    data.consumer.signal();
    thd2->join();
    if(data.producer.getCount() != 0 && data.consumer.getCount() != 0)
        fail("timedWait with counter == 0 not working");
    
    //Testing multiple threads enqueued on the same semaphore
    #ifndef SCHED_TYPE_EDF
    Thread *thds[3];
    for(int i=0; i<3; i++)
    {
        //Priority is > than MAIN_PRIORITY to ensure all the threads get stuck
        //at the `data->consumer.wait();` line and not earlier.
        //This test is disabled on EDF scheduler to avoid having to fudge with
        //deadlines to make this happen.
        thds[i] = Thread::create(t27_t3,STACK_SMALL,MAIN_PRIORITY+1,reinterpret_cast<void*>(&data),Thread::JOINABLE);
        data.producer.wait();
    }
    for(int i=0; i<3; i++) data.consumer.signal();
    for(int i=0; i<3; i++) thds[i]->join(); //This hangs if the test fails
    #endif
    pass();
}

#if defined(_ARCH_CORTEXM7_STM32F7) || defined(_ARCH_CORTEXM7_STM32H7)
static Thread *waiting=nullptr; /// Thread waiting on DMA completion IRQ

/**
 * DMA completion IRQ
 */
void __attribute__((naked)) DMA2_Stream0_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z9dma2s0irqv");
    restoreContext();
}

/**
 * DMA completion IRQ actual implementation
 */
void dma2s0irq()
{
    DMA2->LIFCR=0b111101;
    if(waiting) waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    waiting=nullptr;
}

/**
 * Copy memory to memory using DMA, used to test cache and DMA consistency
 */
void dmaMemcpy(void *dest, const void *source, int size,
               void *slackBeforeDest, void *slackBeforeSource, int slackBeforeSize,
               void *slackAfterDest, void *slackAfterSource, int slackAfterSize
)
{
    FastInterruptDisableLock dLock;
    DMA2_Stream0->NDTR=size;
    DMA2_Stream0->PAR=reinterpret_cast<unsigned int>(source);
    DMA2_Stream0->M0AR=reinterpret_cast<unsigned int>(dest);
    DMA2_Stream0->CR=0             //Select channel 0
                | DMA_SxCR_MINC    //Increment RAM pointer
                | DMA_SxCR_PINC    //Increment RAM pointer
                | DMA_SxCR_DIR_1   //Memory to memory
                | DMA_SxCR_TCIE    //Interrupt on transfer complete
                | DMA_SxCR_TEIE    //Interrupt on transfer error
                | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                | DMA_SxCR_EN;     //Start the DMA
    //Write to the same cache lines the DMA is using to try creating a stale
    if(slackBeforeSize) memcpy(slackBeforeDest,slackBeforeSource,slackBeforeSize);
    if(slackAfterSize) memcpy(slackAfterDest,slackAfterSource,slackAfterSize);
    waiting=Thread::IRQgetCurrentThread();
    do {
        FastInterruptEnableLock eLock(dLock);
        Thread::yield();
    } while(waiting);
}

static const unsigned int cacheLine=32; //Cortex-M7 cache line size
static const unsigned int bufferSize=4096;
static char __attribute__((aligned(32))) src[bufferSize];
static char __attribute__((aligned(32))) dst[bufferSize];

void testOneDmaTransaction(unsigned int size, unsigned int offset)
{
    //Pointer to buffer where the DMA will write, with the desired offset from
    //perfect cache line alignment
    char *source=src+offset;
    char *dest=dst+offset;
    
    //If the DMA memory buffer beginning is misaligned, get pointer and size to
    //the cache line that includes the buffer beginning
    char *slackBeforeSource=reinterpret_cast<char*>(
        reinterpret_cast<unsigned int>(source) & (~(cacheLine-1)));
    char *slackBeforeDest=reinterpret_cast<char*>(
        reinterpret_cast<unsigned int>(dest) & (~(cacheLine-1)));
    unsigned int slackBeforeSize=source-slackBeforeSource;
    
    //If the DMA memory buffer end is misaligned, get pointer and size to
    //the cache line that includes the buffer end
    char *slackAfterSource=source+size;
    char *slackAfterDest=dest+size;
    unsigned int slackAfterSize=cacheLine -
        (reinterpret_cast<unsigned int>(slackAfterSource) & (cacheLine-1));
    if(slackAfterSize==cacheLine) slackAfterSize=0;
    
//     fprintf(stderr,"%d %d %d\n",size,slackBeforeSize,slackAfterSize);
    assert(((size+slackBeforeSize+slackAfterSize) % cacheLine)==0);
    
    //Initialize the DMA buffer source, dest, including the before/after region
    for(unsigned int i=0;i<size+slackBeforeSize+slackAfterSize;i++)
    {
        slackBeforeSource[i]=rand();
        slackBeforeDest[i]=0;
    }
    markBufferBeforeDmaWrite(source,size);
    dmaMemcpy(dest,source,size,
              slackBeforeDest,slackBeforeSource,slackBeforeSize,
              slackAfterDest,slackAfterSource,slackAfterSize);
    markBufferAfterDmaRead(dest,size);
    bool error=false;
    for(unsigned int i=0;i<size+slackBeforeSize+slackAfterSize;i++)
    {
        if(slackBeforeSource[i]==slackBeforeDest[i]) continue;
        error=true;
        break;
    }
    if(error)
    {
        puts("Source memory region");
        memDump(slackBeforeSource,size+slackBeforeSize+slackAfterSize);
        puts("Dest memory region");
        memDump(slackBeforeDest,size+slackBeforeSize+slackAfterSize);
        iprintf("testOneDmaTransaction(size=%d,offset=%d) failed\n",size,offset);
        fail("cache not in sync with DMA");
    }
}

void testCacheAndDMA()
{
    test_name("STM32 cache/DMA");
    {
        FastInterruptDisableLock dLock;
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
        RCC_SYNC();
        NVIC_SetPriority(DMA2_Stream0_IRQn,15);//Lowest priority for serial
        NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    }

    //Testing cache-aligned transactions
    for(unsigned int size=cacheLine;size<=bufferSize;size+=cacheLine)
    {
        testOneDmaTransaction(size,0);
        testOneDmaTransaction(size,0);
    }
    
    //Testing misalignment by offset
    for(unsigned int size=cacheLine;size<=bufferSize-cacheLine;size+=cacheLine)
    {
        testOneDmaTransaction(size,1);
        testOneDmaTransaction(size,1);
        testOneDmaTransaction(size,cacheLine-1);
        testOneDmaTransaction(size,cacheLine-1);
    }
    
    //Testing misalignment by offset and size
    for(unsigned int size=cacheLine/2;size<=bufferSize-2*cacheLine;size+=cacheLine)
    {
        testOneDmaTransaction(size,1);
        testOneDmaTransaction(size,1);
        testOneDmaTransaction(size,cacheLine-1);
        testOneDmaTransaction(size,cacheLine-1);
    }
    
    //Testing misalignment small transfers.
    //This test is meant to trigger bugs where the write buffer is not flushed
    //as it should, which occurred in the serial port driver before
    //markBufferBeforeDmaWrite() had a __DSB().
    //Unfortunately, I could not reproduce the write buffer bug with this
    //testcase, so this corner case remains not covered by this testsuite
    //This test is anyway left just in case it catches something else
    for(unsigned int size=1;size<cacheLine;size+=1)
    {
        for(unsigned int offset=1;offset<cacheLine;offset+=1)
        {
            testOneDmaTransaction(size,offset);
            testOneDmaTransaction(size,offset);
        }
    }
    pass();
}
#endif //_ARCH_CORTEXM7_STM32F7/H7

//
// Kercalls test (in a separate file, shared with syscalls)
//

#include "test_syscalls.cpp"

//
// Syscall test (executed in a separate process)
//

void test_syscalls_process()
{
    ledOn();
    const char *arg[] = { "/bin/test_process", nullptr };
    int exitcode=spawnAndWait(arg);
    if(exitcode!=0) fail("test process has exited with a non-zero exit code");
    ledOff();
}

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
    using namespace std::chrono;
    auto t=system_clock::now();
    extern unsigned long _data asm("_data");
    char *data=reinterpret_cast<char*>(&_data);
    memDump(data,2048);
    auto d=system_clock::now()-t;
    //every line dumps 16 bytes, and is 81 char long (considering \r\n)
    //so (2048/16)*81=10368
    iprintf("Time required to print 10368 char is %lldms\n",
        duration_cast<milliseconds>(d).count());
    unsigned int baudrate=10368*10000/duration_cast<milliseconds>(d).count();
    iprintf("Effective baud rate =%u\n",baudrate);
}

//
// Benchmark 2
//
/*
tests:
context switch speed
*/

static bool b2_v1;
static int b2_v2;

static void b2_p1(void *argv)
{
    for(;;)
    {
        if(Thread::testTerminate()) break;
        Thread::yield();
        if(b2_v1) b2_v2++;
    }
}

static int b2_f1(int priority)
{
    Thread::setPriority(priority);
    b2_v1=false;
    b2_v2=0;
    Thread *t1=Thread::create(b2_p1,STACK_SMALL,priority,NULL,Thread::JOINABLE);
    Thread *t2=Thread::create(b2_p1,STACK_SMALL,priority,NULL,Thread::JOINABLE);
    b2_v1=true; //Start counting
    Thread::sleep(1000);
    b2_v1=false; //Stop counting
    t1->terminate();
    t2->terminate();
    t1->join();
    t2->join();
    return b2_v2;
}

static void benchmark_2()
{
    #ifndef SCHED_TYPE_EDF
    iprintf("%d context switch per second (max priority)\n",b2_f1(3));
    iprintf("%d context switch per second (min priority)\n",b2_f1(0));
    #else //SCHED_TYPE_EDF
    iprintf("Context switch benchmark not possible with EDF\n");
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
    using namespace std::chrono;
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
    int i,max=0;
    auto total=system_clock::now();
    for(i=0;i<1024;i++)
    {
        auto part=system_clock::now();
        if(fwrite(buf,1,BUFSIZE,f)!=BUFSIZE)
        {
            iprintf("Write error\n");
            break;
        }
        auto d=system_clock::now()-part;
        max=std::max(max,static_cast<int>(duration_cast<milliseconds>(d).count()));
    }
    auto d=system_clock::now()-total;
    if(fclose(f)!=0) iprintf("Error in fclose 1\n");
    iprintf("Filesystem write benchmark\n");
    unsigned int writeTime=duration_cast<milliseconds>(d).count();
    unsigned int writeSpeed=static_cast<unsigned int>(1024000.0/writeTime);
    iprintf("Total write time = %dms (%dKB/s)\n",writeTime,writeSpeed);
    iprintf("Max filesystem latency = %dms\n",max);
    //Read benchmark
    max=0;
    if((f=fopen(FILENAME,"r"))==NULL)
    {
        iprintf("Filesystem read benchmark not made. Can't open file\n");
        delete[] buf;
        return;
    }
    setbuf(f,NULL);
    total=system_clock::now();
    for(i=0;i<1024;i++)
    {
        memset(buf,0,BUFSIZE);
        auto part=system_clock::now();
        if(fread(buf,1,BUFSIZE,f)!=BUFSIZE)
        {
            iprintf("Read error 1\n");
            break;
        }
        auto d=system_clock::now()-part;
        max=std::max(max,static_cast<int>(duration_cast<milliseconds>(d).count()));
        for(unsigned j=0;j<BUFSIZE;j++) if(buf[j]!='0')
        {
            iprintf("Read error 2\n");
            goto quit;
        }
    }
    quit:
    d=system_clock::now()-total;
    if(fclose(f)!=0) iprintf("Error in fclose 2\n");
    iprintf("Filesystem read test\n");
    unsigned int readTime=duration_cast<milliseconds>(d).count();
    unsigned int readSpeed=static_cast<unsigned int>(1024000.0/readTime);
    iprintf("Total read time = %dms (%dKB/s)\n",readTime,readSpeed);
    iprintf("Max filesystem latency = %dms\n",max);
    delete[] buf;
}

//
// Benchmark 4
//
/*
tests:
Mutex lock/unlock time
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
