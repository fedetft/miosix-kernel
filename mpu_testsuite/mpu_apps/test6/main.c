#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main()
{
	// 'address' points to a memory location between the
	// code segment and the data segment.
	volatile unsigned int *address = 0x64101404;
	*address = 0xbbbbbbbb;
	return 0;
}
