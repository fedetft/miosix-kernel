/*
 * Linker script for stm32f769
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * NOTE: starting at 0x20000000 there's 128KB of DTCM (Data Tightly Coupled
 * Memory). Then there's 384K of internal SRAM.
 * Since this linker script is for placing only the IRQ stack in SRAM, we use
 * TCM for the interrupt stack and leave the rest of the internal RAM unused
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 128K /* TCM for IRQ stack */
    ram2(wx)    : ORIGIN = 0xc0000000, LENGTH = 16M  /* XRAM */
}

INCLUDE ldscripts/miosix-flash-2ram-separate-irqstack.ld
