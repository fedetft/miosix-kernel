/*
 * Linker script for stm32f746
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * NOTE: starting at 0x20000000 there's 64KB of DTCM (Data Tightly Coupled
 * Memory). Technically, we could use this as normal RAM as there's a way for
 * the DMA to access it, but the datasheet is unclear about performance
 * penalties for doing so. To avoid nonuniform DMA memory access latencies,
 * we leave this 64KB DTCM unused. This leaves us with 256KB of RAM.
 * TODO: in the processes linker script the entire kernel was moved in the
 * dtcm and it worked, maybe reconsider using the dtcm someday
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 1M
    /* ram(wx)     : ORIGIN = 0x20000000, LENGTH = 320K */
    ram(wx)     : ORIGIN = 0x20010000, LENGTH = 256K
}
