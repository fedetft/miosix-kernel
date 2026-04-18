/*
 * Linker script for stm32f207
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

/* No _process_pool_size as it's fixed to the size of ram2 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 1M
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 128K
    ram2(wx)    : ORIGIN = 0x60000000, LENGTH = 512K
}

/* Mark ram2 as being an external RAM */
_xram_start = ORIGIN(ram2);
_xram_size  = LENGTH(ram2);

INCLUDE ldscripts/miosix-flash-2ram-separate-pool.ld
