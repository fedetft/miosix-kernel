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
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x10000000, LENGTH = 64K /* TCM for IRQ stack */
    ram2(wx)    : ORIGIN = 0xd0000000, LENGTH = 8M  /* XRAM */
}

/* Mark ram2 as being an external RAM */
_xram_start = ORIGIN(ram2);
_xram_size  = LENGTH(ram2);

INCLUDE ldscripts/miosix-flash-2ram-separate-irqstack.ld
