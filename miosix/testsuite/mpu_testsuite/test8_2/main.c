#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main()
{
	// 'address' points to a memory location of the program 8.1.
	// The program tries to write the location.
	volatile int *address = 0x64104004;
	*address = 0xbbbbbbbb;
	return 0;
}
