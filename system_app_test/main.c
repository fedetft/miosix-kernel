#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main(){
	int x[64 * 1024];
	
	int i = 123;
	unsigned int *p = &i;
	
	p = p - 1;
	*p = 567;

	
	return 123;
}
