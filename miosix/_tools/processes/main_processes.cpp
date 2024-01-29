
#include <cstdio>
#include <sys/wait.h>
#include <signal.h>
#include "miosix.h"
#include "kernel/process.h"

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
    
    for(;;)
    {
        iprintf("Type enter to start process\n");
        getchar();
        pid_t child=Process::spawn("/bin/example_program");
        int ec;
        pid_t pid=Process::wait(&ec);
        iprintf("Process %d started, %d terminated\n",child,pid);
        if(WIFEXITED(ec))
        {
            iprintf("Exit code is %d\n",WEXITSTATUS(ec));
        } else if(WIFSIGNALED(ec)) {
            if(WTERMSIG(ec)==SIGSEGV) iprintf("Process segfaulted\n");
        }
    }
}
