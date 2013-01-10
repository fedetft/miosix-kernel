#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main()
{
	volatile unsigned int *pointer = 0x64100200;
        volatile unsigned int c = *pointer;
	return 124;
}
