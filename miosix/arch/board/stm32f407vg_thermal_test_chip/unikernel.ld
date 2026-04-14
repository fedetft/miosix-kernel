/*
 * Linker script for stm32f407
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 1M
    /* TCM not used here, left for logging buffers at the application level */
    /* ram1(wx)    : ORIGIN = 0x10000000, LENGTH = 64K */
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 128K
}

INCLUDE ldscripts/miosix-flash-ram.ld
