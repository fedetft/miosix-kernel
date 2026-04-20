/*
 * Linker script for stm32f429
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

/* No _process_pool_size as it's fixed to the size of ram2 */

/*
 * NOTE: starting at 0x10000000 there's 64KB of TCM.
 * Starting at 0x20000000 there's 192K of internal SRAM.
 * Since this linker script is for placing the process pool in XRAM, we use the
 * SRAM for the kernel
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 192K
    ram2(wx)    : ORIGIN = 0xd0000000, LENGTH = 6M  /* XRAM */
}

/*
 * NOTE: the SDRAM is 8MB, but this linker script only uses the lower 6.
 * The upper 2MB are reserved as framebuffers for the LCD. The choice to
 * allocate this space is due to the SDRAM architecture, composed of 4 banks of
 * 2MB each. Reserving a bank for the LCD allows the software running on the
 * board and the LCD to operate (almost) independently.
 * Regardless, we want to configure the MPU to span the entire XRAM
 */
_xram_start = ORIGIN(ram2);
_xram_size  = 8M;

INCLUDE ldscripts/miosix-flash-2ram-separate-pool.ld
