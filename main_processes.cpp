
#include <cstdio>
#include <sys/wait.h>
#include <signal.h>
#include "miosix.h"
#include "kernel/process.h"
#include "kernel/SystemMap.h"

#include "app_template/prog3.h"

using namespace std;
using namespace miosix;

void ledThread(void *)
{
    for(;;)
    {
        ledOn();
        Thread::sleep(200);
        ledOff();
        Thread::sleep(200);
    }
}

int main()
{
    Thread::create(ledThread,STACK_MIN);
	
	//SystemMap::instance().addElfProgram("test", reinterpret_cast<const unsigned int*>(test_elf), test_elf_len);
	
	//iprintf("SystemMap::size: %d\n", SystemMap::instance().getElfCount());
	//std::pair<const unsigned int*, unsigned int> res = SystemMap::instance().getElfProgram("test");
	//iprintf("SystemMap test entry size: %X %d\n", res.first, res.second);
    
    ElfProgram prog(reinterpret_cast<const unsigned int*>(main_elf),main_elf_len);
    for(int i=0;;i++)
    {
        getchar();
        pid_t child=Process::create(prog);
        int ec;
        pid_t pid;
        if(i%2==0) pid=Process::wait(&ec);
        else pid=Process::waitpid(child,&ec,0);
        iprintf("Process %d terminated\n",pid);
        if(WIFEXITED(ec))
        {
            iprintf("Exit code is %d\n",WEXITSTATUS(ec));
        } else if(WIFSIGNALED(ec)) {
            if(WTERMSIG(ec)==SIGSEGV) iprintf("Process segfaulted\n");
        }
    }
}
