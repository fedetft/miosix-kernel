/*
 * Linker script for Sony smartwatch
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

MEMORY
{
    /*
     * Taken from underverk's SmartWatch_Toolchain. The DFU bootloader sits
     * between 0x08000000 and 0x0800c000, that's 48K. Don't know why the code
     * can't be placed directly @ 0x0800c000, but by unpacking the original
     * firmware SmartWatch.dfu resulted in five different binary images:
     *  Base addr  | size    |
     * ------------+---------+--------------------------------------------------
     *  0x0800c000 | 0x5f00  | Seems the bluetooth driver, no interrupt table,
     *             |         | and funnily first 16K are zeros, probably
     *             |         | compiled as non-PIC code at 64K from flash base.
     *  0x08020000 | 0x1b578 | Is a firmware as it starts with an interrupt
     *             |         | table, don't know what it does.
     *  0x0803fffc | 0x188   | Discarding the first 4 bytes, the rest starts
     *             |         | @ 0x08040000 and is an interrupt table.
     *             |         | Bootloader jumps at this address. The interrupt
     *             |         | points to the next firmware image.
     *  0x08040200 | 0xb02b8 | First part is just pictures data, but there's
     *             |         | code at the end, and the lone interrupt table
     *             |         | points into it.
     *  0x080ffffc | 0x4     | Some magic number of some sort.
     */
    flash(rx)   : ORIGIN = 0x08040000, LENGTH = 768K
    /*
     * Taken from underverk's SmartWatch_Toolchain.
     * First 3K of RAM reserved, and last 2K of RAM reserved. Doesn't explain
     * why. TODO: try to use the entire RAM and see what happens.
     */
    ram(wx)     : ORIGIN = 0x20000c00, LENGTH = 128K-5K
}
