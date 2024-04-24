
// Kernel-side program to start a userspace process

#include <cstdio>
#include <sys/wait.h>
#include <signal.h>
#include <spawn.h>

using namespace std;

int main()
{
    pid_t pid;
    const char *arg[] = { "/bin/hello", nullptr };
    const char *env[] = { nullptr };
    int ec=posix_spawn(&pid,arg[0],NULL,NULL,(char* const*)arg,(char* const*)env);
    if(ec!=0)
    {
        iprintf("spawn returned %d\n",ec);
        return 1;
    }
    pid=wait(&ec);
    iprintf("pid %d exited\n",pid);
    if(WIFEXITED(ec))
    {
        iprintf("Exit code is %d\n",WEXITSTATUS(ec));
    } else if(WIFSIGNALED(ec)) {
        if(WTERMSIG(ec)==SIGSEGV) iprintf("Process segfaulted\n");
        else iprintf("Process terminated due to an error\n");
    }
    return 0;
}
