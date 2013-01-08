#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main(){
	//int x[64 * 1024];
	//
	//int i = 123;
	//unsigned int *p = &i;
	//
	//p = p - 1;
	//*p = 567;
	
	volatile unsigned int *pointer = 0x63F00000;// + 0xFFFF;
	*pointer = 456;
	//if(*pointer == 456) write(1, "Ok\n", 3);
	//write(1, okMsg2, mystrlen(okMsg2));

	
	return 123;
}
