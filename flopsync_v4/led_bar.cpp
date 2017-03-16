#include "led_bar.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

using namespace std;

#ifndef _MIOSIX

namespace miosix {

static void memPrint(const char *data, char len)
{
    printf("0x%08x | ",reinterpret_cast<unsigned int>(data));
    for(int i=0;i<len;i++) printf("%02x ",static_cast<unsigned char>(data[i]));
    for(int i=0;i<(16-len);i++) printf("   ");
    printf("| ");
    for(int i=0;i<len;i++)
    {
        if((data[i]>=32)&&(data[i]<127)) printf("%c",data[i]);
        else printf(".");
    }
    printf("\n");
}

void memDump(const void *start, int len)
{
    const char *data=reinterpret_cast<const char*>(start);
    while(len>16)
    {
        memPrint(data,16);
        len-=16;
        data+=16;
    }
    if(len>0) memPrint(data,len);
}

} //namespace miosix

#endif


