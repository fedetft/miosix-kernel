/***************************************************************************
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

/* 
 * System initialization code for RP2040
 * Partially based on code from the Raspberry Pi Pico SDK
 * 
 * The RP2040 boot process consists of three stages:
 *  Stage 1: Bootloader in ROM does basic system initialization. If BOOTSEL is
 *           not set, loads stage 2 and jumps to it.
 *  Stage 2: Stored in the first 256 bytes of flash. Configures XIP peripheral
 *           for the specific FLASH chip used, then jumps to stage 3. Source
 *           code for stage 2 is in common/rp2040/pre_boot.
 *  Stage 3: This file.
 * 
 * Miosix however numbers its own boot stages starting from 1 for this file,
 * so we consider RP2040 stage 1 and stage 2 as "Miosix preboot".
 */

#include "interfaces/arch_registers.h"
#include "core/interrupts.h" //For the unexpected interrupt call
#include "kernel/stage_2_boot.h"
#include <string.h>

void Reset_Handler() __attribute__((__interrupt__, naked, noreturn));
void programStartup() __attribute__((noreturn));
void clockTreeSetup();

/**
 * ELF entry point. Goes back through bootrom + boot2 to properly initialise
 * flash from scratch.
 */
extern "C" __attribute__((naked, noreturn)) void entryPoint()
{
    asm volatile(
        "movs r0, #0           \n"
        "ldr r1, =0xe000ed08   \n"
        "str r0, [r1]          \n" // Reinitialize VTOR to 0 (ROM vector table)
        "ldmia r0!, {r1, r2}   \n" // Load initial entry point and SP in R1,R2
        "msr msp, r1           \n" // Reset stack pointer
        "bx r2                 \n" // Jump to ROM
        :::"r0","r1","r2");
}

/**
 * Reset handler, called by hardware immediately after reset
 */
void Reset_Handler()
{
    /*
     * Check if we are core 0. If it's not the case, go back to the bootrom.
     * This should not be necessary, but the Pico SDK does this check to play it
     * safe...
     */
    asm volatile(
        "   ldr r0, =0xd0000000         \n" // 0xd0000000 = SIO CPUID
        "   ldr r0, [r0]                \n"
        "   cmp r0, #0                  \n" // Is it core zero?
        "   beq 1f                      \n" // Yes: all OK, continue
        "   movs r3, #20                \n" // No: go back to bootrom
        "   ldrh r3, [r3, #4]           \n" // R3 = addr of rom_func_lookup
        "   ldrh r0, [r3, #0]           \n" // arg0 = function table address
        "   ldr r1, = 'W' | ('V' << 8)  \n" // arg1 = function code for _wait_for_vector
        "   blx r3                      \n" // Call rom_func_lookup, returns address of _wait_for_vector in R0
        "   bx r0                       \n" // Jump to _wait_for_vector. Does not return
        "1:                             \n"
        :::"r0");
    /*
     * Initialize process stack and switch to it.
     * This is required for booting Miosix, a small portion of the top of the
     * heap area will be used as stack until the first thread starts. After,
     * this stack will be abandoned and the process stack will point to the
     * current thread's stack.
     */
    asm volatile(
        "ldr r0,  =_heap_end          \n"
        "msr psp, r0                  \n"
        "movs r0, #2                  \n" //Privileged, process stack
        "msr control, r0              \n"
        "isb                          \n"
        :::"r0");

    programStartup();
}

/**
 * Called by Reset_Handler, performs initialization and calls main.
 * Never returns.
 */
