//
// NOTE: this file contains some modifications by Silvano Seva (silseva) to make
// available system clock frequencies greater than 8MHz
//

/**
  ******************************************************************************
  * @file    system_stm32f3xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M4 Device Peripheral Access Layer System Source File.
  *
  * 1. This file provides two functions and one global variable to be called from
  *    user application:
  *      - SystemInit(): This function is called at startup just after reset and 
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32f3xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
  *                                  by the user application to setup the SysTick
  *                                  timer or configure other parameters.
  *
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  * 2. After each device reset the HSI (8 MHz) is used as system clock source.
  *    Then SystemInit() function is called, in "startup_stm32f3xx.s" file, to
  *    configure the system clock before to branch to main program.
  *
  * 3. This file configures the system clock as follows:
  *=============================================================================
  *                         Supported STM32F3xx device
  *-----------------------------------------------------------------------------
  *        System Clock source                    | HSI
  *-----------------------------------------------------------------------------
  *        SYSCLK(Hz)                             | 8000000
  *-----------------------------------------------------------------------------
  *        HCLK(Hz)                               | 8000000
  *-----------------------------------------------------------------------------
  *        AHB Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        APB2 Prescaler                         | 1
  *-----------------------------------------------------------------------------
  *        APB1 Prescaler                         | 1
  *-----------------------------------------------------------------------------
  *        USB Clock                              | DISABLE
  *-----------------------------------------------------------------------------
  *=============================================================================
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32f3xx_system
  * @{
  */

/** @addtogroup STM32F3xx_System_Private_Includes
  * @{
  */
#include "board_settings.h"
//By Silvano Seva: was #include "stm32f3xx.h"
#include "interfaces/arch_registers.h"

/**
  * @}
  */

/** @addtogroup STM32F3xx_System_Private_TypesDefinitions
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32F3xx_System_Private_Defines
  * @{
  */

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    ((uint32_t)8000000) /*!< Default value of the Internal oscillator in Hz.
                                                This value can be provided and adapted by the user application. */
#endif /* HSI_VALUE */

// Added by silseva
#define HSE_STARTUP_TIMEOUT   ((uint16_t)0x0500)

/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x0 /*!< Vector Table base offset field.
                                  This value must be a multiple of 0x200. */
/**
  * @}
  */

/** @addtogroup STM32F3xx_System_Private_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32F3xx_System_Private_Variables
  * @{
  */
  /* This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetHCLKFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
         Note: If you use this function to configure the system clock there is no need to
               call the 2 first functions listed above, since SystemCoreClock variable is 
               updated automatically.
  */

uint32_t SystemCoreClock = miosix::cpuFrequency; /*!< System Clock Frequency (Core Clock) */

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};

/**
  * @}
  */

/** @addtogroup STM32F3xx_System_Private_FunctionPrototypes
  * @{
  */

// Added by silseva
static void SetSysClock(void);

/**
  * @}
  */

/** @addtogroup STM32F3xx_System_Private_Functions
  * @{
  */

/**
  * @brief  Setup the microcontroller system
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
    // Added by silseva
    /* Reset the RCC clock configuration to the default reset state(for debug purpose) */
    /* Set HSION bit */
    RCC->CR |= (uint32_t)0x00000001;

    /* Reset all RCC_CFGR register */
    RCC->CFGR = (uint32_t)0x00000000;

    /* Reset HSEON, CSSON and PLLON bits */
    RCC->CR &= (uint32_t)0xFEF6FFFF;

    /* Reset HSEBYP bit */
    RCC->CR &= (uint32_t)0xFFFBFFFF;

    /* Disable all interrupts and clear pending bits  */
    RCC->CIR = 0x009F0000;

    /* Clear PREDIV bits */
    RCC->CFGR2 &= ~RCC_CFGR2_PREDIV;

    /* Configure the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers */
    /* Configure the Flash Latency cycles and enable prefetch buffer */
    SetSysClock();
    // end addition

/* FPU settings --------------------------------------------------------------*/
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
#endif

