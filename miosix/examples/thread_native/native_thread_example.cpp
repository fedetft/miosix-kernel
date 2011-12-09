
//
// To build this example modify the Makefile so that
// SRC := native_thread_example.cpp
//

#include <stdio.h>
#include <string.h>
#include "miosix.h"

using namespace miosix;

Mutex mutex;
ConditionVariable cond,ack;
char c=0;

void threadfunc(void *argv)
{
    Lock<Mutex> lock(mutex);
    for(int i=0;i<(int)argv;i++)
    {
        ack.signal();
        while(c==0) cond.wait(lock);
        printf("%c",c);
        c=0;
    }
}

int main()
{
    const char str[]="Hello world\n";
    Thread *thread;
    thread=Thread::create(threadfunc,2048,1,(void*)strlen(str),Thread::JOINABLE);
    {
        Lock<Mutex> lock(mutex);
        for(int i=0;i<strlen(str);i++)
        {
            c=str[i];
            cond.signal();
            if(i<strlen(str)-1) ack.wait(lock);
        }
    }
    thread->join();
}
