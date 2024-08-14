#include <spawn.h>
#include <sys/wait.h>

#include <cstdio>

int main()
{
    pid_t pid1, pid2;

    // Arguments and environment variables
    const char *arg1[] = {"/bin/process1", nullptr};
    const char *arg2[] = {"/bin/process2", nullptr};
    const char *env[]  = {nullptr};

    // Start the processes
    posix_spawn(&pid1, arg1[0], NULL, NULL, (char *const *)arg1,
                (char *const *)env);
    posix_spawn(&pid2, arg2[0], NULL, NULL, (char *const *)arg2,
                (char *const *)env);

    // Wait for them to end
    wait(NULL);
    wait(NULL);

    printf("All processes ended\n");

    return 0;
}
