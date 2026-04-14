/*
 * Linker script for stm32f746
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

_process_pool_size = 256K;

/*
 * NOTE: starting at 0x20000000 there's 64KB of DTCM (Data Tightly Coupled
 * Memory). We use this to store the kernel as there's a way for the DMA to
 * access it, even though the datasheet is unclear about performance
 * penalties for doing so. This leaves us with 256KB of RAM for the process pool
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 1M
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 320K
}

INCLUDE ldscripts/miosix-flash-ram.ld
