#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main(){
	int fd = open("/rdtest.txt", O_RDWR|O_TRUNC, 0);
	
	read(fd, 0x00, 10);
	return 0;
}

