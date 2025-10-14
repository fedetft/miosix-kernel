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
 * In Miosix we consider RP2040 stage 1 and stage 2 as "Miosix preboot".
 */

#include "config/miosix_settings.h"
#include "interfaces/arch_registers.h"
#include "interfaces/gpio.h"
#include "mpu/cortexMx_mpu.h"
#include <string.h>

//Include the RP2040 flash stage 2 pre-built bootloader binary.
//Boot2 is also responsible for pointing VTOR to our interrupt table.
#include <rp2040/pre_boot/boot2.h>

namespace miosix {

/// Configure the XOSC peripheral
static void xoscInit()
{
    xosc_hw->ctrl = XOSC_CTRL_FREQ_RANGE_VALUE_1_15MHZ;
    xosc_hw->startup = ((oscillatorFrequency / 1000) + 128) / 256; // Startup wait ~= 1ms
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

static void clockTreeSetup()
{
    // Disable "resuscitator" and enable xosc
    clocks_hw->resus.ctrl = 0;
    xoscInit();

    // Before we touch PLLs, switch sys and ref cleanly away from their aux sources.
    hw_clear_bits(&clocks_hw->clk[clk_sys].ctrl, CLOCKS_CLK_SYS_CTRL_SRC_BITS);
    while(clocks_hw->clk[clk_sys].selected != 0x1) ;
    hw_clear_bits(&clocks_hw->clk[clk_ref].ctrl, CLOCKS_CLK_REF_CTRL_SRC_BITS);
    while(clocks_hw->clk[clk_ref].selected != 0x1) ;

    if(cpuFrequency>133333333)
    {
        // vcore to 1.15V
        vreg_and_chip_reset_hw->vreg=(12<<VREG_AND_CHIP_RESET_VREG_VSEL_LSB) 
                                     | VREG_AND_CHIP_RESET_VREG_EN_BITS;
    } else {
        // vcore to 1.1V
        vreg_and_chip_reset_hw->vreg=(11<<VREG_AND_CHIP_RESET_VREG_VSEL_LSB) 
                                     | VREG_AND_CHIP_RESET_VREG_EN_BITS;
    }
    while(!(vreg_and_chip_reset_hw->vreg&VREG_AND_CHIP_RESET_VREG_ROK_BITS)) ;

    // Setup SYS PLL to cpuFrequency rounded to the nearest MHz
    // VCO frequency (fb_div * oscillatorFrequency) must be >= 750MHz
    static_assert((cpuFrequency / 1000000) * oscillatorFrequency >= 750, "cpuFrequency too slow");
    // SYS PLL = 12MHz * (cpuFrequency/1000000) / 6 / 2 ~= cpuFrequency
    // Optimal parameters for common frequencies
    if(cpuFrequency==133333333) pllInit(pll_sys_hw, 1, 100, 3, 3);
    else if(cpuFrequency==200000000) pllInit(pll_sys_hw, 1, 100, 6, 1);
    else pllInit(pll_sys_hw, 1, cpuFrequency/1000000, 6, 2);
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
 * This function is the first function called during boot to initialize the
 * platform memory and clock subsystems.
 *
 * Code in this function has several important restrictions:
 * - When this function is called, part of the memory address space may not be
 *   available. This occurs when the board includes an external memory, and
 *   indeed it is the purpose of this very function to enable the external
 *   memory (if present) and map it into the address space!
 * - This function is called before global and static variables in .data/.bss
 *   are initialized. As a consequence, this function and all function it calls
 *   are forbidden from referencing global and static variables
 * - This function is called with the stack pointer pointing to the interrupt
 *   stack. This is in general a small stack, but is the only stack that is
 *   guaranteed to be in the internal memory. The allocation of stack-local
 *   variables and the nesting of function calls should be kept to a minimum
 * - This function is called with interrupts disabled, before the kernel is
 *   started and before the I/O subsystem is enabled. There is thus no way
 *   of printing any debug message.
 *
 * This function should perform the following operations:
 * - Configure the internal memory wait states to support the desired target
 *   operating frequency
 * - Configure the CPU clock (e.g: PLL) to run at the desired target frequency
 * - Enable and configure the external memory (if available)
 *
 * As a postcondition of running this function, the entire memory map as
 * specified in the linker script should be accessible, so the rest of the
 * kernel can use the memory to complete the boot sequence, and the CPU clock
 * should be configured at the desired target frequency so the boot can proceed
 * quickly.
 */
void IRQmemoryAndClockInit()
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
        :::"r0","cc");

    //On RP2040 this function is empty, as they do not really support CMSIS
    //properly. We do everyting ourselves.
    //SystemInit();

    // Reset all peripherals to put system into a known state, except for:
    // - QSPI pads and the XIP IO bank, as this is fatal if running from flash
    // - the PLLs, as this is fatal if clock muxing has not been reset on this boot
    reset_block(~(RESETS_RESET_IO_QSPI_BITS
        | RESETS_RESET_PADS_QSPI_BITS
        | RESETS_RESET_PLL_USB_BITS
        | RESETS_RESET_PLL_SYS_BITS)); // These are the peripherals NOT being reset
    // Kill clock of non-essential peripherals
    // Essential peripherals are BUSCTRL, BUSFAB, VREG, Resets, ROM, SRAMs
    // We also leave on some more stuff we need
    clocks_hw->wake_en0=clocks_hw->sleep_en0=
          CLOCKS_WAKE_EN0_CLK_SYS_CLOCKS_BITS
        | CLOCKS_WAKE_EN0_CLK_SYS_BUSCTRL_BITS // (essential)
        | CLOCKS_WAKE_EN0_CLK_SYS_BUSFABRIC_BITS // (essential)
        | CLOCKS_WAKE_EN0_CLK_SYS_IO_BITS // (essential as we are using QSPI)
        | CLOCKS_WAKE_EN0_CLK_SYS_VREG_AND_CHIP_RESET_BITS // (essential)
        | CLOCKS_WAKE_EN0_CLK_SYS_PADS_BITS // (essential as we are using QSPI)
        | CLOCKS_WAKE_EN0_CLK_SYS_PLL_SYS_BITS // (essential as we might be clocked using the PLL now)
        | CLOCKS_WAKE_EN0_CLK_SYS_PLL_USB_BITS // (essential as we might be clocked using the PLL now)
        | CLOCKS_WAKE_EN0_CLK_SYS_PSM_BITS // Power on State Machine
        | CLOCKS_WAKE_EN0_CLK_SYS_RESETS_BITS // (essential)
        | CLOCKS_WAKE_EN0_CLK_SYS_ROM_BITS // (essential)
        | CLOCKS_WAKE_EN0_CLK_SYS_ROSC_BITS // Internal ring oscillator (essential as we might be clocked using the ROSC now)
        | CLOCKS_WAKE_EN0_CLK_SYS_SIO_BITS // Single-Cycle IO (GPIOs/CPUID/CPU FIFOs/Spinlocks) (needed by the kernel later)
        | CLOCKS_WAKE_EN0_CLK_SYS_SRAM0_BITS // (essential)
        | CLOCKS_WAKE_EN0_CLK_SYS_SRAM1_BITS // (essential)
        | CLOCKS_WAKE_EN0_CLK_SYS_SRAM2_BITS // (essential)
        | CLOCKS_WAKE_EN0_CLK_SYS_SRAM3_BITS; // (essential)
    clocks_hw->wake_en1=clocks_hw->sleep_en1=
          CLOCKS_WAKE_EN1_CLK_SYS_SRAM4_BITS // (essential)
        | CLOCKS_WAKE_EN1_CLK_SYS_SRAM5_BITS // (essential)
        | CLOCKS_WAKE_EN1_CLK_SYS_WATCHDOG_BITS // (needed by the kernel later for rebooting)
        | CLOCKS_WAKE_EN1_CLK_SYS_XIP_BITS // (essential because almost certainly in use now)
        | CLOCKS_WAKE_EN1_CLK_SYS_XOSC_BITS; // (essential because we might be clocked by this now)

    //QUIRK: the rp2040 GPIO when reset configures pins as output by default!
    //This was causing problems if a fault reset occurs such as a HardFault as
    //immediately after printing the fault debug message, the chip would reboot
    //and hit the reset above that would configure the serial TX as
    //output low, inadvertently sending a break condition that would prevent
    //the debug message from being printed. As a positive side effect of this
    //fix, ALL GPIOs, not just the serial ones get disabled at boot, which is
    //desirable as pins driven externally will cause a short circuit with the
    //default output configuration. This is still not perfect, as between the
    //reset_block() and the loop there's still a few microsecond glitch where
    //the GPIO pins are driven low. Sadly, that's the best we can do to fix
    //bad hardware design.
    for(unsigned int i=0; i<NUM_BANK0_GPIOS; i++)
        padsbank0_hw->io[i]=toUint(Mode::DISABLED);
    
    //QUIRK: You would expect the hardware spinlocks to be reset on a CPU
    //reset but they are not.
    for(unsigned int i=0; i<32; i++) sio_hw->spinlock[i]=1;

    // Setup clock generation
    clockTreeSetup();

    // Peripheral clocks should now all be running, turn on basic peripherals
    clocks_hw->wake_en0=clocks_hw->sleep_en0|=
          CLOCKS_WAKE_EN0_CLK_SYS_BUSCTRL_BITS;
    clocks_hw->wake_en1=clocks_hw->sleep_en1|=
          CLOCKS_WAKE_EN1_CLK_SYS_SYSINFO_BITS
        | CLOCKS_WAKE_EN1_CLK_SYS_SYSCFG_BITS;
    unreset_block_wait(RESETS_RESET_SYSINFO_BITS
                     | RESETS_RESET_SYSCFG_BITS
                     | RESETS_RESET_BUSCTRL_BITS);
    // Disable peripherals we surely don't need anymore
    clocks_hw->wake_en0=clocks_hw->sleep_en0&=~CLOCKS_WAKE_EN0_CLK_SYS_ROSC_BITS;
    // Disable sleep clocks of peripherals that won't be accessed if the CPU sleeps
    clocks_hw->sleep_en0&=~(
          CLOCKS_WAKE_EN0_CLK_SYS_CLOCKS_BITS // can't reconfigure clocks when in sleep
        | CLOCKS_WAKE_EN0_CLK_SYS_IO_BITS // can't reconfigure pads when in sleep
        | CLOCKS_WAKE_EN0_CLK_SYS_PADS_BITS // can't reconfigure pads when in sleep
        | CLOCKS_WAKE_EN0_CLK_SYS_RESETS_BITS // can't reconfigure resets when in sleep
        | CLOCKS_WAKE_EN0_CLK_SYS_ROM_BITS // can't read the ROM when in sleep
        | CLOCKS_WAKE_EN0_CLK_SYS_SIO_BITS); // only accessible by CPUs
    clocks_hw->sleep_en1&=~(
          CLOCKS_WAKE_EN1_CLK_SYS_SYSINFO_BITS // only accessible by CPUs
        | CLOCKS_WAKE_EN1_CLK_SYS_SYSCFG_BITS // only accessible by CPUs
        | CLOCKS_WAKE_EN1_CLK_SYS_WATCHDOG_BITS // only used for resetting
        | CLOCKS_WAKE_EN1_CLK_SYS_XIP_BITS); // won't DMA from the XIP ROM (if we do, reenable!)

    // Architecture has MPU, enable kernel-level W^X protection
    IRQconfigureMPU();
}

} //namespace miosix
