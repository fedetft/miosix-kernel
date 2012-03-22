
#include <unistd.h>

int mystrlen(const char *s)
{
	int result=0;
	while(*s++) result++;
	return result;
}

int main()
{
	static const char str[]="1=sleep 5s, 2=exit\n";
	static const char str2[]="Unexpected command\n";
	for(;;)
	{
		char result[100];
		write(1,str,mystrlen(str));
		int len=read(0,result,sizeof(result));
		if(len<1) continue;
		switch(result[0])
		{
			case '1':
				usleep(5000000);
				break;
			case '2':
				return 0;
			default:
				write(1,str2,mystrlen(str2));
		}
	}
}
