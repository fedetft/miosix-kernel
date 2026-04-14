/*
 * Linker script for STM32U585
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 2M
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 768K
}
