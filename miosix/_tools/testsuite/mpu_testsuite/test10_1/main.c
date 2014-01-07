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
        unsigned int i=0;
        for(i=0; i<5000000; i++)
        return 0;
}
