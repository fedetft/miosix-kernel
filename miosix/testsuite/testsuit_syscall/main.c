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
	
	int fd = open("/testsuit.bin", O_RDWR, 0);
	
	if(fd != -1)
		return error(0);
	
	fd = open("/testsuit.bin", O_RDWR|O_TRUNC, 0);
	
	if(fd == -1)
		return error(1);
	
	if(fd >= 0 && fd <= 2)
		return error(2);
	
	if(write(fd, buffer, SIZE) != SIZE)
		return error(3);
		
	seek(fd, SEEK_SET, 0);
		
	if(read(fd, buffer2, SIZE) != SIZE)
		return error(4);
		
	for(i = 0; i < SIZE; i++){
		if(buffer[i] != buffer2[i])
			return error(5);
	}		
		
	if(close(fd) != 0)
		return error(6);
	
	fd = open("/testsuit.bin", O_RDWR);
	
	if(fd < 2)
		return error(7);
		
	if(read(fd, buffer2, SIZE) != SIZE)
		return error(8);
			
	for(i = 0; i < SIZE; i++){
		if(buffer[i] != buffer2[i])
			return error(9);
	}
	
	if(close(fd))
		return error(10);
	
	return 0;
}
