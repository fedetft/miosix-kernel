/*
 * Linker script for stm32f103
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 512K
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 64K
    ram2(wx)    : ORIGIN = 0x68000000, LENGTH = 1M
}

/* Mark ram2 as being an external RAM */
_xram_start = ORIGIN(ram2);
_xram_size  = LENGTH(ram2);

INCLUDE ldscripts/miosix-flash-2ram-separate-heap.ld
