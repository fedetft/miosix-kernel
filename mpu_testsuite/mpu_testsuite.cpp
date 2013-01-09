#include <cstdio>
#include <sys/wait.h>
#include <signal.h>
#include <stdexcept>
#include "miosix.h"
#include "kernel/process.h"
#include "kernel/process_pool.h"

//Include the test programs
#include "mpu_testsuite/altered_elfs/includes.h"
#include "mpu_testsuite/mpu_apps/includes.h"

using namespace std;
using namespace miosix;

void runElfTest(const char *name, const unsigned char *filename, unsigned int file_length);
void runMpuTest(const char *name, const unsigned char *filename, unsigned int file_length);
void allocationTest();

int main()
{
	allocationTest();

	// Altered elfs tests
	iprintf("\nExecuting ELF tests.\n");
	iprintf("--------------------\n");
	try
	{
		runElfTest("Test1", aelf1, aelf1_len);
		iprintf("not passed.\n");
	} catch (runtime_error err)
	{
		iprintf("passed.\n");
	}
	try
	{
		runElfTest("Test2", aelf2, aelf2_len);
		iprintf("not passed.\n");
	} catch (runtime_error err)
	{
		iprintf("passed.\n");
	}
	try
	{
		runElfTest("Test3", aelf3, aelf3_len);
		iprintf("not passed.\n");
	} catch (runtime_error err)
	{
		iprintf("passed.\n");
	}
	try
	{
		runElfTest("Test4", aelf4, aelf4_len);
		iprintf("not passed.\n");
	} catch (runtime_error err)
	{
		iprintf("passed.\n");
	}
	try
	{
		runElfTest("Test5", aelf5, aelf5_len);
		iprintf("not passed.\n");
	} catch (runtime_error err)
	{
		iprintf("passed.\n");
	}
	try
	{
		runElfTest("Test6", aelf6, aelf6_len);
		iprintf("not passed.\n");
	} catch (runtime_error err)
	{
		iprintf("passed.\n");
	}
	try
	{
		runElfTest("Test7", aelf7, aelf7_len);
		iprintf("not passed.\n");
	} catch (runtime_error err)
	{
		iprintf("passed.\n");
	}
	
	// Direct mpu tests
	iprintf("\n\nExecuting MPU tests.\n");
	iprintf("---------------------\n");
        runMpuTest("Test8", test1_elf, test1_elf_len);
	runMpuTest("Test9", test2_elf, test2_elf_len);
	runMpuTest("Test10", test3_elf, test3_elf_len);
}

void runElfTest(const char *name, const unsigned char *filename, unsigned int file_length)
{
	iprintf("Executing %s...", name);
	ElfProgram prog(reinterpret_cast<const unsigned int*>(filename),file_length);
}

void runMpuTest(const char *name, const unsigned char *filename, unsigned int file_length)
{
	iprintf("Executing %s...\n", name);
	ElfProgram prog(reinterpret_cast<const unsigned int*>(filename),file_length);
	pid_t child=Process::create(prog);
	int ec;
	pid_t pid;
	pid=Process::waitpid(child,&ec,0);
	//iprintf("Process %d terminated\n",pid);
	if(WIFEXITED(ec))
	{
		iprintf("not passed! (Exit status %d)\n", WEXITSTATUS(ec));
	}
	else if(WIFSIGNALED(ec))
	{
		if(WTERMSIG(ec)==SIGSEGV) iprintf("passed!\n");
	}
}

void allocationTest()
{
	iprintf("Executing Allocation test...\n");
	unsigned int *size = ProcessPool::instance().allocate(2048);
	iprintf("Allocated mem pointer: %p\n", size);
}
