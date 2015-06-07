#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main()
{
	// 'address' points to a memory location allocated and owned by
	// the kernel. The program tries to write the location.
	volatile unsigned int *address = 0x63F0FFFF;
	*address = 0xbbbbbbbb;
	return 0;
}