#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}

/**
   * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.
  *
  * @note   - The system frequency computed by this function is not the real
  *           frequency in the chip. It is calculated based on the predefined
  *           constant and the selected clock source:
  *
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(*)
  *
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(**)
  *
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(**)
  *             or HSI_VALUE(*) multiplied/divided by the PLL factors.
  *
  *         (*) HSI_VALUE is a constant defined in stm32f3xx_hal.h file (default value
  *             8 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.
  *
  *         (**) HSE_VALUE is a constant defined in stm32f3xx_hal.h file (default value
  *              8 MHz), user has to ensure that HSE_VALUE is same as the real
  *              frequency of the crystal used. Otherwise, this function may
  *              have wrong result.
  *
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  *
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate (void)
{
  uint32_t tmp = 0, pllmull = 0, pllsource = 0, predivfactor = 0;

  /* Get SYSCLK source -------------------------------------------------------*/
  tmp = RCC->CFGR & RCC_CFGR_SWS;

  switch (tmp)
  {
    case RCC_CFGR_SWS_HSI:  /* HSI used as system clock */
      SystemCoreClock = HSI_VALUE;
      break;
    case RCC_CFGR_SWS_HSE:  /* HSE used as system clock */
      SystemCoreClock = miosix::hseFrequency;
      break;
    case RCC_CFGR_SWS_PLL:  /* PLL used as system clock */
      /* Get PLL clock source and multiplication factor ----------------------*/
      pllmull = RCC->CFGR & RCC_CFGR_PLLMUL;
      pllsource = RCC->CFGR & RCC_CFGR_PLLSRC;
      pllmull = ( pllmull >> 18) + 2;

#if defined (STM32F302xE) || defined (STM32F303xE) || defined (STM32F398xx)
        predivfactor = (RCC->CFGR2 & RCC_CFGR2_PREDIV) + 1;
      if (pllsource == RCC_CFGR_PLLSRC_HSE_PREDIV)
      {
        /* HSE oscillator clock selected as PREDIV1 clock entry */
        SystemCoreClock = (miosix::hseFrequency / predivfactor) * pllmull;
      }
      else
      {
        /* HSI oscillator clock selected as PREDIV1 clock entry */
        SystemCoreClock = (HSI_VALUE / predivfactor) * pllmull;
      }
#else      
      if (pllsource == RCC_CFGR_PLLSRC_HSI_DIV2)
      {
        /* HSI oscillator clock divided by 2 selected as PLL clock entry */
        SystemCoreClock = (HSI_VALUE >> 1) * pllmull;
      }
      else
      {
        predivfactor = (RCC->CFGR2 & RCC_CFGR2_PREDIV) + 1;
        /* HSE oscillator clock selected as PREDIV1 clock entry */
        SystemCoreClock = (miosix::hseFrequency / predivfactor) * pllmull;
      }
#endif /* STM32F302xE || STM32F303xE || STM32F398xx */
      break;
    default: /* HSI used as system clock */
      SystemCoreClock = HSI_VALUE;
      break;
  }
  /* Compute HCLK clock frequency ----------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
  /* HCLK clock frequency */
  SystemCoreClock >>= tmp;
}

// Added by silseva
/**
  * @brief  Configures the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers.
  * @param  None
  * @retval None
  */
