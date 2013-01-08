#include <cstdio>
#include <sys/wait.h>
#include <signal.h>
#include "miosix.h"
#include "kernel/process.h"

//Include the test programs
#include "app_mpu_test/includes.h"

using namespace std;
using namespace miosix;

int main()
{
	ElfProgram prog(reinterpret_cast<const unsigned int*>(test1_elf),test1_elf_len);
	pid_t child=Process::create(prog);
	int ec;
	pid_t pid;
	pid=Process::waitpid(child,&ec,0);
	iprintf("Process %d terminated\n",pid);
	if(WIFEXITED(ec))
	{
		iprintf("Exit code is %d\n",WEXITSTATUS(ec));
	}
	else if(WIFSIGNALED(ec))
	{
		if(WTERMSIG(ec)==SIGSEGV) iprintf("\nProcess segfaulted. The system is safe!!\n");
	}
	return 0;
}
