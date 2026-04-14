/*
 * Linker script for stm32f429
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * NOTE: starting at 0x10000000 there's 64KB of TCM.
 * Starting at 0x20000000 there's 192K of internal SRAM.
 * Since this linker script is for placing everything (but the IRQ stack) in
 * XRAM, we use TCM for the interrupt stack and leave the rest of the internal
 * RAM unused
 */

/*
 * NOTE: the SDRAM is 8MB, but this linker script only uses the lower 6.
 * The upper 2MB are reserved as framebuffers for the LCD. The choice to
 * allocate this space is due to the SDRAM architecture, composed of 4 banks of
 * 2MB each. Reserving a bank for the LCD allows the software running on the
 * board and the LCD to operate (almost) independently.
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x10000000, LENGTH = 64K /* TCM for IRQ stack */
    ram2(wx)    : ORIGIN = 0xd0000000, LENGTH = 6M  /* XRAM */
}

INCLUDE ldscripts/miosix-flash-2ram-separate-irqstack.ld
