/*
 * Linker script for stm32f103
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    /* First 10K taken up by bootloader */
    flash(rx)   : ORIGIN = 0x2800,     LENGTH = 512K-10K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 64K
}
