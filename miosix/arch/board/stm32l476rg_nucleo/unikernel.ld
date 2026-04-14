/*
 * Linker script for stm32l476
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 1M
    ram1(wx)    : ORIGIN = 0x10000000, LENGTH = 32K /* ram1 must be the small one */
    ram2(wx)    : ORIGIN = 0x20000000, LENGTH = 96K /* ram2 must be the large one */
}
