
#include "interfaces/arch_registers.h"
#include "core/interrupts.h" //For the unexpected interrupt call
#include "kernel/stage_2_boot.h"
#include <string.h>

/*
 * startup.cpp
 * STM32 C++ startup. 
 * Supports interrupt handlers in C++ without extern "C"
 * Developed by Terraneo Federico, based on ST startup code.
 * Additionally modified to boot Miosix.
 */

void setupClockTree()
{
    //TODO: shouldn't we select voltage range 1 too? Don't have this board so can't test
    static_assert(HSE_VALUE==8000000,"Unsupported HSE oscillator frequency");
    static_assert(SYSCLK_FREQ_32MHz==32000000,"Unsupported target SYSCLK");

    // Check if PLL is used as system clock
    if ((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL)
    {
        // Select HSI as system clock
        RCC->CFGR = (RCC->CFGR & (uint32_t)~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {}
    }
    // Disable PLL
    RCC->CR &= (uint32_t)~RCC_CR_PLLON;
    while(RCC->CR & RCC_CR_PLLRDY) {}

    // Enable HSE with bypass
    RCC->CR |= RCC_CR_HSEON | RCC_CR_HSEBYP;
    while((RCC->CR & RCC_CR_HSERDY) == 0) {}

    // Set flash latency to 1 wait state
    FLASH->ACR |= FLASH_ACR_LATENCY;

    // Set PLL multiplier to 12 (HSI is 8MHz, gives 96MHz) and divider by 3
    // 96MHz is needed for the 48MHz output to work correctly (it is hardcoded
    // to divide by 2)
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PLLMUL | RCC_CFGR_PLLDIV))
                | (RCC_CFGR_PLLMUL12 | RCC_CFGR_PLLDIV3);
    // Set PLL source
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PLLSRC)) | RCC_CFGR_PLLSRC_HSE;
    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {}

    // Set PLL as system clock
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
}

/**
 * Called by Reset_Handler, performs initialization and calls main.
 * Never returns.
 */
void program_startup() __attribute__((noreturn));
void program_startup()
{
    //Cortex M0 core appears to get out of reset with interrupts already enabled
    __disable_irq();

	//SystemInit() is called *before* initializing .data and zeroing .bss
	//Despite all startup files provided by ST do the opposite, there are three
	//good reasons to do so:
	//First, the CMSIS specifications say that SystemInit() must not access
	//global variables, so it is actually possible to call it before
	//Second, when running Miosix with the xram linker scripts .data and .bss
	//are placed in the external RAM, so we *must* call SystemInit(), which
	//enables xram, before touching .data and .bss
	//Third, this is a performance improvement since the loops that initialize
	//.data and zeros .bss now run with the CPU at full speed instead of 8MHz
    
    SystemInit();
    setupClockTree();

	//These are defined in the linker script
	extern unsigned char _etext asm("_etext");
	extern unsigned char _data asm("_data");
	extern unsigned char _edata asm("_edata");
	extern unsigned char _bss_start asm("_bss_start");
	extern unsigned char _bss_end asm("_bss_end");

    //Initialize .data section, clear .bss section
    unsigned char *etext=&_etext;
    unsigned char *data=&_data;
    unsigned char *edata=&_edata;
    unsigned char *bss_start=&_bss_start;
    unsigned char *bss_end=&_bss_end;
    memcpy(data, etext, edata-data);
    memset(bss_start, 0, bss_end-bss_start);

    // Update SystemCoreClock
    SystemCoreClockUpdate();

	//Move on to stage 2
	_init();

	//If main returns, reboot
	NVIC_SystemReset();
    for(;;) ;
}

/**
 * Reset handler, called by hardware immediately after reset
 */
