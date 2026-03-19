#include <spawn.h>
#include <sys/wait.h>

#include <cstdio>

void start_process(pid_t* pid, char* const argv[], char* const envp[])
{
    int error_code = posix_spawn(pid, argv[0], NULL, NULL, argv, envp);

    if(error_code != 0)
    {
        printf("spawn returned %d\n", error_code);
    }
}

int main()
{
    pid_t pid1, pid2;

    // Arguments and environment variables
    const char* arg1[] = {"/bin/process1", nullptr};
    const char* arg2[] = {"/bin/process2", nullptr};
    const char* env[]  = {nullptr};

    // Start the processes
    start_process(&pid1, (char* const*)arg1, (char* const*)env);
    start_process(&pid2, (char* const*)arg2, (char* const*)env);

    // Wait for them to end
    wait(NULL);
    wait(NULL);

    printf("All processes ended\n");

    return 0;
}
