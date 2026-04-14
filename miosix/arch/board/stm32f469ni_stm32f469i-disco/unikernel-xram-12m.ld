/*
 * Linker script for stm32f469
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * NOTE: starting at 0x10000000 there's 64KB of TCM.
 * Starting at 0x20000000 there's 320K of internal SRAM.
 * Since this linker script is for placing everything (but the IRQ stack) in
 * XRAM, but the bootloader of this board has the quirk that boot fails if the
 * initial stack pointer is in the TCM, we need to put the IRQ stack in SRAM
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 320K
    ram2(wx)    : ORIGIN = 0xc0000000, LENGTH = 16M  /* XRAM */
}

INCLUDE ldscripts/miosix-flash-2ram-separate-irqstack.ld
