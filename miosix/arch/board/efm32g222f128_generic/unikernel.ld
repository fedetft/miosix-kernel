/*
 * Linker script for efm32g222f128
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x00000800, LENGTH = 128K-2K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 16K
}

INCLUDE ldscripts/miosix-flash-ram.ld
