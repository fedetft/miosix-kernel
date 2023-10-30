#include <string.h>

#include "core/cache_cortexMx.h"
#include "core/interrupts.h"  //For the unexpected interrupt call
#include "core/interrupts_cortexMx.h"
#include "interfaces/arch_registers.h"
#include "interfaces/bsp.h"
#include "kernel/stage_2_boot.h"
#include "interfaces/delays.h"

/*
 * startup.cpp
 * STM32 C++ startup.
 * NOTE: for stm32f767 devices ONLY.
 * Supports interrupt handlers in C++ without extern "C"
 * Developed by Terraneo Federico, based on ST startup code.
 * Additionally modified to boot Miosix.
 */

void configureSDRAM()
{
    // Enable all gpios used by the FMC
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN |
                    RCC_AHB1ENR_GPIOFEN | RCC_AHB1ENR_GPIOGEN |
                    RCC_AHB1ENR_GPIOHEN | RCC_AHB1ENR_GPIOIEN;
    RCC_SYNC();
    // Set FMC GPIO speed to 100MHz
    //            port  F  E  D  C  B  A  9  8  7  6  5  4  3  2  1  0
    GPIOD->OSPEEDR = 0b11'11'00'00'00'11'11'11'00'00'00'00'00'00'11'11;
    GPIOE->OSPEEDR = 0b11'11'11'11'11'11'11'11'11'00'00'00'00'00'11'11;
    GPIOF->OSPEEDR = 0b11'11'11'11'11'00'00'00'00'00'11'11'11'11'11'11;
    GPIOG->OSPEEDR = 0b11'00'00'00'00'00'00'11'00'00'11'11'00'11'11'11;
    GPIOH->OSPEEDR = 0b11'11'11'11'11'11'11'11'11'11'11'00'11'11'00'10;
    GPIOI->OSPEEDR = 0b00'00'00'00'00'00'11'00'11'11'11'11'11'11'11'11; // FIXME PI10 should have been D31 but is wired to LED0
    // Set FMC GPIO to alternate mode
    //            port  F  E  D  C  B  A  9  8  7  6  5  4  3  2  1  0
    GPIOD->MODER   = 0b10'10'00'00'00'10'10'10'00'00'00'00'00'00'10'10;
    GPIOE->MODER   = 0b10'10'10'10'10'10'10'10'10'00'00'00'00'00'10'10;
    GPIOF->MODER   = 0b10'10'10'10'10'00'00'00'00'00'10'10'10'10'10'10;
    GPIOG->MODER   = 0b10'00'00'00'00'00'00'10'00'00'10'10'00'10'10'10;
    GPIOH->MODER   = 0b10'10'10'10'10'10'10'10'10'10'10'00'10'10'00'10;
    GPIOI->MODER   = 0b00'00'00'00'00'00'10'00'10'10'10'10'10'10'10'10; // FIXME PI10 should have been D31 but is wired to LED0
    // No need to update PUPD as the default of zero is already correct.
    // Set FMC GPIO alternate modes
    //         port   FEDCBA98                    76543210
    GPIOD->AFR[1] = 0xcc000ccc; GPIOD->AFR[0] = 0x000000cc;
    GPIOE->AFR[1] = 0xcccccccc; GPIOE->AFR[0] = 0xc00000cc;
    GPIOF->AFR[1] = 0xccccc000; GPIOF->AFR[0] = 0x00cccccc;
    GPIOG->AFR[1] = 0xc000000c; GPIOG->AFR[0] = 0x00cc0ccc;
    GPIOH->AFR[1] = 0xcccccccc; GPIOH->AFR[0] = 0xccc0cc0c;
    GPIOI->AFR[1] = 0x000000c0; GPIOI->AFR[0] = 0xcccccccc;  // FIXME PI10 should have been D31 but is wired to LED0

    // Power on FMC
    RCC->AHB3ENR = RCC_AHB3ENR_FMCEN;
    RCC_SYNC();
    // Program memory device features and timings for 8 paralleled
    // IS42S86400D-7TLI chips
    uint32_t sdcr =
          (3 << FMC_SDCR1_NC_Pos)       // 11 column address bits
        | (2 << FMC_SDCR1_NR_Pos)       // 13 row address bits
        | (2 << FMC_SDCR1_MWID_Pos)     // 32 bit data bus
        | (1 << FMC_SDCR1_NB_Pos)       // 4 internal banks
        | (2 << FMC_SDCR1_CAS_Pos)      // 2 cycle CAS latency
        | (0 << FMC_SDCR1_WP_Pos)       // write allowed
        | (3 << FMC_SDCR1_SDCLK_Pos)    // HCLK / 3 clock (FIXME: conservative)
        | (0 << FMC_SDCR1_RBURST_Pos)   // no burst mode on single read requests (FIXME: conservative)
        | (0 << FMC_SDCR1_RPIPE_Pos);   // 0 delay after reading
    FMC_Bank5_6->SDCR[0] = sdcr;
    FMC_Bank5_6->SDCR[1] = sdcr;
    // FIXME: these timings are copied from stm32f769ni_discovery!
    // To be fair it's the same brand of ram and a similar type but they need to
    // be checked. ATM they probably are conservative because we are running
    // the RAM at a slower clock (HCLK/3 rather than HCLK/2)
    uint32_t sdtr =
          (2 - 1) << FMC_SDTR1_TRCD_Pos   // 2 cycles TRCD (18.52ns > 18ns)
        | (2 - 1) << FMC_SDTR1_TRP_Pos    // 2 cycles TRP  (18.52ns > 18ns)
        | (2 - 1) << FMC_SDTR1_TWR_Pos    // 2 cycles TWR  (18.52ns > 12ns)
        | (7 - 1) << FMC_SDTR1_TRC_Pos    // 7 cycles TRC  (64.82ns > 60ns)
        | (5 - 1) << FMC_SDTR1_TRAS_Pos   // 5 cycles TRAS (46.3ns  > 42ns)
        | (8 - 1) << FMC_SDTR1_TXSR_Pos   // 8 cycles TXSR (74.08ns > 70ns)
        | (2 - 1) << FMC_SDTR1_TMRD_Pos;  // 2 cycles TMRD (18.52ns > 12ns)
    FMC_Bank5_6->SDTR[0] = sdtr;
    FMC_Bank5_6->SDTR[1] = sdtr;
    // Send init command and wait for powerup
    FMC_Bank5_6->SDCMR = (1 << FMC_SDCMR_MODE_Pos) | FMC_SDCMR_CTB1 | FMC_SDCMR_CTB2;
    while (FMC_Bank5_6->SDSR & FMC_SDSR_BUSY) ;
    miosix::delayUs(200); // see RAM datasheet p.21 for wait time
    // Precharge all banks
    FMC_Bank5_6->SDCMR = (2 << FMC_SDCMR_MODE_Pos) | FMC_SDCMR_CTB1 | FMC_SDCMR_CTB2;
    while (FMC_Bank5_6->SDSR & FMC_SDSR_BUSY) ;
    // Issue 8 auto-refresh commands (see RAM datasheet p.21 for number of refreshes)
    FMC_Bank5_6->SDCMR = (3 << FMC_SDCMR_MODE_Pos) | (7 << FMC_SDCMR_NRFS_Pos) | FMC_SDCMR_CTB1 | FMC_SDCMR_CTB2;
    while (FMC_Bank5_6->SDSR & FMC_SDSR_BUSY) ;
    // Issue mode register set command
    uint32_t mrd = 0b1'00'010'0'000; // TODO: check bit 9, "Write Burst Mode", stm32f769 sets it to 1 but what controls this on STM32 side?
    FMC_Bank5_6->SDCMR = (4 << FMC_SDCMR_MODE_Pos) | (mrd << FMC_SDCMR_MRD_Pos) | FMC_SDCMR_CTB1 | FMC_SDCMR_CTB2;
    while (FMC_Bank5_6->SDSR & FMC_SDSR_BUSY) ;
    // Program refresh rate
    FMC_Bank5_6->SDRTR = 300 << FMC_SDRTR_COUNT_Pos; // TODO: conservative
}

