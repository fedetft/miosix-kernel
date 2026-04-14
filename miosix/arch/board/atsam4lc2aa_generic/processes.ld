/*
 * Linker script for ATSAM4L
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

_process_pool_size = 16K;

MEMORY
{
    /* 16KB are reserved for the SAM-BA bootloader */
    flash(rx)   : ORIGIN = 0x00004000, LENGTH = 112K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 32K
}

INCLUDE ldscripts/miosix-flash-ram.ld
