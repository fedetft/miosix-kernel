
#include <cstdio>
#include <functional>
#include <stdint.h>
#include "e20/e20.h"
#include "miosix.h"
#include "kernel/scheduler/scheduler.h"

using namespace std;
using namespace miosix;

//Connect IR sensor output to PC6 of the stm32f4discovery
typedef Gpio<GPIOC_BASE,6> timerIn;

static unsigned short previous=0;

void start()
{
    previous=0;
    printf("start\n");
}

void timestamp(unsigned short t)
{
    unsigned short delta=t-previous;
    previous=t;
    printf("%d\n",delta);
}

void stop()
{
    printf("stop\n");
}

FixedEventQueue<100,12> queue;

void __attribute__((used)) tim3impl()
{
    bool hppw=false;
    if(TIM3->SR & TIM_SR_UIF)
    {
        queue.IRQpost(bind(stop),hppw);
    } else {
        if(TIM3->CR1 & TIM_CR1_CEN)
        {
            queue.IRQpost(bind(timestamp,TIM3->CCR1),hppw);
        } else {
            TIM3->CR1|=TIM_CR1_CEN; //Start timer at first edge
            queue.IRQpost(bind(start),hppw);
        }
    }
    TIM3->SR=0; //Clear interrupt flag
    if(hppw) Scheduler::IRQfindNextThread();
}

void __attribute__((naked)) TIM3_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z8tim3implv");
    restoreContext();
}

int main()
{
    {
        FastInterruptDisableLock dLock;
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
        RCC_SYNC();
    }
    TIM3->CNT=0;
    TIM3->PSC=84-1; //Prescaler clocked at 84MHz, timer incremented every 1us
    TIM3->ARR=0xffff;
    TIM3->EGR=TIM_EGR_UG; //Update ARR shadow register
    TIM3->SR=0; //Clear interrupt flag caused by setting UG
    TIM3->CCMR1=TIM_CCMR1_CC1S_0;
    TIM3->CCER=TIM_CCER_CC1P
             | TIM_CCER_CC1NP
             | TIM_CCER_CC1E;
    TIM3->DIER=TIM_DIER_CC1IE
             | TIM_DIER_UIE;
    TIM3->CR1=TIM_CR1_OPM; //One pulse mode, timer not started yet
    timerIn::mode(Mode::ALTERNATE);
    timerIn::alternateFunction(2);
    NVIC_SetPriority(TIM3_IRQn,8);
    NVIC_EnableIRQ(TIM3_IRQn);
    queue.run();
}
