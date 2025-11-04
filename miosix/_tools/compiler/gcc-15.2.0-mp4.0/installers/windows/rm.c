/*
 * FIXME: try to crosscompile coreutils for windows, so as to get a real rm.exe
 */

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int i;
	for(i=0;i<argc;i++)
	{
		if(strlen(argv[i])>0 && argv[i][0]!='-')
			remove(argv[i]);
	}
	return 0;
}