/**
 * Called by Reset_Handler, performs initialization and calls main.
 * Never returns.
 */
void program_startup() __attribute__((noreturn));
void program_startup()
{
    //Cortex M7 core appears to get out of reset with interrupts already enabled
    __disable_irq();

    /**
     * SystemInit() is called *before* initializing .data and zeroing .bss
     * Despite all startup files provided by ST do the opposite, there are three
     * good reasons to do so:
     * 1. First, the CMSIS specifications say that SystemInit() must not access
     *    global variables, so it is actually possible to call it before
     * 2. Second, when running Miosix with the xram linker scripts .data and
     *    .bss are placed in the external RAM, so we *must* call SystemInit(),
     *    which enables xram, before touching .data and .bss
     * 3. Third, this is a performance improvement since the loops that
     *    initialize .data and zeros .bss now run with the CPU at full speed
     *    instead of 8MHz
     */
    SystemInit();

    configureSDRAM();

    miosix::IRQconfigureCache();

    // These are defined in the linker script
    extern unsigned char _etext asm("_etext");
    extern unsigned char _data asm("_data");
    extern unsigned char _edata asm("_edata");
    extern unsigned char _bss_start asm("_bss_start");
    extern unsigned char _bss_end asm("_bss_end");

    // Initialize .data section, clear .bss section
    unsigned char *etext=&_etext;
    unsigned char *data=&_data;
    unsigned char *edata=&_edata;
    unsigned char *bss_start=&_bss_start;
    unsigned char *bss_end=&_bss_end;
    memcpy(data, etext, edata-data);
    memset(bss_start, 0, bss_end-bss_start);

    // Move on to stage 2
    _init();

    // If main returns, reboot
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
     * Load into the program stack pointer the heap end address and switch from
     * the msp to sps.
     * This is required for booting Miosix, a small portion of the top of the
     * heap area will be used as stack until the first thread starts. After,
     * this stack will be abandoned and the process stack will point to the
     * current thread's stack.
     */
    asm volatile(
        "ldr r0,  =_heap_end          \n\t"
        "msr psp, r0                  \n\t"
        "movw r0, #2                  \n\n"  // Set the control register to use
        "msr control, r0              \n\t"  // the process stack
        "isb                          \n\t":::"r0");

    program_startup();
}

