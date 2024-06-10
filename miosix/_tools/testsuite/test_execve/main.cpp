
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

extern char **environ;

int main(int argc, char *argv[], char *envp[])
{
    if(argc!=1) return 66;
    if(argv[0]==nullptr) return 66;
    if(argv[1]!=nullptr) return 66;
    if(strcmp(argv[0],"/bin/test_execve")!=0) return 66;
    if(environ!=envp) return 66;
    if(environ[0]==nullptr) return 66;
    if(environ[1]!=nullptr) return 66;
    if(strcmp(environ[0],"TEST_ENVP=test")!=0) return 66;
    return 77;
}
