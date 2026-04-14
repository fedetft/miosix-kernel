/*
 * Linker script for efm32gg332f1024
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

_irq_stack_size = 768; /* Drivers require bigger IRQ stack than Miosix default */
_process_pool_size = 96K;

MEMORY
{
    flash(rx)   : ORIGIN = 0x00004000, LENGTH = 1M-16K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 128K
}

INCLUDE ldscripts/miosix-flash-ram.ld
