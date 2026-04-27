/*
 * Linker script for stm32f767
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

 _xram_start = 0xd0000000;
 _xram_size = 8M;

/*
 * NOTE: starting at 0x20000000 there's 128KB of DTCM (Data Tightly Coupled
 * Memory). Technically, we could use this as normal RAM as there's a way for
 * the DMA to access it, but the datasheet is unclear about performance
 * penalties for doing so. To avoid nonuniform DMA memory access latencies,
 * we leave this 128KB DTCM unused. This leaves us with 384KB of RAM.
 * TODO: test if we can just use the whole RAM
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x20000000, LENGTH = 128K
    ram2(wx)    : ORIGIN = 0xd0000000, LENGTH = 8M
}

INCLUDE ldscripts/miosix-flash-2ram-separate-irqstack.ld