void Reset_Handler() __attribute__((__interrupt__, noreturn));
void Reset_Handler()
{
    /*
     * Initialize process stack and switch to it.
     * This is required for booting Miosix, a small portion of the top of the
     * heap area will be used as stack until the first thread starts. After,
     * this stack will be abandoned and the process stack will point to the
     * current thread's stack.
     */
    asm volatile("ldr r0,  =_heap_end          \n\t"
                 "msr psp, r0                  \n\t"
                 "movs r0, #2                  \n\n" //Privileged, process stack
                 "msr control, r0              \n\t"
                 "isb                          \n\t":::"r0");

    program_startup();
}

/**
 * All unused interrupts call this function.
 */
extern "C" void Default_Handler() 
{
    unexpectedInterrupt();
}

//System handlers
void /*__attribute__((weak))*/ Reset_Handler();     //These interrupts are not
void /*__attribute__((weak))*/ NMI_Handler();       //weak because they are
void /*__attribute__((weak))*/ HardFault_Handler(); //surely defined by Miosix
void /*__attribute__((weak))*/ SVC_Handler();
void /*__attribute__((weak))*/ PendSV_Handler();
void __attribute__((weak)) SysTick_Handler();

// These system handlers are present in Miosix but not supported by the
// architecture, so are defined as weak
// void __attribute__((weak)) MemManage_Handler();
// void __attribute__((weak)) BusFault_Handler();
// void __attribute__((weak)) UsageFault_Handler();
// void __attribute__((weak)) DebugMon_Handler();

//Interrupt handlers
void __attribute__((weak)) WWDG_IRQHandler();
void __attribute__((weak)) PVD_IRQHandler();
void __attribute__((weak)) RTC_IRQHandler();
void __attribute__((weak)) FLASH_IRQHandler();
void __attribute__((weak)) RCC_CRS_IRQHandler();
void __attribute__((weak)) EXTI0_1_IRQHandler();
void __attribute__((weak)) EXTI2_3_IRQHandler();
void __attribute__((weak)) EXTI4_15_IRQHandler();
void __attribute__((weak)) TSC_IRQHandler();
void __attribute__((weak)) DMA1_Channel1_IRQHandler();
void __attribute__((weak)) DMA1_Channel2_3_IRQHandler();
void __attribute__((weak)) DMA1_Channel4_5_6_7_IRQHandler();
void __attribute__((weak)) ADC1_COMP_IRQHandler();
void __attribute__((weak)) LPTIM1_IRQHandler();
void __attribute__((weak)) TIM2_IRQHandler();
void __attribute__((weak)) TIM6_DAC_IRQHandler();
void __attribute__((weak)) TIM21_IRQHandler();
void __attribute__((weak)) TIM22_IRQHandler();
void __attribute__((weak)) I2C1_IRQHandler();
void __attribute__((weak)) I2C2_IRQHandler();
void __attribute__((weak)) SPI1_IRQHandler();
void __attribute__((weak)) SPI2_IRQHandler();
void __attribute__((weak)) USART1_IRQHandler();
void __attribute__((weak)) USART2_IRQHandler();
void __attribute__((weak)) RNG_LPUART1_IRQHandler();
void __attribute__((weak)) LCD_IRQHandler();
void __attribute__((weak)) USB_IRQHandler();

//Stack top, defined in the linker script
extern char _main_stack_top asm("_main_stack_top");

