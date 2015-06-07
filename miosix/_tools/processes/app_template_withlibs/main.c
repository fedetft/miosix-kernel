
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main()
{
	printf("hello world %d\n",25);
	static const char str[]="0=divzero 1=sleep 5s, 2=exit 3=bkpt 4=dangling\n";
	static const char str2[]="Unexpected command\n";
	for(;;)
	{
		char result[100];
		write(1,str,strlen(str));
		int len=read(0,result,sizeof(result));
		if(len<1) continue;
		int i=10/(int)(result[0]-'0');
		unsigned int *p=(unsigned int *)0xc0000000;
		switch(result[0])
		{
			case '0':
				usleep(i);
				break;
			case '1':
				usleep(5000000);
				break;
			case '2':
				return 0;
			case '3':
				asm volatile("bkpt");
				break;
			case '4':
				usleep(*p);
				break;
			default:
				write(1,str2,strlen(str2));
		}
	}
}
