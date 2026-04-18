/*
 * Linker script for stm32f429
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x10000000, LENGTH = 64K
    ram2(wx)    : ORIGIN = 0x20000000, LENGTH = 192K
}

/* Mark XRAM even if it's not used in memory map, may be used for framebuffer */
_xram_start = 0xd0000000;
_xram_size  = 8M;

INCLUDE ldscripts/miosix-flash-2ram-separate-heap.ld
