#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define error(x)	(x)

#define SIZE 1024

int main(){
	unsigned char buffer[SIZE],
				  buffer2[SIZE];
	
	int i = 0;
	for(i = 0; i < SIZE; i++)
		buffer[i] = i;
	
	int fd = open("/testsuite.bin", O_RDWR, 0);
	
	if(fd != -1)
		return error(1);
	
	close(fd);
	
	fd = open("/testsuite.bin", O_RDWR|O_TRUNC, 0);
	
	if(fd == -1)
		return error(2);
	
	if(fd >= 0 && fd <= 2)
		return error(3);
	
	if(write(fd, buffer, SIZE) != SIZE)
		return error(4);
		
	seek(fd, SEEK_SET, 0);
		
	if(read(fd, buffer2, SIZE) != SIZE)
		return error(5);
		
	for(i = 0; i < SIZE; i++){
		if(buffer[i] != buffer2[i])
			return error(6);
	}		
		
	if(close(fd) != 0)
		return error(7);
	
	fd = open("/testsuite.bin", O_RDWR);
	
	if(fd < 2)
		return error(8);
		
	if(read(fd, buffer2, SIZE) != SIZE)
		return error(9);
			
	for(i = 0; i < SIZE; i++){
		if(buffer[i] != buffer2[i])
			return error(10);
	}
	
	if(close(fd))
		return error(11);
	
	return 0;
}