/**
 * All unused interrupts call this function.
 */
extern "C" void Default_Handler() { unexpectedInterrupt(); }

// System handlers
void /*__attribute__((weak))*/ Reset_Handler();      // These interrupts are not
void /*__attribute__((weak))*/ NMI_Handler();        // weak because they are
void /*__attribute__((weak))*/ HardFault_Handler();  // surely defined by Miosix
void /*__attribute__((weak))*/ MemManage_Handler();
void /*__attribute__((weak))*/ BusFault_Handler();
void /*__attribute__((weak))*/ UsageFault_Handler();
void /*__attribute__((weak))*/ SVC_Handler();
void /*__attribute__((weak))*/ DebugMon_Handler();
void /*__attribute__((weak))*/ PendSV_Handler();
void __attribute__((weak)) SysTick_Handler();

// Interrupt handlers
void __attribute__((weak)) WWDG_IRQHandler();
void __attribute__((weak)) PVD_IRQHandler();
void __attribute__((weak)) TAMP_STAMP_IRQHandler();
void __attribute__((weak)) RTC_WKUP_IRQHandler();
void __attribute__((weak)) FLASH_IRQHandler();
void __attribute__((weak)) RCC_IRQHandler();
void __attribute__((weak)) EXTI0_IRQHandler();
void __attribute__((weak)) EXTI1_IRQHandler();
void __attribute__((weak)) EXTI2_IRQHandler();
void __attribute__((weak)) EXTI3_IRQHandler();
void __attribute__((weak)) EXTI4_IRQHandler();
void __attribute__((weak)) DMA1_Stream0_IRQHandler();
void __attribute__((weak)) DMA1_Stream1_IRQHandler();
void __attribute__((weak)) DMA1_Stream2_IRQHandler();
void __attribute__((weak)) DMA1_Stream3_IRQHandler();
void __attribute__((weak)) DMA1_Stream4_IRQHandler();
void __attribute__((weak)) DMA1_Stream5_IRQHandler();
void __attribute__((weak)) DMA1_Stream6_IRQHandler();
void __attribute__((weak)) ADC_IRQHandler();
void __attribute__((weak)) CAN1_TX_IRQHandler();
void __attribute__((weak)) CAN1_RX0_IRQHandler();
void __attribute__((weak)) CAN1_RX1_IRQHandler();
void __attribute__((weak)) CAN1_SCE_IRQHandler();
void __attribute__((weak)) EXTI9_5_IRQHandler();
void __attribute__((weak)) TIM1_BRK_TIM9_IRQHandler();
void __attribute__((weak)) TIM1_UP_TIM10_IRQHandler();
void __attribute__((weak)) TIM1_TRG_COM_TIM11_IRQHandler();
void __attribute__((weak)) TIM1_CC_IRQHandler();
void __attribute__((weak)) TIM2_IRQHandler();
void __attribute__((weak)) TIM3_IRQHandler();
void __attribute__((weak)) TIM4_IRQHandler();
void __attribute__((weak)) I2C1_EV_IRQHandler();
void __attribute__((weak)) I2C1_ER_IRQHandler();
void __attribute__((weak)) I2C2_EV_IRQHandler();
void __attribute__((weak)) I2C2_ER_IRQHandler();
void __attribute__((weak)) SPI1_IRQHandler();
void __attribute__((weak)) SPI2_IRQHandler();
void __attribute__((weak)) USART1_IRQHandler();
void __attribute__((weak)) USART2_IRQHandler();
void __attribute__((weak)) USART3_IRQHandler();
void __attribute__((weak)) EXTI15_10_IRQHandler();
void __attribute__((weak)) RTC_Alarm_IRQHandler();
void __attribute__((weak)) OTG_FS_WKUP_IRQHandler();
void __attribute__((weak)) TIM8_BRK_TIM12_IRQHandler();
void __attribute__((weak)) TIM8_UP_TIM13_IRQHandler();
void __attribute__((weak)) TIM8_TRG_COM_TIM14_IRQHandler();
void __attribute__((weak)) TIM8_CC_IRQHandler();
void __attribute__((weak)) DMA1_Stream7_IRQHandler();
void __attribute__((weak)) FMC_IRQHandler();
void __attribute__((weak)) SDMMC1_IRQHandler();
void __attribute__((weak)) TIM5_IRQHandler();
void __attribute__((weak)) SPI3_IRQHandler();
void __attribute__((weak)) UART4_IRQHandler();
void __attribute__((weak)) UART5_IRQHandler();
void __attribute__((weak)) TIM6_DAC_IRQHandler();
void __attribute__((weak)) TIM7_IRQHandler();
void __attribute__((weak)) DMA2_Stream0_IRQHandler();
void __attribute__((weak)) DMA2_Stream1_IRQHandler();
void __attribute__((weak)) DMA2_Stream2_IRQHandler();
void __attribute__((weak)) DMA2_Stream3_IRQHandler();
void __attribute__((weak)) DMA2_Stream4_IRQHandler();
void __attribute__((weak)) ETH_IRQHandler();
void __attribute__((weak)) ETH_WKUP_IRQHandler();
void __attribute__((weak)) CAN2_TX_IRQHandler();
void __attribute__((weak)) CAN2_RX0_IRQHandler();
void __attribute__((weak)) CAN2_RX1_IRQHandler();
void __attribute__((weak)) CAN2_SCE_IRQHandler();
void __attribute__((weak)) OTG_FS_IRQHandler();
void __attribute__((weak)) DMA2_Stream5_IRQHandler();
void __attribute__((weak)) DMA2_Stream6_IRQHandler();
void __attribute__((weak)) DMA2_Stream7_IRQHandler();
void __attribute__((weak)) USART6_IRQHandler();
void __attribute__((weak)) I2C3_EV_IRQHandler();
void __attribute__((weak)) I2C3_ER_IRQHandler();
void __attribute__((weak)) OTG_HS_EP1_OUT_IRQHandler();
void __attribute__((weak)) OTG_HS_EP1_IN_IRQHandler();
void __attribute__((weak)) OTG_HS_WKUP_IRQHandler();
void __attribute__((weak)) OTG_HS_IRQHandler();
void __attribute__((weak)) DCMI_IRQHandler();
void __attribute__((weak)) CRYP_IRQHandler();
void __attribute__((weak)) RNG_IRQHandler();
void __attribute__((weak)) FPU_IRQHandler();
void __attribute__((weak)) UART7_IRQHandler();
void __attribute__((weak)) UART8_IRQHandler();
void __attribute__((weak)) SPI4_IRQHandler();
void __attribute__((weak)) SPI5_IRQHandler();
void __attribute__((weak)) SPI6_IRQHandler();
void __attribute__((weak)) SAI1_IRQHandler();
void __attribute__((weak)) LTDC_IRQHandler();
void __attribute__((weak)) LTDC_ER_IRQHandler();
void __attribute__((weak)) DMA2D_IRQHandler();
void __attribute__((weak)) SAI2_IRQHandler();
void __attribute__((weak)) QUADSPI_IRQHandler();
void __attribute__((weak)) LPTIM1_IRQHandler();
void __attribute__((weak)) CEC_IRQHandler();
void __attribute__((weak)) I2C4_EV_IRQHandler();
void __attribute__((weak)) I2C4_ER_IRQHandler();
void __attribute__((weak)) SPDIF_RX_IRQHandler();
void __attribute__((weak)) DSIHOST_IRQHandler();
void __attribute__((weak)) DFSDM1_FLT0_IRQHandler();
void __attribute__((weak)) DFSDM1_FLT1_IRQHandler();
void __attribute__((weak)) DFSDM1_FLT2_IRQHandler();
void __attribute__((weak)) DFSDM1_FLT3_IRQHandler();
void __attribute__((weak)) SDMMC2_IRQHandler();
void __attribute__((weak)) CAN3_TX_IRQHandler();
void __attribute__((weak)) CAN3_RX0_IRQHandler();
void __attribute__((weak)) CAN3_RX1_IRQHandler();
void __attribute__((weak)) CAN3_SCE_IRQHandler();
void __attribute__((weak)) JPEG_IRQHandler();
void __attribute__((weak)) MDIOS_IRQHandler();

