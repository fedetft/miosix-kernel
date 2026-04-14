/*
 * Linker script for stm32h753
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    /* NOTE: for now wer support only the AXI SRAM */
    ram(wx)     : ORIGIN = 0x24000000, LENGTH = 512K
}

INCLUDE ldscripts/miosix-flash-ram.ld
