#include <cstdio>
#include <sys/wait.h>
#include <signal.h>
#include "miosix.h"
#include "kernel/process.h"

//Include the test programs
#include "mpu_testsuite/mpu_apps/includes.h"

using namespace std;
using namespace miosix;

void checkProcessTermination(int ec);
void test1();
void test2();

int main()
{
	test1();
	test2();
}

void checkProcessTermination(int ec)
{
	if(WIFEXITED(ec))
	{
		iprintf("Process exited correctly. Exit code is %d. Something wrong happened!!\n",WEXITSTATUS(ec));
	}
	else if(WIFSIGNALED(ec))
	{
		if(WTERMSIG(ec)==SIGSEGV) iprintf("Process segfaulted. The system is safe!!\n");
	}
}

void test1()
{
	ElfProgram prog(reinterpret_cast<const unsigned int*>(test1_elf),test1_elf_len);
	pid_t child=Process::create(prog);
	int ec;
	pid_t pid;
	pid=Process::waitpid(child,&ec,0);
	iprintf("Process %d terminated\n",pid);
	checkProcessTermination(ec);
}

void test2()
{
	ElfProgram prog(reinterpret_cast<const unsigned int*>(test2_elf),test2_elf_len);
	pid_t child=Process::create(prog);
	int ec;
	pid_t pid;
	pid=Process::waitpid(child,&ec,0);
	iprintf("Process %d terminated\n",pid);
	checkProcessTermination(ec);
}
