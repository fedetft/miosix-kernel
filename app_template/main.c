
#include <unistd.h>

int mystrlen(const char *s)
{
	int result=0;
	while(*s++) result++;
	return result;
}

int main()
{
	//FIXME: without static code fails!!
	static const char str[]="Hello world\n";
	write(1,str,mystrlen(str));
	return 0;
}
