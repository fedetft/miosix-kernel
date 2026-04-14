/*
 * Linker script for stm32f100
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    /* 1KB reserved by BSP for non volatile storage */
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 63K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 8K
}

INCLUDE ldscripts/miosix-flash-ram.ld
