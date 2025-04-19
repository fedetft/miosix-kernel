#include <miosix.h>
#include <cstdio>
#include <limits>
#include <thread>

#ifndef SCHED_TYPE_EDF
#error Example is meant to be used with EDF scheduler
#endif

using namespace miosix;

void rt_task_func1(void* arg) 
{
    const auto period1=1000000000LL; //4 second
    auto deadline=getTime();
    for(;;)
    {
        deadline+=period1;                 
        Thread::setPriority(deadline);   
        printf("RT1 Task %p running\n", Thread::getCurrentThread());    
        delayMs(450);
        if(getTime()>deadline) printf("Deadline Miss!\n");                   
        Thread::nanoSleepUntil(deadline);
    }
}


void rt_task_func2(void* arg)
{
    const auto period2=1000000000LL; //5 second
    auto deadline=getTime();
    for(;;)
    {
        deadline+=period2;                 
        Thread::setPriority(deadline);   
        printf("RT2 Task %p running\n", Thread::getCurrentThread());    
        delayMs(450);
        if(getTime()>deadline) printf("Deadline Miss!\n");                      
        Thread::nanoSleepUntil(deadline); 
    }
}


void nrt_task_func(void* arg)
{
    for(;;)
    {
        printf("NRT Task %p running\n", Thread::getCurrentThread());    
        delayMs(500);
     
    }
}

int main()
{
    printf("Starting EDF Scheduler Test...\n");

    long long t = getTime();

    long long param = t + 1000000000LL;

    Thread* rt1 = Thread::create(rt_task_func1, 2048, param, nullptr);
    Thread* rt2 = Thread::create(rt_task_func2, 2048, param, nullptr);

    Thread* nrt1 = Thread::create(nrt_task_func, 2048, DEFAULT_PRIORITY, nullptr);
    Thread* nrt2 = Thread::create(nrt_task_func, 2048, DEFAULT_PRIORITY, nullptr);
    
    printf("Threads created. Running test...\n");

    for(;;) Thread::wait();
}
