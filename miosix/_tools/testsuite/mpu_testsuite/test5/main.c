#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main()
{
	// 'address' points to a memory location inside the code
	// segment of the program. The program tries to write the location.
	volatile unsigned int *address = 0x64101000;
	*address = 0xbbbbbbbb;
	return 0;
}
