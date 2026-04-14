/*
 * Linker script for stm32l476
 * For compiling Miosix as a fluid kernel.
 * Uses ram1 for the kernel and kernelspace applications, ram2 for the process
 * pool for userspace applications.
 */

/* NOTE: _process_pool_size cannot be defined as all of ram2 is used */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 1M
    ram1(wx)    : ORIGIN = 0x10000000, LENGTH = 32K /* ram1 must be the small one */
    ram2(wx)    : ORIGIN = 0x20000000, LENGTH = 96K /* ram2 must be the large one */
}
