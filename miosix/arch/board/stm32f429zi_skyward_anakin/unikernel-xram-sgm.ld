/*
 * Linker script for stm32f429
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * NOTE: starting at 0x10000000 there's 64KB of TCM.
 * Starting at 0x20000000 there's 192K of internal SRAM.
 * Since this linker script is for placing only the heap in XRAM, we use SRAM
 * as ram1
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 192K
    ram2(wx)    : ORIGIN = 0xd0000000, LENGTH = 8M  /* XRAM */
    sgm(rw)     : ORIGIN = 0x40024000, LENGTH = 4K  /* safeguard memory (.preserve) */
}
