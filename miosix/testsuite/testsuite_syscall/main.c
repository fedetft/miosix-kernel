#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define error(x)	(x)

#define SIZE 1024

int mystrlen(const char *s){
	int result=0;
	while(*s++) result++;
	return result;
}

void print(const char *s){
	write(1, s, mystrlen(s));
}

int main(){
	unsigned char buffer[SIZE],
				  buffer2[SIZE];
	
	int i = 0;
	for(i = 0; i < SIZE; i++)
		buffer[i] = i;
	
	int fd = open("/testsuite.bin", O_RDWR, 0);
	
	if(fd != -1)
		return error(1);
	
	print("'Open' of non existing file: PASSED\n");
	fd = open("/testsuite.bin", O_RDWR|O_TRUNC, 0);
	
	if(fd == -1)
		return error(2);
	
	if(fd >= 0 && fd <= 2)
		return error(3);
	
	print("Open file: PASSED\n");
	
	if(write(fd, buffer, SIZE) != SIZE)
		return error(4);
	
	print("Write file: PASSED\n");
		
	seek(fd, SEEK_SET, 0);
		
	if(read(fd, buffer2, SIZE) != SIZE)
		return error(5);
		
	for(i = 0; i < SIZE; i++){
		if(buffer[i] != buffer2[i])
			return error(6);
	}
	
	print("Read: PASSED\n");
		
	if(close(fd) != 0)
		return error(7);
	
	print("close: PASSED\n");
	
	fd = open("/testsuite.bin", O_RDWR);
	
	if(fd < 2)
		return error(8);
	
	print("Open of an existing file: PASSED\n");
		
	if(read(fd, buffer2, SIZE) != SIZE)
		return error(9);
			
	for(i = 0; i < SIZE; i++){
		if(buffer[i] != buffer2[i])
			return error(10);
	}
	
	print("Read data corectness: PASSED\n");
	
	if(close(fd))
		return error(11);
	
	print("close: passed\n");
	
	return 0;
}
