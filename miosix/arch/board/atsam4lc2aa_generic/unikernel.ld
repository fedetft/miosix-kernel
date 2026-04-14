/*
 * Linker script for ATSAM4L
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    /* 16KB are reserved for the SAM-BA bootloader */
    flash(rx)   : ORIGIN = 0x00004000, LENGTH = 112K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 32K
}
