/*
 * Linker script for stm32f207
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 1M
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 128K
    ram2(wx)    : ORIGIN = 0x60000000, LENGTH = 512K
}

INCLUDE ldscripts/miosix-flash-2ram-separate-heap.ld
