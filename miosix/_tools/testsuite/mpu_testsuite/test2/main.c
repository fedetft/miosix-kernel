#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main()
{
	// 'address' points to a memory location allocated with ProcessPool
	// and owned by the kernel. The program tries to read the location.
	volatile unsigned int *address = 0x64100200;
	volatile unsigned int c = *address;
	return 0;
}
