/*
 * Linker script for RP2040
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x10000000, LENGTH = 2M
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 264K
}

INCLUDE ldscripts/miosix-flash-ram-rp2040.ld
