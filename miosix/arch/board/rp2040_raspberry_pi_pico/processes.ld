/*
 * Linker script for RP2040
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

_process_pool_size = 200K;

MEMORY
{
    flash(rx)   : ORIGIN = 0x10000000, LENGTH = 2M
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 264K
}
