/*
 * Linker script for efm32gg990f1024
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    /* The first 16KB are normally used by the factory-programmed bootloader
     * but the demo firmware on this board already overwrote it */
    flash(rx)   : ORIGIN = 0x00000000, LENGTH = 1M
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 128K
}

INCLUDE ldscripts/miosix-flash-ram.ld
