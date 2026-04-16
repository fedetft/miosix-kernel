Miosix can boot directly from a microcontroller flash memory without any
special bootloader and is usually flashed using JTAG/SWD or whichever ROM-based
bootloader the chip manufacturer provides.

This directory contains an optional bootloader that can be used on selected
stm32 boards to allow loading the kernel in RAM and execute it from there.
This bootloader is only useful for debugging purposes as the kernel will need to
be reloaded at every powercycle, and running code from RAM is slower than flash.

This bootloader allows to load code to external RAM, for easy development
without wearing out the STM32's FLASH which is only rated 10000 write cycles.
Ofcourse, it is not suitable for release code, since at every reboot the loaded
code gets lost. Please also note that running code from external RAM will be
~10 times slower than internal FLASH, but for code development and debugging
it is fine.

This bootloader contains code to forward interrupts @ 0x6800000 which is the
start of the external RAM.


Installation on PC
------------------
You'll need gcc (and g++), CMake and the boost libraries installed.
Windows build was not tested but should work. Linux and Mac OSX were tested.

run these commands with a shell opened in the
tools/booltloaders/stm32/pc_loader/pc_loader
directory

mkdir build
cd build
cmake ..
make
cp pc_loader ..


Installation on microcontroller
-------------------------------
Use your preferred method of loading code to the STM32, such as
serial bootloader or JTAG. Do not use the ST USB bootloader since it
reserves some space in the FLASH for the ST USB bootloader itself.
This Miosix bootloader is not PIC (position independent code)  so loading
it at another address will fail.


Loading code using the bootloader in external RAM
-------------------------------------------------
1) Make sure Miosix is configured to run from the STM32's external RAM:
in miosix/config/Makefile.inc options:

OPT_BOARD := stm32f103ze_stm3210e-eval

And in file miosix/config/board/stm32f103ze_stm3210e-eval/Makefile.inc

LINKER_SCRIPT := unikernel-all-in-xram.ld
XRAM := -D__ENABLE_XRAM -D__CODE_IN_XRAM

should be uncommented (no # at the start of the line)

2) Build Miosix as usual with

make

3) Connect the USART1 of the STM32 board with a serial cable to the PC
the expected device name for the serial port is /dev/ttyUSB0,
if not modify the line

PROG ?= $(KPATH)/../tools/bootloaders/stm32/pc_loader/pc_loader \
            /dev/ttyUSB0 $(if $(ROMFS_DIR), image.bin, main.bin)

in file miosix/arch/board/stm32f103ze_stm3210e-eval/Makefile.inc

4) then do a

make program && screen /dev/ttyUSB0

The bootloader should send data to the board, and run the binary.


Debugging code with Openocd in external RAM
-------------------------------------------
The bootloader should be loaded to the STM32's FLASH memeory to forward
interrupts @ 0x6800000, or Miosix will fail at the first interrupt.

Then run openocd in a shell:

openocd -f miosix/arch/board/stm32f103ze_stm3210e-eval/openocd.cfg

and in another shell type:

arm-miosix-eabi-gdb main.elf
target extended-remote :3333
monitor soft_reset_halt
load
c

After typing c miosix will start running. You can set breakpoints with
"break" and see variables with "print". For a more in-depth tutorial see
a gdb guide.
