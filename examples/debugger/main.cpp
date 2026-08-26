#include <miosix.h>
#include <sys/wait.h>
#include <spawn.h>
#include <sys/stat.h>
#include <fcntl.h>

// // Enable processes uncommenting
// #define WITH_PROCESSES
// // in miosix_settings.h

// // Enable process debugger uncommenting
// #define PROCESS_DEBUGGER
// // in miosix_settings.h

// // Enable auxiliar tty defining
// #define AUX_SERIAL "auxtty"
// // in miosix_settings.h

#include <debugger/debugger.h>

int main() {

    // Spawn a process
    pid_t pid,ec;
    const char *arg[] = { "/bin/hello", nullptr };
    const char *env[] = { nullptr };

    ec = posix_spawn(&pid,arg[0],NULL,NULL,(char* const*)arg,(char* const*)env);
    if(ec!=0)
    {
        perror("posix_spawn");
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

    // Start debugger

    // Reference executable for debugger is in './process_template/bin'

    // // Alternatively, pass the path directly
    // debugger.listen("/dev/auxtty");
    miosix::Debugger debugger;
    int serial = open("/dev/auxtty",O_RDWR | O_NOCTTY);
    debugger.listen(serial);
    close(serial);

    // Code here runs if debugger fails
    iprintf("Debugger failed\n");

}
