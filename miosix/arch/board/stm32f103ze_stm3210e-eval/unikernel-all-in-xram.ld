/*
 * Linker script for stm32f103
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    ram(wx)     : ORIGIN = 0x68000000, LENGTH = 1M
}

/* Mark ram as being an external RAM */
_xram_start = ORIGIN(ram);
_xram_size  = LENGTH(ram);

INCLUDE ldscripts/miosix-all-in-xram.ld
