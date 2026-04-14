/*
 * Linker script for stm32f103
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    ram(wx)     : ORIGIN = 0x68000000, LENGTH = 1M
}
