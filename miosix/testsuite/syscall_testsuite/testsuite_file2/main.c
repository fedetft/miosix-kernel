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


char *strrev(char *str, unsigned int len){
	if(str == 0 || *str == '\0')
		return str;
	
	char tmp = 0, *p1, *p2;
	for(*p1 = str, *p2 = str + len - 1; p2 > p1; ++p1, --p2){
		tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
	}

	return str;
}

int fast_itoa(int value, char *pBuffer){
	char *pTmp = pBuffer;
	unsigned int digitCount = 0;

	if(value == 0){
		*pTmp = '0';
		return 1;
	}

	if(value < 0){
		value = -value;
		*pTmp++ = '-';
		++digitCount;
	}

	while(value > 0){
		*pTmp++ = (value % 10) + '0';
		value /= 10;
		++digitCount;
	}

	*pTmp = '\0';

	strrev(pBuffer, digitCount);
	return digitCount;
}

int main(){
	int len = mystrlen(TEST_STRING);
	
	int fd = open(TEST_FILE, O_RDWR|O_TRUNC, 0);
	
	if(fd == -1)
		return 1;
	
	char buffer[30] = {0};
	
	int i = 0;
	for(i = 0; i < 1000; i++){
/*		if(i % 10 == 0){
			fast_itoa(i, buffer);
			print("\nProcess 2: ");
			print(buffer);
		}
	*/				
		if(write(fd, TEST_STRING, len) != len){
			close(fd);
			return 2;
		}
	}
	
	close(fd);

	return 0;
}
