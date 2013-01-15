#include <cstdio>
#include <sys/wait.h>
#include <signal.h>
#include <stdexcept>
#include <string.h>
#include "miosix.h"
#include "kernel/process.h"
#include "kernel/process_pool.h"

//Include the test programs
#include "mpu_testsuite/altered_elfs/includes.h"
#include "mpu_testsuite/mpu_apps/includes.h"

using namespace std;
using namespace miosix;

unsigned int* memAllocation(unsigned int size);
void runElfTest(const char *name, const unsigned char *filename, unsigned int file_length);

int runProgram(const unsigned char *filename, unsigned int file_length);
bool isSignaled(int exit_code);
void mpuTest1();
void mpuTest2();
void mpuTest3();
void mpuTest4();
void mpuTest5();

int main()
{
	//ProcessPool allocates memory starting from address 0x64100000

	// Altered elfs tests
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
	memAllocation(2048);
	mpuTest1();
	mpuTest2();
	mpuTest3();
	mpuTest4();
	mpuTest5();
}

unsigned int* memAllocation(unsigned int size)
{
	unsigned int *p = ProcessPool::instance().allocate(size);
	memset(p, WATERMARK_FILL, size);
	iprintf("Allocated 2048 bytes. Base: %p. Size: 0x%x.\n\n", p, size);
	return p;
}

void runElfTest(const char *name, const unsigned char *filename, unsigned int file_length)
{
	iprintf("Executing %s...", name);
	try
	{
		ElfProgram prog(reinterpret_cast<const unsigned int*>(filename),file_length);
		iprintf("not passed.\n");
	}
	catch (runtime_error err)
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
		else if(*addr == 0xaaaaaaaa)
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
		if(*addr == 0xaaaaaaaa)
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
	unsigned int *addr = (unsigned int*) 0x64100800;
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
