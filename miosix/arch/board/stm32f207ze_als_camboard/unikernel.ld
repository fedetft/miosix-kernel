/*
 * Linker script for stm32f207
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * NOTE: This board has an external RAM, but used as hardcoded buffers in a
 * legacy application codebase
 */
MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 512K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 128K
}

INCLUDE ldscripts/miosix-flash-ram.ld
