/*
 * Linker script for stm32f401
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

_process_pool_size = 64K;

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 512K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 96K
}
