#include <stdio.h>
#include <pthread.h>
#include "miosix.h"
#include "kernel/logging.h"
#include "interfaces/cstimer.h"
using namespace std;
using namespace miosix;

static void t1Task(void* p){
    while (true){
        IRQbootlog("1\r\n");
        Thread::sleep(1000);
    }
}

int main(){
    //ContextSwitchTimer::instance();
    Thread::setPriority(1);
    printf("Context Switch Timer ....T=1ms\n");
    Thread *p=Thread::create(t1Task,512,1,NULL);
    while (true){
        IRQbootlog("0\r\n");
        Thread::sleep(250);
    }
}
