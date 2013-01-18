#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main()
{
        // This process goes to sleep for 2 seconds, while the second
        // process tries to  access the data region of this process.
        unsigned int x = 5000000;
        unsigned int i;
        for(int i=0; i<x; i++)
        return 0;
}
