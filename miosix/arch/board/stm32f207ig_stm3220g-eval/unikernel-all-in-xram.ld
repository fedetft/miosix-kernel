/*
 * Linker script for stm32f207
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    ram(wx)     : ORIGIN = 0x64000000, LENGTH = 2M
}

/* Mark ram as being an external RAM */
_xram_start = ORIGIN(ram);
_xram_size  = LENGTH(ram);

INCLUDE ldscripts/miosix-all-in-xram.ld
