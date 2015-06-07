#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main()
{
	// 'address' points to a memory location allocated with ProcessPool
	// and owned by the kernel. The program tries to write the location.
	volatile unsigned int *address = 0x64100000;
	*address = 0xbbbbbbbb;
	return 0;
}
