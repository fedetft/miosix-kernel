/*
 * Linker script for stm32f469
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

/* NOTE: _process_pool_size cannot be defined as all of ram2 is used */

/*
 * NOTE: starting at 0x10000000 there's 64KB of TCM.
 * Starting at 0x20000000 there's 320K of internal SRAM.
 * NOTE: the bootloader of this board also has the quirk that boot fails if the
 * initial stack pointer is in the TCM, we need to put the IRQ stack in SRAM
 * TODO: the previous linker script used the TCM for .data/.bss
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 320K
    ram2(wx)    : ORIGIN = 0xc0000000, LENGTH = 16M  /* XRAM */
}

/* Mark ram2 as being an external RAM */
_xram_start = ORIGIN(ram2);
_xram_size  = LENGTH(ram2);

INCLUDE ldscripts/miosix-flash-2ram-separate-pool.ld
