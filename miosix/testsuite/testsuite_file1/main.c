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
	int len = mystrlen(TEST_STRING);
	
	int fd = open(TEST_FILE, O_RDWR|O_TRUNC, 0);
	
	if(fd == -1)
		return 1;
	
	if(write(fd, TEST_STRING, len) != len){
		close(fd);
		return 2;
	}
	
	close(fd);

	return 0;
}
