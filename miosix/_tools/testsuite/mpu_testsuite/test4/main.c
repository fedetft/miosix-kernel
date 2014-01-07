#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main()
{
	// The process tries to allocate an array of size greater than
	// the available memory and writes to it;
	volatile unsigned char array[16 * 1024 + 1];
	array[0] = 0;
	return 0;
}
