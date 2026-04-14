/*
 * Linker script for stm32f303
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 256K
    ram1(wx)    : ORIGIN = 0x10000000, LENGTH = 8K
    ram2(wx)    : ORIGIN = 0x20000000, LENGTH = 40K
}
