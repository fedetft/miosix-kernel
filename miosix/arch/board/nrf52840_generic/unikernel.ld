/*
 * Linker script for nrf52840
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x00000000, LENGTH = 1M
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 256K
}
