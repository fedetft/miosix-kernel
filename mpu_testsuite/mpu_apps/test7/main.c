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
	// 16376 = (16 * 1024) - 8

//volatile unsigned char array[16345];
	//volatile unsigned char array[100];
	//array[0] = 0;
	//itoa(array + 16200/*16012*/, array, 16);
//write(1, "asd", 3);
//array[0]=1;
	//itoa(str+10, str, 16);
	//write(1, str, 8);
//write(1, "asd\n", 4);
	//usleep(2000000);
	
	return 0;
}
