/*
 * Linker script for stm32f207
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    ram(wx)     : ORIGIN = 0x64000000, LENGTH = 2M
}

INCLUDE ldscripts/miosix-all-in-xram.ld
