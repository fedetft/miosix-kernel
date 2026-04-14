/*
 * Linker script for stm32f407
 * For compiling Miosix as a fluid kernel.
 * Separates RAM in two partitions, the first used by the kernel and
 * kernelspace applications, the second being the process pool for userspace
 * applications.
 */

/*
 * NOTE: The stm32f407 has the architectural quirk that ram1 is a TCM that is
 * not accessible by the DMA. As such, we can't use the entire ram2 for the
 * process pool, since that would leave the kernel with no DMA-capable RAM!
 * The DMA-based drivers for stm32f407 are hardcoded to assume that only heap
 * memory is DMA-capable, so with this chip we *must* place the kernel keap in
 * ram2!
 * We thus split ram2 into 32K for the kernel heap and 96K for the process pool.
 */
_process_pool_size = 96K;

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 1M
    ram1(wx)    : ORIGIN = 0x10000000, LENGTH = 64K
    ram2(wx)    : ORIGIN = 0x20000000, LENGTH = 128K
}
