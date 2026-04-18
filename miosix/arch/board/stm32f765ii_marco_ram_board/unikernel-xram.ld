/*
 * Linker script for stm32f765
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * NOTE: starting at 0x20000000 there's 128KB of DTCM (Data Tightly Coupled
 * Memory). Then there's 384K of internal SRAM.
 * Since this linker script is for placing everything (but the IRQ stack) in
 * XRAM, we use TCM for the interrupt stack and leave the rest of the internal
 * RAM unused
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 128K /* TCM for IRQ stack */
    ram2(wx)    : ORIGIN = 0xd0000000, LENGTH = 256M /* XRAM */
}

/* Mark ram2 as being an external RAM */
_xram_start = ORIGIN(ram2);
_xram_size  = LENGTH(ram2);

INCLUDE ldscripts/miosix-flash-2ram-separate-irqstack.ld
