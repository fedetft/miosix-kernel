/*
 * Linker script for stm32f429
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * NOTE: The stm32f429 has the architectural quirk that ram1 is a TCM that is
 * not accessible by the DMA. As such, we can't use the entire ram2 for the
 * process pool, since that would leave the kernel with no DMA-capable RAM!
 * The DMA-based drivers for stm32f407 are hardcoded to assume that only heap
 * memory is DMA-capable, so with this chip we *must* place the kernel keap in
 * ram2!
 * We thus split ram2 into 64K for the kernel heap and 128K for the process pool.
 */
_process_pool_size = 128K;

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