void programStartup()
{
    //Cortex-M0 appears to get out of reset with interrupts already enabled.
    //Amazingly, even though we went through the bootrom and the flash
    //bootloader, interrupts are STILL enabled!
    __disable_irq();

    //Initialize the system *before* initializing .data and zeroing .bss.
    //Usually the opposite is done, as there is no guarantee C code works
    //properly if those memory areas are not properly initialized.
    //However, there are three good reasons to do so:
    //First, the CMSIS specifications say that SystemInit() must not access
    //global variables, so it is actually possible to call it before
    //Second, when running Miosix with the xram linker scripts .data and .bss
    //are placed in the external RAM, so we *must* call SystemInit(), which
    //enables xram, before touching .data and .bss
    //Third, this is a performance improvement since the loops that initialize
    //.data and zeros .bss now run with the CPU at full speed

    //On RP2040 this function is empty, as they do not really support CMSIS
    //properly. We do everyting ourselves.
    //SystemInit();

    // Reset all peripherals to put system into a known state,
    // - except for QSPI pads and the XIP IO bank, as this is fatal if running from flash
    // - and the PLLs, as this is fatal if clock muxing has not been reset on this boot
    // - and USB, syscfg, as this disturbs USB-to-SWD on core 1
    reset_block(~(RESETS_RESET_IO_QSPI_BITS | RESETS_RESET_PADS_QSPI_BITS |
            RESETS_RESET_PLL_USB_BITS | RESETS_RESET_USBCTRL_BITS |
            RESETS_RESET_SYSCFG_BITS | RESETS_RESET_PLL_SYS_BITS));
    // Remove reset from peripherals which are clocked only by clk_sys and
    // clk_ref. Other peripherals stay in reset until we've configured clocks.
    unreset_block_wait(RESETS_RESET_BITS & ~(RESETS_RESET_ADC_BITS |
            RESETS_RESET_RTC_BITS | RESETS_RESET_SPI0_BITS |
            RESETS_RESET_SPI1_BITS | RESETS_RESET_UART0_BITS |
            RESETS_RESET_UART1_BITS | RESETS_RESET_USBCTRL_BITS));
    
    // Setup clock generation
    clockTreeSetup();

    // Peripheral clocks should now all be running, turn on basic peripherals
    unreset_block_wait(RESETS_RESET_SYSINFO_BITS | 
        RESETS_RESET_SYSCFG_BITS | RESETS_RESET_BUSCTRL_BITS);

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
    SystemCoreClock = CLK_SYS_FREQ;

    //Move on to stage 2
    _init();

    //If main returns, reboot
    NVIC_SystemReset();
    for(;;) ;
}

/// Configure the XOSC peripheral
static void xoscInit(void)
{
    xosc_hw->ctrl = XOSC_CTRL_FREQ_RANGE_VALUE_1_15MHZ;
    xosc_hw->startup = ((XOSC_FREQ / 1000) + 128) / 256; // Startup wait ~= 1ms
    hw_set_bits(&xosc_hw->ctrl, XOSC_CTRL_ENABLE_VALUE_ENABLE<<XOSC_CTRL_ENABLE_LSB);
    // Wait for XOSC to be stable
    while(!(xosc_hw->status & XOSC_STATUS_STABLE_BITS));
}

/// Configure a specific PLL
static void pllInit(pll_hw_t *pll, uint32_t refdiv, uint32_t fbdiv,
        uint32_t post_div1, uint32_t post_div2)
{
    // Reset the PLL
    uint32_t pll_reset = (pll_usb_hw==pll)? RESETS_RESET_PLL_USB_BITS:
                                            RESETS_RESET_PLL_SYS_BITS;
    reset_block(pll_reset);
    unreset_block_wait(pll_reset);
    // Load VCO-related dividers before starting VCO
    pll->cs = refdiv;
    pll->fbdiv_int = fbdiv;
    // Turn on PLL
    hw_clear_bits(&pll->pwr, PLL_PWR_PD_BITS | PLL_PWR_VCOPD_BITS);
    // Wait for PLL to lock
    while (!(pll->cs & PLL_CS_LOCK_BITS));
    // Setup and turn on post divider
    pll->prim = (post_div1<<PLL_PRIM_POSTDIV1_LSB) |
                (post_div2<<PLL_PRIM_POSTDIV2_LSB);
    hw_clear_bits(&pll->pwr, PLL_PWR_POSTDIVPD_BITS);
}

/// Configure a clock generator device with glitchless mux
static void glitchlessClockInit(enum clock_index clk_index,
        uint32_t src, uint32_t auxsrc, uint32_t div)
{
    clock_hw_t *clock = &clocks_hw->clk[clk_index];
    div = div << CLOCKS_CLK_GPOUT0_DIV_INT_LSB;
    // If increasing divisor, set divisor before source. Otherwise set source
    // before divisor. This avoids a momentary overspeed when e.g. switching
    // to a faster source and increasing divisor to compensate.
    if (div > clock->div)
        clock->div = div;
    // If switching a glitchless slice (ref or sys) to an aux source, switch
    // away from aux *first* to avoid passing glitches when changing aux mux.
    // Assume (!!!) glitchless source 0 is no faster than the aux source.
    if (src == CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX)
    {
        hw_clear_bits(&clock->ctrl, CLOCKS_CLK_REF_CTRL_SRC_BITS);
        while (!(clock->selected & 1u)) {}
    }
    // Set aux mux first, and then glitchless mux
    hw_write_masked(&clock->ctrl,
        (auxsrc << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB),
        CLOCKS_CLK_SYS_CTRL_AUXSRC_BITS);
    hw_write_masked(&clock->ctrl,
        src << CLOCKS_CLK_REF_CTRL_SRC_LSB,
        CLOCKS_CLK_REF_CTRL_SRC_BITS);
    while (!(clock->selected & (1u << src))) {}
    // Now that the source is configured, we can trust that the user-supplied
    // divisor is a safe value.
    clock->div = div;
}