void SetSysClock(void)
{
  uint32_t StartUpCounter = 0;
  (void)StartUpCounter;

  static_assert(miosix::oscillatorType==miosix::OscillatorType::HSE
                || miosix::oscillatorType==miosix::OscillatorType::HSI,
                "Oscillator type unsupported");
  static_assert(!(miosix::oscillatorType==miosix::OscillatorType::HSI
                  && miosix::cpuFrequency>56000000),
                "sysclk is capped to 56MHz when running on HSI");
  static_assert(miosix::oscillatorType!=miosix::OscillatorType::HSE
                || miosix::hseFrequency>1000000,
                "Unlikely HSE frequency");
  constexpr unsigned int oscFreq=
      miosix::oscillatorType==miosix::OscillatorType::HSE ?
        miosix::hseFrequency : HSI_VALUE;
  if constexpr(miosix::oscillatorType==miosix::OscillatorType::HSE)
  {
    /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/
    /* Enable HSE */
    RCC->CR |= ((uint32_t)RCC_CR_HSEON);

    /* Wait till HSE is ready and if Time out is reached exit */
    unsigned int HSEStatus;
    do
    {
      HSEStatus = RCC->CR & RCC_CR_HSERDY;
      StartUpCounter++;  
    } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

    // If HSE failed to start up do nothing and return
    if ((RCC->CR & RCC_CR_HSERDY) == 0) return;
  }

  /* Enable Prefetch Buffer */
  FLASH->ACR |= FLASH_ACR_PRFTBE;

  // Configure Flash wait states
  unsigned int flashLatency;
  if(miosix::cpuFrequency<=24000000) flashLatency=0;
  else if(miosix::cpuFrequency<=48000000) flashLatency=1;
  else flashLatency=2;
  //NOTE: the value of the constant FLASH_ACR_LATENCY_0 changed from meaning
  //"0 wait states" to meaning "bit 0", thus 1 wait states, so don't use it!
  FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | (flashLatency<<FLASH_ACR_LATENCY_Pos);

  /* HCLK = SYSCLK */
  RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
    
  /* PCLK2 = HCLK */
  RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
  
  /* PCLK1 = HCLK or HCLK/2 (must not exceed 36MHz) */
  if(miosix::cpuFrequency<=36000000) RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
  else RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;

  if(oscFreq==miosix::cpuFrequency && miosix::oscillatorType==miosix::OscillatorType::HSE)
  {
    /* Select HSE as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_HSE;    

    /* Wait till HSE is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x04)
    {
    }
    return;
  } else if(oscFreq==miosix::cpuFrequency && miosix::oscillatorType==miosix::OscillatorType::HSI) {
    // Nothing to do here, it's the default after reset
    return;
  }
  // Otherwise we need to configure the PLL
  
  constexpr unsigned int div=miosix::cpuFrequency<=36000000 ? 2:1;
  #if defined (STM32F302xE) || defined (STM32F303xE) || defined (STM32F398xx)
  // PREDIV is after PLLSRC mux
  RCC->CFGR2&=~RCC_CFGR2_PREDIV_Msk;
  RCC->CFGR2|=(div-1)<<RCC_CFGR2_PREDIV_Pos;
  constexpr unsigned int pllInput=miosix::oscFreq/div;
  #else
  // PREDIV is only on HSE (HSI is always divided by 2)
  constexpr unsigned int pllInput=
    miosix::oscillatorType==miosix::OscillatorType::HSI ? HSI_VALUE/2 :
                                             miosix::hseFrequency/div;
  if constexpr(miosix::oscillatorType==miosix::OscillatorType::HSE)
  {
    RCC->CFGR2&=~RCC_CFGR2_PREDIV_Msk;
    RCC->CFGR2|=(div-1)<<RCC_CFGR2_PREDIV_Pos;
  }
  #endif
  constexpr unsigned int pllmul=miosix::cpuFrequency/pllInput;
  static_assert(pllmul>=2 && pllmul<=16,"Unsupported sysclk");
  RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMUL));
  RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC | ((pllmul-2)<<RCC_CFGR_PLLMUL_Pos));

  //By TFT
  if(miosix::oscillatorType==miosix::OscillatorType::HSI) RCC->CFGR &= ~RCC_CFGR_PLLSRC;

  /* Enable PLL */
  RCC->CR |= RCC_CR_PLLON;

  /* Wait till PLL is ready */
  while((RCC->CR & RCC_CR_PLLRDY) == 0)
  {
  }

  /* Select PLL as system clock source */
  RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
  RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

  /* Wait till PLL is used as system clock source */
  while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08)
  {
  }
}

// end addition

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

