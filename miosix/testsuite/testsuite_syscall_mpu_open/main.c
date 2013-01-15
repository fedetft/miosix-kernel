#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main(){
	int fd = open(0x00000000, O_RDWR|O_TRUNC, 0);
	return 0;
}

