
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

void readLine(char *str, int len)
{
    if(len==0) return;
    for(int i=0;i<len;i++)
    {
        str[i]=getchar();
        //in raw mode no conversion of \r to \n is performed!
        if(str[i]!='\n' && str[i]!='\r' && i!=len-1) continue;
        str[i]='\0';
        return;
    }
}

int main()
{
    bool raw=false;
    bool echo=true;
    puts("R toggles raw mode, and E toggles echo mode");
    for(;;)
    {
        char line[256];
        readLine(line,sizeof(line));
        if(strlen(line)==0) return 0;
        struct termios t;
        if(line[0]=='R')
        {
            raw=!raw;
            tcgetattr(STDIN_FILENO,&t);
            if(raw) t.c_lflag &= ~(ISIG | ICANON);
            else t.c_lflag |= (ISIG | ICANON);
            tcsetattr(STDIN_FILENO,TCSANOW,&t);
        } else if(line[0]=='E') {
            echo=!echo;
            tcgetattr(STDIN_FILENO,&t);
            if(echo) t.c_lflag |= ECHO;
            else t.c_lflag &= ~ECHO;
            tcsetattr(STDIN_FILENO,TCSANOW,&t);
        }
        printf("You entered '%s'\n",line);
    }
}