//Interrupt vectors, must be placed @ address 0x00000000
//The extern declaration is required otherwise g++ optimizes it out
extern void (* const __Vectors[])();
void (* const __Vectors[])() __attribute__ ((section(".isr_vector"))) =
{
    reinterpret_cast<void (*)()>(&_main_stack_top),/* Stack pointer*/
    Reset_Handler,              /* Reset Handler */
    NMI_Handler,                /* NMI Handler */
    HardFault_Handler,          /* Hard Fault Handler */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    SVC_Handler,                /* SVCall Handler */
    0,                          /* Reserved */
    0,                          /* Reserved */
    PendSV_Handler,             /* PendSV Handler */
    SysTick_Handler,            /* SysTick Handler */    

    WWDG_IRQHandler,                  /* Window WatchDog              */
    PVD_IRQHandler,                   /* PVD through EXTI Line detect */
    RTC_IRQHandler,                   /* RTC through the EXTI line    */
    FLASH_IRQHandler,                 /* FLASH                        */
    RCC_CRS_IRQHandler,               /* RCC and CRS                  */
    EXTI0_1_IRQHandler,               /* EXTI Line 0 and 1            */
    EXTI2_3_IRQHandler,               /* EXTI Line 2 and 3            */
    EXTI4_15_IRQHandler,              /* EXTI Line 4 to 15            */
    TSC_IRQHandler,                   /* TSC                          */
    DMA1_Channel1_IRQHandler,         /* DMA1 Channel 1               */
    DMA1_Channel2_3_IRQHandler,       /* DMA1 Channel 2 and Channel 3 */
    DMA1_Channel4_5_6_7_IRQHandler,   /* DMA1 Channel 4, Channel 5, Channel 6 and Channel 7*/
    ADC1_COMP_IRQHandler,             /* ADC1, COMP1 and COMP2        */
    LPTIM1_IRQHandler,                /* LPTIM1                       */
    0,                                /* Reserved                     */
    TIM2_IRQHandler,                  /* TIM2                         */
    0,                                /* Reserved                     */
    TIM6_DAC_IRQHandler,              /* TIM6 and DAC                 */
    0,                                /* Reserved                     */
    0,                                /* Reserved                     */
    TIM21_IRQHandler,                 /* TIM21                        */
    0,                                /* Reserved                     */
    TIM22_IRQHandler,                 /* TIM22                        */
    I2C1_IRQHandler,                  /* I2C1                         */
    I2C2_IRQHandler,                  /* I2C2                         */
    SPI1_IRQHandler,                  /* SPI1                         */
    SPI2_IRQHandler,                  /* SPI2                         */
    USART1_IRQHandler,                /* USART1                       */
    USART2_IRQHandler,                /* USART2                       */
    RNG_LPUART1_IRQHandler,           /* RNG and LPUART1              */
    LCD_IRQHandler,                   /* LCD                          */
    USB_IRQHandler                    /* USB                          */
};

#pragma weak SysTick_Handler = Default_Handler
#pragma weak WWDG_IRQHandler = Default_Handler
#pragma weak PVD_IRQHandler = Default_Handler
#pragma weak RTC_IRQHandler = Default_Handler
#pragma weak FLASH_IRQHandler = Default_Handler
#pragma weak RCC_CRS_IRQHandler = Default_Handler
#pragma weak EXTI0_1_IRQHandler = Default_Handler
#pragma weak EXTI2_3_IRQHandler = Default_Handler
#pragma weak EXTI4_15_IRQHandler = Default_Handler
#pragma weak TSC_IRQHandler = Default_Handler
#pragma weak DMA1_Channel1_IRQHandler = Default_Handler
#pragma weak DMA1_Channel2_3_IRQHandler = Default_Handler
#pragma weak DMA1_Channel4_5_6_7_IRQHandler = Default_Handler
#pragma weak ADC1_COMP_IRQHandler = Default_Handler
#pragma weak LPTIM1_IRQHandler = Default_Handler
#pragma weak TIM2_IRQHandler = Default_Handler
#pragma weak TIM6_DAC_IRQHandler = Default_Handler
#pragma weak TIM21_IRQHandler = Default_Handler
#pragma weak TIM22_IRQHandler = Default_Handler
#pragma weak I2C1_IRQHandler = Default_Handler
#pragma weak I2C2_IRQHandler = Default_Handler
#pragma weak SPI1_IRQHandler = Default_Handler
#pragma weak SPI2_IRQHandler = Default_Handler
#pragma weak USART1_IRQHandler = Default_Handler
#pragma weak USART2_IRQHandler = Default_Handler
#pragma weak USART3_4_IRQHandler = Default_Handler
#pragma weak RNG_LPUART1_IRQHandler = Default_Handler
#pragma weak LCD_IRQHandler = Default_Handler
#pragma weak USB_IRQHandler = Default_Handler
