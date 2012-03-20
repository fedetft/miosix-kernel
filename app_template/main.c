
#include <unistd.h>

int mystrlen(const char *s)
{
	int result=0;
	while(*s++) result++;
	return result;
}

int a=0;
int *b=&a;

int main()
{
	static const char str[]="Hello world\n";
//	return write(1,str,mystrlen(str));
	a=write(1,str,mystrlen(str));
	return *b;
}
