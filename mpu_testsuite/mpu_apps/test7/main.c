#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main()
{
	// The process tries to fill almost all the stack with an array
	// and then tries to save the context by calling a syscall.
	// With an array of 16344 bytes, there is no fault.
	volatile unsigned char array[16345];
	array[0]=1;
	usleep(2000000);
	return 0;
}
