/*
 * Linker script for efm32gg990f1024
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

_process_pool_size = 96K;

MEMORY
{
    /* The first 16KB are normally used by the factory-programmed bootloader
     * but the demo firmware on this board already overwrote it */
    flash(rx)   : ORIGIN = 0x00000000, LENGTH = 1M
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 128K
}

INCLUDE ldscripts/miosix-flash-ram.ld