/// Configure a clock generator device without glitchless mux
static void clockInit(enum clock_index clk_index, uint32_t auxsrc, uint32_t div)
{
    clock_hw_t *clock = &clocks_hw->clk[clk_index];
    div = div << CLOCKS_CLK_GPOUT0_DIV_INT_LSB;
    // If increasing divisor, set divisor before source. Otherwise set source
    // before divisor. This avoids a momentary overspeed when e.g. switching
    // to a faster source and increasing divisor to compensate.
    if (div > clock->div)
        clock->div = div;
    // Cleanly stop the clock to avoid glitches propagating when
    // changing aux mux.
    hw_clear_bits(&clock->ctrl, CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS);
    // Delay for around 200 cycles to ensure the clock propagated.
    // We consider a worst case scenario where the CPU clock is clocked at the
    // full 125MHz and the clock to configure is clocked at 1MHz
    for (int i=0; i<200; i++) asm("");
    // Set aux mux
    hw_write_masked(&clock->ctrl,
        (auxsrc << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB),
        CLOCKS_CLK_SYS_CTRL_AUXSRC_BITS);
    // Enable clock.
    hw_set_bits(&clock->ctrl, CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS);
    // Now that the source is configured, we can trust that the user-supplied
    // divisor is a safe value.
    clock->div = div;
}

void clockTreeSetup(void)
{
    // Disable "resuscitator" and enable xosc
    clocks_hw->resus.ctrl = 0;
    xoscInit();

    // Before we touch PLLs, switch sys and ref cleanly away from their aux sources.
    hw_clear_bits(&clocks_hw->clk[clk_sys].ctrl, CLOCKS_CLK_SYS_CTRL_SRC_BITS);
    while (clocks_hw->clk[clk_sys].selected != 0x1);
    hw_clear_bits(&clocks_hw->clk[clk_ref].ctrl, CLOCKS_CLK_REF_CTRL_SRC_BITS);
    while (clocks_hw->clk[clk_ref].selected != 0x1);
    // Setup SYS PLL to CLK_SYS_FREQ rounded to the nearest MHz
    // VCO frequency (fb_div * XOSC_FREQ) must be >= 750MHz
    static_assert((CLK_SYS_FREQ / 1000000) * XOSC_FREQ >= 750, "CLK_SYS_FREQ too slow");
    // SYS PLL = 12MHz * (CLK_SYS_FREQ/1000000) / 6 / 2 ~= CLK_SYS_FREQ
    pllInit(pll_sys_hw, 1, CLK_SYS_FREQ/1000000, 6, 2);
    // USB PLL = 12MHz * 64 / 4 / 4 = 48 MHz
    pllInit(pll_usb_hw, 1, 64, 4, 4);

    // Configure clocks:
    // CLK_REF = XOSC (usually) 12MHz / 1 = 12MHz
    glitchlessClockInit(clk_ref,
            CLOCKS_CLK_REF_CTRL_SRC_VALUE_CLKSRC_CLK_REF_AUX,
            CLOCKS_CLK_REF_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB, 1);
    // Enable watchdog tick timer (also used by timer peripheral)
    // Frequency: 12MHz (clk_ref) / 12 = 1MHz
    watchdog_hw->tick = WATCHDOG_TICK_ENABLE_BITS | 1;
    // CLK SYS = PLL SYS (usually) 125MHz / 1 = 125MHz
    glitchlessClockInit(clk_sys,
            CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
            CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, 1);
    // CLK USB = PLL USB 48MHz / 1 = 48MHz
    clockInit(clk_usb, CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB, 1);
    // CLK ADC = PLL USB 48MHZ / 1 = 48MHz
    clockInit(clk_adc, CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB, 1);
    // CLK RTC = PLL USB 48MHz / 1024 = 46875Hz
    clockInit(clk_rtc, CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB, 1024);
    // CLK PERI = clk_sys.
    clockInit(clk_peri, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, 1);
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

// These system handlers are present in Miosix but not supported by the
// architecture, so are defined as weak
// void __attribute__((weak)) MemManage_Handler();
// void __attribute__((weak)) BusFault_Handler();
// void __attribute__((weak)) UsageFault_Handler();
// void __attribute__((weak)) DebugMon_Handler();

//Interrupt handlers
void __attribute__((weak)) SysTick_Handler();
void __attribute__((weak)) TIMER_IRQ_0_Handler();
void __attribute__((weak)) TIMER_IRQ_1_Handler();
void __attribute__((weak)) TIMER_IRQ_2_Handler();
void __attribute__((weak)) TIMER_IRQ_3_Handler();
void __attribute__((weak)) PWM_IRQ_WRAP_Handler();
void __attribute__((weak)) USBCTRL_IRQ_Handler();
void __attribute__((weak)) XIP_IRQ_Handler();
void __attribute__((weak)) PIO0_IRQ_0_Handler();
void __attribute__((weak)) PIO0_IRQ_1_Handler();
void __attribute__((weak)) PIO1_IRQ_0_Handler();
void __attribute__((weak)) PIO1_IRQ_1_Handler();
void __attribute__((weak)) DMA_IRQ_0_Handler();
void __attribute__((weak)) DMA_IRQ_1_Handler();
void __attribute__((weak)) IO_IRQ_BANK0_Handler();
void __attribute__((weak)) IO_IRQ_QSPI_Handler();
void __attribute__((weak)) SIO_IRQ_PROC0_Handler();
void __attribute__((weak)) SIO_IRQ_PROC1_Handler();
void __attribute__((weak)) CLOCKS_IRQ_Handler();
void __attribute__((weak)) SPI0_IRQ_Handler();
void __attribute__((weak)) SPI1_IRQ_Handler();
void __attribute__((weak)) UART0_IRQ_Handler();
void __attribute__((weak)) UART1_IRQ_Handler();
void __attribute__((weak)) ADC_IRQ_FIFO_Handler();
void __attribute__((weak)) I2C0_IRQ_Handler();
void __attribute__((weak)) I2C1_IRQ_Handler();
void __attribute__((weak)) RTC_IRQ_Handler();

//Stack top, defined in the linker script
extern char _main_stack_top asm("_main_stack_top");

//Include the RP2040 flash stage 2 pre-built bootloader binary.
//Boot2 is also responsible for pointing VTOR to our interrupt table.
#include <rp2040/pre_boot/boot2.h>

//Interrupt vectors
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

    TIMER_IRQ_0_Handler,
    TIMER_IRQ_1_Handler,
    TIMER_IRQ_2_Handler,
    TIMER_IRQ_3_Handler,
    PWM_IRQ_WRAP_Handler,
    USBCTRL_IRQ_Handler,
    XIP_IRQ_Handler,
    PIO0_IRQ_0_Handler,
    PIO0_IRQ_1_Handler,
    PIO1_IRQ_0_Handler,
    PIO1_IRQ_1_Handler,
    DMA_IRQ_0_Handler,
    DMA_IRQ_1_Handler,
    IO_IRQ_BANK0_Handler,
    IO_IRQ_QSPI_Handler,
    SIO_IRQ_PROC0_Handler,
    SIO_IRQ_PROC1_Handler,
    CLOCKS_IRQ_Handler,
    SPI0_IRQ_Handler,
    SPI1_IRQ_Handler,
    UART0_IRQ_Handler,
    UART1_IRQ_Handler,
    ADC_IRQ_FIFO_Handler,
    I2C0_IRQ_Handler,
    I2C1_IRQ_Handler,
    RTC_IRQ_Handler
};

