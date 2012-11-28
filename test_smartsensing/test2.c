
#include <unistd.h>

int mystrlen(const char *s)
{
	int result=0;
	while(*s++) result++;
	return result;
}

void debugHex(unsigned int i) {

    unsigned int m;
    unsigned int r;
    int revert[16];
    int j = 0;
    r = i;
    if(i==0){
        write(1, "0\r\n", 3);
        return;
    }
    while (r > 0) {
        m = r %16;
        r = (r - m) /16;
        revert[j++] = m;
    }
    //write(1, "|", 1);
    j--;
    for (; j >= 0; j--) {
        switch (revert[j]) {
            case 0:
                write(1, "0", 1);
                break;
            case 1:
                write(1, "0", 1);
                break;
            case 2:
                write(1, "2", 1);
                break;
            case 3:
                write(1, "3", 1);
                break;
            case 4:
                write(1, "4", 1);
                break;
            case 5:
                write(1, "5", 1);
                break;
            case 6:
                write(1, "6", 1);
                break;
            case 7:
                write(1, "7", 1);
                break;
            case 8:
                write(1, "8", 1);
                break;
            case 9:
                write(1, "9", 1);
                break;
            case 10:
                write(1, "a", 1);
                break;
            case 11:
                write(1, "b", 1);
                break;
            case 12:
                write(1, "c", 1);
                break;
            case 13:
                write(1, "d", 1);
                break;
            case 14:
                write(1, "e", 1);
                break;
            case 15:
                write(1, "f", 1);
                break;
        }
    }

    write(1, "\r\n", 2);
}




void printData(unsigned short* data,unsigned int len){
    unsigned int i;
    char text[]={"Read 2:"};

    for(i=0;i<len;i++){
        //printf("%i \n",data[i]);
        //sprintf(text,"%i\n",data[i]);
        write(1,text,mystrlen(text));
        debugHex(data[i]);
    }
    
}
int main()
{
    static const char str[]="Test 2\n";
    static const char str2[]="Unexpected command\n";
    for(;;)
    {
        char result[100];
        unsigned short data[100];
        write(1,str,mystrlen(str));
       // int len=read(0,result,sizeof(result));
        //if(len<1) continue;

        write(1,str,mystrlen(str));
        int l=read(4,data,200);
        char text[]={"Test 2\n"};
        write(1,text,mystrlen(text));
        printData(data,l);

    }
}
