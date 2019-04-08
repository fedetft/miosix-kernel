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

// bool PowerManagement::isUsbConnected() const
// {
//     return usb::USB5V_Detected_Pin::value();
// }

// bool PowerManagement::isCharging()
// {
//     if(isUsbConnected()==false) return false;
//     Lock<FastMutex> l(i2cMutex());
//     unsigned char chgstatus;
//     //During testing the i2c command never failed. If it does, we lie and say
//     //we're not charging
//     if(i2cReadReg(i2c,PMU_I2C_ADDRESS,CHGSTATUS,chgstatus)==false) return false;
//     return (chgstatus & CH_ACTIVE_MSK)!=0;
// }

// int PowerManagement::getBatteryStatus()
// {
//     const int battCharged=4000; //4.0V
//     const int battDead=3000; //3V
//     return max(0,min(100,(getBatteryVoltage()-battDead)*100/(battCharged-battDead)));
// }

// int PowerManagement::getBatteryVoltage()
// {
//     Lock<FastMutex> l(powerManagementMutex);
//     power::BATT_V_ON_Pin::high(); //Enable battry measure circuitry
//     ADC1->CR2=ADC_CR2_ADON; //Turn ADC ON
//     Thread::sleep(5); //Wait for voltage to stabilize
//     ADC1->CR2 |= ADC_CR2_SWSTART; //Start conversion
//     while((ADC1->SR & ADC_SR_EOC)==0) ; //Wait for conversion
//     int result=ADC1->DR; //Read result
//     ADC1->CR2=0; //Turn ADC OFF
//     power::BATT_V_ON_Pin::low(); //Disable battery measurement circuitry
//     return result*4440/4095;
// }

// void PowerManagement::setCoreFrequency(CoreFrequency cf)
// {
//     if(cf==coreFreq) return;
    
//     Lock<FastMutex> l(powerManagementMutex);
//     //We need to reconfigure I2C for the new frequency 
//     Lock<FastMutex> l2(i2cMutex());
    
//     {
//         FastInterruptDisableLock dLock;
//         CoreFrequency oldCoreFreq=coreFreq;
//         coreFreq=cf; //Need to change this *before* setting prescalers/core freq
//         if(coreFreq>oldCoreFreq)
//         {
//             //We're increasing the frequency, so change prescalers first
//             IRQsetPrescalers();
//             IRQsetCoreFreq();
//         } else {
//             //We're decreasing the frequency, so change frequency first
//             IRQsetCoreFreq();
//             IRQsetPrescalers();
//         }
        
//         //Changing frequency requires to change many things that depend on
//         //said frequency:
        
//         //Miosix's preemption tick
//         SystemCoreClockUpdate();
//         SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
//         SysTick->LOAD=SystemCoreClock/miosix::TICK_FREQ;
//         SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
        
//         //Miosix's auxiliary timer (if required)
//         #ifdef SCHED_TYPE_CONTROL_BASED
//         int timerClock=SystemCoreClock;
//         int apb1prescaler=(RCC->CFGR>>10) & 7;
//         if(apb1prescaler>4) timerClock>>=(apb1prescaler-4);
//         TIM3->CR1 &= ~TIM_CR1_CEN; //Stop timer
//         TIM3->PSC=(timerClock/miosix::AUX_TIMER_CLOCK)-1;
//         TIM3->CR1 |= TIM_CR1_CEN; //Start timer
//         #endif //SCHED_TYPE_CONTROL_BASED
//     }
    
// }

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
  IRQsetSystemClock();
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
      __WFE();
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
    // ADC1->CR1=0;
    // ADC1->CR2=0; //Keep the ADC OFF to save power
    // ADC1->SMPR2=ADC_SMPR2_SMP2_0; //Sample for 15 cycles channel 2 (battery)
    // ADC1->SQR1=0; //Do only one conversion
    // ADC1->SQR2=0;
    // ADC1->SQR3=2; //Convert channel 2 (battery voltage)
    
    // unsigned char config0=VSYS_4_4V
    //                     | ACIC_100mA_DPPM_ENABLE
    //                     | TH_LOOP
    //                     | DYN_TMR
    //                     | TERM_EN
    //                     | CH_EN;
    // unsigned char config1=I_TERM_10
    //                     | ISET_100
    //                     | I_PRE_10;
    // unsigned char config2=SFTY_TMR_5h
    //                     | PRE_TMR_30m
    //                     | NTC_10k
    //                     | V_DPPM_4_3_V
    //                     | VBAT_COMP_ENABLE;
    // unsigned char defdcdc=DCDC_DISCH
    //                     | DCDC1_DEFAULT;
    // bool error=false;
    // if(error) errorMarker(10); //Should never happen
    IRQsetSystemClock();
}

