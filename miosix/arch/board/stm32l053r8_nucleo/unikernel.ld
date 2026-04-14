/*
 * Linker script for stm32l053
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 64K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 8K
}

INCLUDE ldscripts/miosix-flash-ram.ld
