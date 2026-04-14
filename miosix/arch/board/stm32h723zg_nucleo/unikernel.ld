/*
 * Linker script for stm32h723
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 1M
    /* NOTE: for now we support only the AXI SRAM */
    ram(wx)     : ORIGIN = 0x24000000, LENGTH = 128K
}

INCLUDE ldscripts/miosix-flash-ram.ld
