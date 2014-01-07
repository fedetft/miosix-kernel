#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int mystrlen(const char *s)
{
	int result=0;
	while(*s++) result++;
	return result;
}

int main()
{
	int i = 0;
	static const char str[]="Test application:\n0=divzero\n1=sleep 5s\n2=exit\n3=bkpt\n4=dangling\n5=open\n6=system\n";
	static const char str2[]="Unexpected command\n";
	static const char okMsg[] = "Everything's shiny, Cap'n\n";

	for(;;)
	{
		char result[100];
		write(1,str,mystrlen(str));
		int len=read(0,result,sizeof(result));
		if(len<1) continue;
		int i=10/(int)(result[0]-'0');
		unsigned int *p=(unsigned int *)0xc0000000;
		
		int fp = 0;
		
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
			case '5':
				fp = open("/test.txt", O_RDWR|O_TRUNC, 0);
				
				if(fp != -1){
					write(1, "File opened\n", 12);
					int check = 0;
					
					if(write(fp, "ciao", 4) == 4){
						write(1, "Write ok\n", 9);
						check |= 0x01;
					}
				
					//close(fp);
					//fp = open("/test.txt", O_RDWR, 0);
					seek(fp, SEEK_SET, 0);
					
					if(write(fp, "123", 3) == 3){
						write(1, "Write ok\n", 9);
						check |= 0x01;
					}
					
					seek(fp, SEEK_SET, 0);
					
					if(read(fp, result, 100) == 4){
						write(1, "Read ok\n", 8);
						check |= 0x02;
					}
					
					if(close(fp) == 0){
						write(1, "Close ok\n", 9);
						check |= 0x04;
					}
					
					if(check == 0x07)
						write(1, okMsg, mystrlen(okMsg));
				}
				 
				
				break;
			case '6':
				system("test");
				break;
			default:
				write(1,str2,mystrlen(str2));
		}
	}
}
