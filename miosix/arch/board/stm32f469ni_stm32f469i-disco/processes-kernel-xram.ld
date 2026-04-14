/*
 * Linker script for stm32f469
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

_process_pool_size = 15872K; /* 512KB kernel memory, 15.5MB process pool */

/*
 * NOTE: starting at 0x10000000 there's 64KB of TCM.
 * Starting at 0x20000000 there's 320K of internal SRAM.
 * NOTE: the bootloader of this board also has the quirk that boot fails if the
 * initial stack pointer is in the TCM, we need to put the IRQ stack in SRAM
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 320K
    ram2(wx)    : ORIGIN = 0xc0000000, LENGTH = 16M  /* XRAM */
}

INCLUDE ldscripts/miosix-flash-2ram-separate-irqstack.ld
