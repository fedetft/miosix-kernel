/*
 * Linker script for efm32gg332f1024
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

_irq_stack_size = 768; /* Drivers require bigger IRQ stack than Miosix default */

MEMORY
{
    flash(rx)   : ORIGIN = 0x00004000, LENGTH = 1M-16K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 128K
}
