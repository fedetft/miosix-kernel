/*
 * Linker script for stm32f429
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram1(wx)    : ORIGIN = 0x10000000, LENGTH = 64K
    ram2(wx)    : ORIGIN = 0x20000000, LENGTH = 192K
    sgm(rw)     : ORIGIN = 0x40024000, LENGTH = 4K /* safeguard memory (.preserve) */
}

INCLUDE ldscripts/miosix-flash-3ram-separate-heap-sgm.ld
