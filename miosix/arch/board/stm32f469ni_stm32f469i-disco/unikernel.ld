/*
 * Linker script for stm32f469
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * The bootloader of this board has the quirk that boot fails if the initial
 * stack pointer is in the TCM, thus we need to put the IRQ stack with the heap
 * in ram2
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x10000000, LENGTH = 64K
    ram2(wx)    : ORIGIN = 0x20000000, LENGTH = 320K
}

INCLUDE ldscripts/miosix-flash-2ram-separate-data-bss.ld