#pragma weak SysTick_Handler = Default_Handler
#pragma weak TIMER_IRQ_0_Handler = Default_Handler
#pragma weak TIMER_IRQ_1_Handler = Default_Handler
#pragma weak TIMER_IRQ_2_Handler = Default_Handler
#pragma weak TIMER_IRQ_3_Handler = Default_Handler
#pragma weak PWM_IRQ_WRAP_Handler = Default_Handler
#pragma weak USBCTRL_IRQ_Handler = Default_Handler
#pragma weak XIP_IRQ_Handler = Default_Handler
#pragma weak PIO0_IRQ_0_Handler = Default_Handler
#pragma weak PIO0_IRQ_1_Handler = Default_Handler
#pragma weak PIO1_IRQ_0_Handler = Default_Handler
#pragma weak PIO1_IRQ_1_Handler = Default_Handler
#pragma weak DMA_IRQ_0_Handler = Default_Handler
#pragma weak DMA_IRQ_1_Handler = Default_Handler
#pragma weak IO_IRQ_BANK0_Handler = Default_Handler
#pragma weak IO_IRQ_QSPI_Handler = Default_Handler
#pragma weak SIO_IRQ_PROC0_Handler = Default_Handler
#pragma weak SIO_IRQ_PROC1_Handler = Default_Handler
#pragma weak CLOCKS_IRQ_Handler = Default_Handler
#pragma weak SPI0_IRQ_Handler = Default_Handler
#pragma weak SPI1_IRQ_Handler = Default_Handler
#pragma weak UART0_IRQ_Handler = Default_Handler
#pragma weak UART1_IRQ_Handler = Default_Handler
#pragma weak ADC_IRQ_FIFO_Handler = Default_Handler
#pragma weak I2C0_IRQ_Handler = Default_Handler
#pragma weak I2C1_IRQ_Handler = Default_Handler
#pragma weak RTC_IRQ_Handler = Default_Handler

