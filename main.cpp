#include <cstdio>
#include <cmath>
#include "miosix.h"
#include "kernel/logging.h"
#include "e20/e20.h"
#include "cstimer.h"
using namespace std;
using namespace miosix;

void basicTests(){
    ContextSwitchTimer t = ContextSwitchTimer::instance();
    long long chkpoint = 0xFFFFFFFFll+1899760;
    //chkpoint = 0xFFFFFFFFll-20; //Test Case: CHKPNT right really before rollover -> OK
    //chkpoint = 0xFFFFFFFFll-2000; //Test Case: CHKPNT right before rollover -> OK
    //chkpoint = 0xFFFFFFFFll+2000; //Test Case: CHKPNT right afrer rollover -> OK
    //chkpoint = 0xFFFFFFFFll+20; //Test Case: CHKPNT right really afrer rollover -> OK
    t.setNextInterrupt(chkpoint); //checkpoint near the rollover
    while(1){
        Thread::sleep(10000);
        printf("-CNT: %llu , NEXT CHKPNT: %llu\n",t.getCurrentTick(),t.getNextInterrupt());

    }
}
void rightAfterCurrentTime(){
    ContextSwitchTimer t = ContextSwitchTimer::instance();

    while(1){
        Thread::sleep(10000);
        //checkpoint near the current tick
        //A chkpoint with dist. less than 80 is not set in time => lost!
        t.setNextInterrupt(t.getCurrentTick()+80);
        printf("-CNT: %llu , NEXT CHKPNT: %llu\n",t.getCurrentTick(),t.getNextInterrupt());

    }
}

int main(){
    iprintf("Context Switch Timer Driver Test1 (80)\n");

    //basicTests(); //passed
    rightAfterCurrentTime();
}
