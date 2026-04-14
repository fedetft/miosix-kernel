/*
 * Linker script for stm32l151
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * The chip has 10K RAM but this board is for using a legacy firmware that's
 * hardcoded to use the top 2K for buffering, hence we lie to the linker script
 * and tell it there's only 8K. Don't copy this hack into any other board using
 * the same or a similar chip!
 */

MEMORY
{
    flash(rx)   : ORIGIN = 0x08000000, LENGTH = 64K
    ram(wx)     : ORIGIN = 0x20000000, LENGTH = 8K
}