// Stack top, defined in the linker script
extern char _main_stack_top asm("_main_stack_top");

// Interrupt vectors, must be placed @ address 0x00000000
// The extern declaration is required otherwise g++ optimizes it out
extern void (*const __Vectors[])();
void (*const __Vectors[])() __attribute__((section(".isr_vector"))) =
{
    reinterpret_cast<void (*)()>(&_main_stack_top), /* Stack pointer*/
    Reset_Handler,                                  /* Reset Handler */
    NMI_Handler,                                    /* NMI Handler */
    HardFault_Handler,                              /* Hard Fault Handler */
    MemManage_Handler,                              /* MPU Fault Handler */
    BusFault_Handler,                               /* Bus Fault Handler */
    UsageFault_Handler,                             /* Usage Fault Handler */
    0,                                              /* Reserved */
    0,                                              /* Reserved */
    0,                                              /* Reserved */
    0,                                              /* Reserved */
    SVC_Handler,                                    /* SVCall Handler */
    DebugMon_Handler,                               /* Debug Monitor Handler */
    0,                                              /* Reserved */
    PendSV_Handler,                                 /* PendSV Handler */
    SysTick_Handler,                                /* SysTick Handler */

    /* External Interrupts */
    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMP_STAMP_IRQHandler,
    RTC_WKUP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Stream0_IRQHandler,
    DMA1_Stream1_IRQHandler,
    DMA1_Stream2_IRQHandler,
    DMA1_Stream3_IRQHandler,
    DMA1_Stream4_IRQHandler,
    DMA1_Stream5_IRQHandler,
    DMA1_Stream6_IRQHandler,
    ADC_IRQHandler,
    CAN1_TX_IRQHandler,
    CAN1_RX0_IRQHandler,
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM9_IRQHandler,
    TIM1_UP_TIM10_IRQHandler,
    TIM1_TRG_COM_TIM11_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,
    RTC_Alarm_IRQHandler,
    OTG_FS_WKUP_IRQHandler,
    TIM8_BRK_TIM12_IRQHandler,
    TIM8_UP_TIM13_IRQHandler,
    TIM8_TRG_COM_TIM14_IRQHandler,
    TIM8_CC_IRQHandler,
    DMA1_Stream7_IRQHandler,
    FMC_IRQHandler,
    SDMMC1_IRQHandler,
    TIM5_IRQHandler,
    SPI3_IRQHandler,
    UART4_IRQHandler,
    UART5_IRQHandler,
    TIM6_DAC_IRQHandler,
    TIM7_IRQHandler,
    DMA2_Stream0_IRQHandler,
    DMA2_Stream1_IRQHandler,
    DMA2_Stream2_IRQHandler,
    DMA2_Stream3_IRQHandler,
    DMA2_Stream4_IRQHandler,
    ETH_IRQHandler,
    ETH_WKUP_IRQHandler,
    CAN2_TX_IRQHandler,
    CAN2_RX0_IRQHandler,
    CAN2_RX1_IRQHandler,
    CAN2_SCE_IRQHandler,
    OTG_FS_IRQHandler,
    DMA2_Stream5_IRQHandler,
    DMA2_Stream6_IRQHandler,
    DMA2_Stream7_IRQHandler,
    USART6_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    OTG_HS_EP1_OUT_IRQHandler,
    OTG_HS_EP1_IN_IRQHandler,
    OTG_HS_WKUP_IRQHandler,
    OTG_HS_IRQHandler,
    DCMI_IRQHandler,
    CRYP_IRQHandler,
    RNG_IRQHandler,
    FPU_IRQHandler,
    UART7_IRQHandler,
    UART8_IRQHandler,
    SPI4_IRQHandler,
    SPI5_IRQHandler,
    SPI6_IRQHandler,
    SAI1_IRQHandler,
    LTDC_IRQHandler,
    LTDC_ER_IRQHandler,
    DMA2D_IRQHandler,
    SAI2_IRQHandler,
    QUADSPI_IRQHandler,
    LPTIM1_IRQHandler,
    CEC_IRQHandler,
    I2C4_EV_IRQHandler,
    I2C4_ER_IRQHandler,
    SPDIF_RX_IRQHandler,
    DSIHOST_IRQHandler,
    DFSDM1_FLT0_IRQHandler,
    DFSDM1_FLT1_IRQHandler,
    DFSDM1_FLT2_IRQHandler,
    DFSDM1_FLT3_IRQHandler,
    SDMMC2_IRQHandler,
    CAN3_TX_IRQHandler,
    CAN3_RX0_IRQHandler,
    CAN3_RX1_IRQHandler,
    CAN3_SCE_IRQHandler,
    JPEG_IRQHandler,
    MDIOS_IRQHandler,
};

