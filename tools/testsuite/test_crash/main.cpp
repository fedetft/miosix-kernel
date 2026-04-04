/***************************************************************************
 *   Copyright (C) 2025 by Terraneo Federico                               *
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

// A collection of actions a process can do to crash the kernel, used to test
// if the abstraction of processes is implemented correctly

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

//Choose whether arg are read from command line or stdin
#define ARG_FROM_CMDLINE

using namespace std;

#ifdef ARG_FROM_CMDLINE
char **global_argv;
#endif

/**
 * Helper function to get an address
 */
int *address()
{
    int *p;
    #ifdef ARG_FROM_CMDLINE
    siscanf(global_argv[2],"%p",&p);
    #else
    iscanf("%p",&p);
    #endif
    return p;
}

/**
 * Plain old division by 0
 */
void tryZero()
{
    volatile int i=0;
    exit(42/i);
}

/**
 * Attempt to disable interrupts
 */
void tryLockup()
{
    //NOTE: although these instructions are privileged, ARM does not trap them
    //but rather turns them into nop if the CPU is in user mode
    asm volatile("cpsid i   \n\t"
                 "cpsid f   \n\t");
}

/**
 * What if the CPU encounters a breakpoint instruction? We don't want this to
 * become a DoS if no debugger is connected
 */
void tryBkpt()
{
    asm volatile("bkpt");
}

/**
 * Arbitrary address memory read
 */
void tryRead()
{
    exit(*address());
}

/**
 * Arbitrary address memory write
 */
void tryWrite()
{
    *address()=0;
}

/**
 * Arbitrary address memory read through syscall
 */
void trySysRead()
{
    //To write to file, kernel would need to read memory
    int fd=open("/dev/null",O_WRONLY);
    if(write(fd,address(),1)!=-1 || errno!=EFAULT) exit(1);
    exit(0);
}

/**
 * Arbitrary address memory write through syscall
 */
void trySysWrite()
{
    //To read from file, kernel would need to write memory
    int fd=open("/dev/zero",O_RDONLY);
    if(read(fd,address(),1)!=-1 || errno!=EFAULT) exit(1);
    exit(0);
}

/**
 * Arbitrary address execute
 */
void tryExec()
{
    auto fun=reinterpret_cast<void (*)()>(address());
    fun();
}

/**
 * Try executing a thumb2 function without bit 0 set, thus telling the CPU it's
 * coded in the ARM instruction set, not the thumb2 one. Should fail with invalid
 * EPSR access
 */
void tryEpsr()
{
    int *(*p)()=&address; //Valid pointer to function
    unsigned int pp=reinterpret_cast<unsigned int>(p);
    pp &= ~1; //Make pointer invalid (non-thumb)
    p=reinterpret_cast<int *(*)()>(pp);
    p();
}

/**
 * What if the CPU encounters an invalid instruction?
 */
void tryInvalid()
{
    asm volatile(".word 0xffffffff"); //0xffffffff is an ARM invalid instruction
}

/**
 * Set stack pointer to an arbitrary value and after that cause a syscall.
 * The CPU will try to save registers on the stack, but the stack is not valid
 * so it should fault badly, the OS needs to be capable of recovering
 */
void __attribute__((naked,noreturn)) tryStack()
{
    asm volatile("bl   _Z7addressv \n\t"
                 "mov  sp, r0      \n\t"
                 "movs r3, #43     \n\t" //syscall(43) is exit
                 "svc  0           \n\t");
}

/**
 * Plain old stack overflow
 */
void __attribute__((noinline)) tryOverflow()
{
    volatile char big[3*1024];
    big[0]=0;
    sleep(1);
    exit(big[0]);
}

/**
 * This should be the interrupt return pattern. When running it code faults with
 * attempted intruction fetch @ 0xfffffffc so CPU knows it's not inside an IRQ,
 * ignores IRQ return, interprets the number as an address, and faults
 * attempting to jump there
 */
void tryIret()
{
    asm volatile("movs r0, #0         \n\t"
                 "movs r1, #3         \n\t"
                 "sub  r0, r0, r1     \n\t" //0-3=0xfffffffd
                 "mov  lr, r0         \n\t"
                 "bx   lr             \n\t");
}

/**
 * Causes a usage fault with the stack in an invalid location. Tests the case in
 * which multiple exceptions are pending simultaneously. Miosix should only
 * report the root exception (the usage fault) and ignore the memory fault.
 */
void __attribute__((naked,noreturn)) tryMultiple()
{
    asm volatile("bl   _Z7addressv \n\t"
                 "mov  sp, r0      \n\t"
                 "movs r0, #0      \n\t"
                 "movs r1, #123    \n\t"
                 ".word 0xffffffff \n\t"); //0xffffffff is an ARM invalid instruction
}

int *foo() { return reinterpret_cast<int*>(0x1000); }
void __attribute__((naked,noreturn)) nofloat()
{
    //Attempt to cause a stack overflow while entering an svc from a process
    //that does not have floating point registers that need to be saved in the
    //stack frame
    asm volatile("bl   _Z3foov     \n\t"
                 "mov  sp, r0      \n\t"
                 "movs r3, #43     \n\t" //syscall(43) is exit
                 "svc  0           \n\t");
}

int *bar()
{
    volatile float f=8192.f;
    volatile int i=f/2;
    return reinterpret_cast<int*>(i);
}
void __attribute__((naked,noreturn)) yesfloat()
{
    //Attempt to cause a stack overflow while entering an svc from a process
    //that does have floating point registers that need to be saved in the
    //stack frame. This is of course only relevant for ARM cores with hardware
    //floating point extensions, otherwise this test behaves exactly as noFloat
    asm volatile("bl   _Z3barv     \n\t"
                 "mov  sp, r0      \n\t"
                 "movs r3, #43     \n\t" //syscall(43) is exit
                 "svc  0           \n\t");
}

int main(int argc, char *argv[])
{
    #ifdef ARG_FROM_CMDLINE
    global_argv=argv;
    switch(argv[1][0])
    #else
    switch(getchar())
    #endif
    {
        case 'z': tryZero();     break;
        case 'l': tryLockup();   break;
        case 'b': tryBkpt();     break;
        case 'r': tryRead();     break;
        case 'w': tryWrite();    break;
        case 'R': trySysRead();  break;
        case 'W': trySysWrite(); break;
        case 'x': tryExec();     break;
        case 'e': tryEpsr();     break;
        case 'i': tryInvalid();  break;
        case 's': tryStack();    break;
        case 'o': tryOverflow(); break;
        case 'u': tryIret();     break;
        case 'm': tryMultiple(); break;
        case '-': nofloat();     break;
        case '+': yesfloat();    break;
    }
    return 0;
}
