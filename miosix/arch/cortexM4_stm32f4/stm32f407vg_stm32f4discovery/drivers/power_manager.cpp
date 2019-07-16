#include <miosix.h>
#include <sys/ioctl.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/kernel.h>

#include "power_manager.h"
#include "rtc.h"


#define PLL_M (HSE_VALUE/1000000)
#define PLL_N      336
#define PLL_P      2
#define PLL_Q      7

using namespace miosix;


void IRQpowerManagementInit() {
  PowerManagement::instance();
}

PowerManagement& PowerManagement::instance()
{
    static PowerManagement singleton;
    return singleton;
}


void PowerManagement::IRQgoDeepSleepFor(unsigned long long int ns)
{
  // From RTC	
  Rtc& rtc = Rtc::instance();
  rtc.setWakeupInterrupt();
  long long int wut_ticks = rtc.wkp_tc.ns2tick(ns);
  unsigned int remaining_wakeups = wut_ticks / 0xffff;
  unsigned int last_wut = wut_ticks % 0xffff;
  while (remaining_wakeups > 0 )
    {
      rtc.setWakeupTimer(0xffff);
      IRQenterStopMode();
      remaining_wakeups--;
    }
  rtc.setWakeupTimer(last_wut);
  IRQenterStopMode();
}

void PowerManagement::IRQenterStopMode()
{
  Rtc& rtc = Rtc::instance();
      // Safe procedure to enter stopmode
      RTC->CR &= ~(RTC_CR_WUTIE);
      RTC->ISR &= ~(RTC_ISR_WUTF);
      PWR->CSR &= ~(PWR_CSR_WUF);
      EXTI->PR = 0xffff; // reset pending bits
      RTC->CR |= RTC_CR_WUTIE;
      rtc.startWakeupTimer(); 
      //Enter stop mode by issuing a WFE
      PWR->CR |= PWR_CR_FPDS  //Flash in power down while in stop
      	| PWR_CR_LPDS; //Regulator in low power mode while in stop
      SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; //Select stop mode
      __SEV();
      __WFE();
      __WFE(); // fast fix to a bug of STM32F4
      SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; //Unselect stop mode
      IRQsetSystemClock();
      //Configure PLL and turn it on
      rtc.stopWakeupTimer();
}

void PowerManagement::IRQsetSystemClock()
{
  RCC->CR |= RCC_CR_HSEON;
 
  /* Wait till HSE is ready and if Time out is reached exit */
  while(((RCC->CR & RCC_CR_HSERDY) == 0));

  /* Select regulator voltage output Scale 1 mode, System frequency up to 168 MHz */
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  RCC_SYNC();
  PWR->CR |= PWR_CR_VOS;

  /* HCLK = SYSCLK / 1*/
  RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
      
  /* PCLK2 = HCLK / 2*/
  RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;
    
  /* PCLK1 = HCLK / 4*/
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

  /* Configure the main PLL */
  RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) -1) << 16) |
    (RCC_PLLCFGR_PLLSRC_HSE) | (PLL_Q << 24);

  /* Enable the main PLL */
  RCC->CR |= RCC_CR_PLLON;

  /* Wait till the main PLL is ready */
  while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }
   
  /* Configure Flash prefetch, Instruction cache, Data cache and wait state */
  FLASH->ACR = FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;

  /* Select the main PLL as system clock source */
  RCC->CFGR &= ~(RCC_CFGR_SW);
  RCC->CFGR |= RCC_CFGR_SW_PLL;

  /* Wait till the main PLL is used as system clock source */
  while ((RCC->CFGR & RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL);
  {
  }

}

PowerManagement::PowerManagement() : 
        chargingAllowed(true), wakeOnButton(false), coreFreq(FREQ_120MHz),
         powerManagementMutex(FastMutex::RECURSIVE)
{
    {
        FastInterruptDisableLock dLock;
        RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_SYSCFGEN;
        RCC_SYNC();
        //Configure PB1 (POWER_BUTTON) as EXTI input
        SYSCFG->EXTICR[2] &= ~(0xf<<12);
        SYSCFG->EXTICR[2] |= 1<<12;
        //Then disable SYSCFG access, as we don't need it anymore
        RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;
    }
    // IRQsetSystemClock();
}