#pragma weak SysTick_IRQHandler = Default_Handler
#pragma weak WWDG_IRQHandler = Default_Handler
#pragma weak PVD_IRQHandler = Default_Handler
#pragma weak TAMP_STAMP_IRQHandler = Default_Handler
#pragma weak RTC_WKUP_IRQHandler = Default_Handler
#pragma weak FLASH_IRQHandler = Default_Handler
#pragma weak RCC_IRQHandler = Default_Handler
#pragma weak EXTI0_IRQHandler = Default_Handler
#pragma weak EXTI1_IRQHandler = Default_Handler
#pragma weak EXTI2_IRQHandler = Default_Handler
#pragma weak EXTI3_IRQHandler = Default_Handler
#pragma weak EXTI4_IRQHandler = Default_Handler
#pragma weak DMA1_Stream0_IRQHandler = Default_Handler
#pragma weak DMA1_Stream1_IRQHandler = Default_Handler
#pragma weak DMA1_Stream2_IRQHandler = Default_Handler
#pragma weak DMA1_Stream3_IRQHandler = Default_Handler
#pragma weak DMA1_Stream4_IRQHandler = Default_Handler
#pragma weak DMA1_Stream5_IRQHandler = Default_Handler
#pragma weak DMA1_Stream6_IRQHandler = Default_Handler
#pragma weak ADC_IRQHandler = Default_Handler
#pragma weak CAN1_TX_IRQHandler = Default_Handler
#pragma weak CAN1_RX0_IRQHandler = Default_Handler
#pragma weak CAN1_RX1_IRQHandler = Default_Handler
#pragma weak CAN1_SCE_IRQHandler = Default_Handler
#pragma weak EXTI9_5_IRQHandler = Default_Handler
#pragma weak TIM1_BRK_TIM9_IRQHandler = Default_Handler
#pragma weak TIM1_UP_TIM10_IRQHandler = Default_Handler
#pragma weak TIM1_TRG_COM_TIM11_IRQHandler = Default_Handler
#pragma weak TIM1_CC_IRQHandler = Default_Handler
#pragma weak TIM2_IRQHandler = Default_Handler
#pragma weak TIM3_IRQHandler = Default_Handler
#pragma weak TIM4_IRQHandler = Default_Handler
#pragma weak I2C1_EV_IRQHandler = Default_Handler
#pragma weak I2C1_ER_IRQHandler = Default_Handler
#pragma weak I2C2_EV_IRQHandler = Default_Handler
#pragma weak I2C2_ER_IRQHandler = Default_Handler
#pragma weak SPI1_IRQHandler = Default_Handler
#pragma weak SPI2_IRQHandler = Default_Handler
#pragma weak USART1_IRQHandler = Default_Handler
#pragma weak USART2_IRQHandler = Default_Handler
#pragma weak USART3_IRQHandler = Default_Handler
#pragma weak EXTI15_10_IRQHandler = Default_Handler
#pragma weak RTC_Alarm_IRQHandler = Default_Handler
#pragma weak OTG_FS_WKUP_IRQHandler = Default_Handler
#pragma weak TIM8_BRK_TIM12_IRQHandler = Default_Handler
#pragma weak TIM8_UP_TIM13_IRQHandler = Default_Handler
#pragma weak TIM8_TRG_COM_TIM14_IRQHandler = Default_Handler
#pragma weak TIM8_CC_IRQHandler = Default_Handler
#pragma weak DMA1_Stream7_IRQHandler = Default_Handler
#pragma weak FMC_IRQHandler = Default_Handler
#pragma weak SDMMC1_IRQHandler = Default_Handler
#pragma weak TIM5_IRQHandler = Default_Handler
#pragma weak SPI3_IRQHandler = Default_Handler
#pragma weak UART4_IRQHandler = Default_Handler
#pragma weak UART5_IRQHandler = Default_Handler
#pragma weak TIM6_DAC_IRQHandler = Default_Handler
#pragma weak TIM7_IRQHandler = Default_Handler
#pragma weak DMA2_Stream0_IRQHandler = Default_Handler
#pragma weak DMA2_Stream1_IRQHandler = Default_Handler
#pragma weak DMA2_Stream2_IRQHandler = Default_Handler
#pragma weak DMA2_Stream3_IRQHandler = Default_Handler
#pragma weak DMA2_Stream4_IRQHandler = Default_Handler
#pragma weak ETH_IRQHandler = Default_Handler
#pragma weak ETH_WKUP_IRQHandler = Default_Handler
#pragma weak CAN2_TX_IRQHandler = Default_Handler
#pragma weak CAN2_RX0_IRQHandler = Default_Handler
#pragma weak CAN2_RX1_IRQHandler = Default_Handler
#pragma weak CAN2_SCE_IRQHandler = Default_Handler
#pragma weak OTG_FS_IRQHandler = Default_Handler
#pragma weak DMA2_Stream5_IRQHandler = Default_Handler
#pragma weak DMA2_Stream6_IRQHandler = Default_Handler
#pragma weak DMA2_Stream7_IRQHandler = Default_Handler
#pragma weak USART6_IRQHandler = Default_Handler
#pragma weak I2C3_EV_IRQHandler = Default_Handler
#pragma weak I2C3_ER_IRQHandler = Default_Handler
#pragma weak OTG_HS_EP1_OUT_IRQHandler = Default_Handler
#pragma weak OTG_HS_EP1_IN_IRQHandler = Default_Handler
#pragma weak OTG_HS_WKUP_IRQHandler = Default_Handler
#pragma weak OTG_HS_IRQHandler = Default_Handler
#pragma weak DCMI_IRQHandler = Default_Handler
#pragma weak CRYP_IRQHandler = Default_Handler
#pragma weak RNG_IRQHandler = Default_Handler
#pragma weak FPU_IRQHandler = Default_Handler
#pragma weak UART7_IRQHandler = Default_Handler
#pragma weak UART8_IRQHandler = Default_Handler
#pragma weak SPI4_IRQHandler = Default_Handler
#pragma weak SPI5_IRQHandler = Default_Handler
#pragma weak SPI6_IRQHandler = Default_Handler
#pragma weak SAI1_IRQHandler = Default_Handler
#pragma weak LTDC_IRQHandler = Default_Handler
#pragma weak LTDC_ER_IRQHandler = Default_Handler
#pragma weak DMA2D_IRQHandler = Default_Handler
#pragma weak SAI2_IRQHandler = Default_Handler
#pragma weak QUADSPI_IRQHandler = Default_Handler
#pragma weak LPTIM1_IRQHandler = Default_Handler
#pragma weak CEC_IRQHandler = Default_Handler
#pragma weak I2C4_EV_IRQHandler = Default_Handler
#pragma weak I2C4_ER_IRQHandler = Default_Handler
#pragma weak SPDIF_RX_IRQHandler = Default_Handler
#pragma weak DSIHOST_IRQHandler = Default_Handler
#pragma weak DFSDM1_FLT0_IRQHandler = Default_Handler
#pragma weak DFSDM1_FLT1_IRQHandler = Default_Handler
#pragma weak DFSDM1_FLT2_IRQHandler = Default_Handler
#pragma weak DFSDM1_FLT3_IRQHandler = Default_Handler
#pragma weak SDMMC2_IRQHandler = Default_Handler
#pragma weak CAN3_TX_IRQHandler = Default_Handler
#pragma weak CAN3_RX0_IRQHandler = Default_Handler
#pragma weak CAN3_RX1_IRQHandler = Default_Handler
#pragma weak CAN3_SCE_IRQHandler = Default_Handler
#pragma weak JPEG_IRQHandler = Default_Handler
#pragma weak MDIOS_IRQHandler = Default_Handler
