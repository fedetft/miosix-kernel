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

unsigned int *allocatedMem;

int main()
{
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
	allocatedMem = memAllocation(4096);
	mpuTest1();
	mpuTest2();
	mpuTest3();
	mpuTest4();
	mpuTest5();
	mpuTest6();
	mpuTest7();
	mpuTest8();
	mpuTest9();
}

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
	iprintf("Executing MPU Test 7...\n");
	ElfProgram prog(reinterpret_cast<const unsigned int*>(test7_elf), test7_elf_len);
	pid_t child=Process::create(prog);
	delayMs(1000);
	Process::waitpid(child, &ec, 0);

	if(isSignaled(ec))
	{
		iprintf("...passed!.\n\n");
	}
	else
	{
		iprintf("...not passed! Process exited normally.\n\n");
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
		catch (bad_alloc &ex)
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
	iprintf("Test passed\n\n");
}
