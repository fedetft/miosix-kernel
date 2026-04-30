/*
 * Linker script for nrf52840
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

_process_pool_size = 128K;

MEMORY
{
    flash(rx)   : ORIGIN = 0x00000000, LENGTH = 1M
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 256K
}

INCLUDE ldscripts/miosix-flash-ram.ld
