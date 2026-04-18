/*
 * Linker script for stm32f207
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 512K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 128K
}

/*
 * NOTE: This board has an external RAM, but used as hardcoded buffers in a
 * legacy application codebase
 */
_xram_start = 0x60000000;
_xram_size  = 1M;

INCLUDE ldscripts/miosix-flash-ram.ld
