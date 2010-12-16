
//
// To build this example modify the Makefile so that
// SRC := pthread_example.cpp
//

#include <stdio.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
pthread_cond_t ack=PTHREAD_COND_INITIALIZER;
char c=0;

void *threadfunc(void *argv)
{
    pthread_mutex_lock(&mutex);
    for(int i=0;i<(int)argv;i++)
    {
        pthread_cond_signal(&ack);
        while(c==0) pthread_cond_wait(&cond,&mutex);
        printf("%c",c);
        c=0;
    }
    pthread_mutex_unlock(&mutex);
}

int main()
{
    getchar();
    const char str[]="Hello world\n";
    pthread_t thread;
    pthread_create(&thread,NULL,threadfunc,(void*)strlen(str));
    pthread_mutex_lock(&mutex);
    for(int i=0;i<strlen(str);i++)
    {
        c=str[i];
        pthread_cond_signal(&cond);
        if(i<strlen(str)-1) pthread_cond_wait(&ack,&mutex);
    }
    pthread_mutex_unlock(&mutex);
    pthread_join(thread,NULL);
}
