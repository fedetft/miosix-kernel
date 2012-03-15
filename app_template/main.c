
#include <unistd.h>

int mystrlen(const char *s)
{
	int result=0;
	while(*s++) result++;
	return result;
}

int youShouldNotSeeThisInTheElfFile=0;
int *youShouldNotSeeThisInTheElfFile2=&youShouldNotSeeThisInTheElfFile;

int main()
{
	//FIXME: without static code fails!!
	static const char str[]="Hello world\n";
	youShouldNotSeeThisInTheElfFile=write(1,str,mystrlen(str));
	return *youShouldNotSeeThisInTheElfFile2;
}
