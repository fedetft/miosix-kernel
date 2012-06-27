
#include <cstdio>
#include <sys/wait.h>
#include <signal.h>
#include "miosix.h"
#include "kernel/process.h"
#include "interfaces/suspend_support.h"
#include "app_template/prog3.h"

using namespace std;
using namespace miosix;

void ledThread(void *)
{
    for(;;)
    {
        ledOn();
        Thread::sleep(200);
        ledOff();
        Thread::sleep(200);
    }
}

//void RTC_WKUP_IRQHandler()
//{
//    RTC->ISR= ~RTC_ISR_WUTF;
//    EXTI->PR = 1<<22;
//    
//    static bool flag=true;
//    if(flag) ledOn();
//    else ledOff();
//    flag=!flag;
//}

int main()
{
    Thread::create(ledThread,STACK_MIN);
//    
//    EXTI->PR = 1<<22;
//    EXTI->IMR |= 1<<22;
//    EXTI->RTSR |= 1<<22;
//    
//    RTC->CR &= ~RTC_CR_WUTE;
//    while((RTC->ISR & RTC_ISR_WUTWF)==0) ; //Wait
//    RTC->WUTR=2048;
//    RTC->CR |= RTC_CR_WUTE;
//    RTC->ISR= ~RTC_ISR_WUTF;
//    
//    NVIC_ClearPendingIRQ(RTC_WKUP_IRQn);
//    NVIC_SetPriority(RTC_WKUP_IRQn,10);
//	  NVIC_EnableIRQ(RTC_WKUP_IRQn);    

//    iprintf("%s\n",firstBoot() ? "First boot" : "RTC boot");
//    getchar();
//    iprintf("Bye\n");
//    doSuspend(15);
    
    ElfProgram prog(reinterpret_cast<const unsigned int*>(main_elf),main_elf_len);
    for(int i=0;;i++)
    {
        getchar();
        pid_t child=Process::create(prog);
        int ec;
        pid_t pid;
        if(i%2==0) pid=Process::wait(&ec);
        else pid=Process::waitpid(child,&ec,0);
        iprintf("Process %d terminated\n",pid);
        if(WIFEXITED(ec))
        {
            iprintf("Exit code is %d\n",WEXITSTATUS(ec));
        } else if(WIFSIGNALED(ec)) {
            if(WTERMSIG(ec)==SIGSEGV) iprintf("Process segfaulted\n");
        }
    }
}
