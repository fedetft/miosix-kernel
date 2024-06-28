This tool provides an alternative way to flash RP2040 microcontrollers as usb
thumb drives, allowing to convert bin files into the uf2 file format the RP2040
expects.

By default it is not used by the Miosix build system, but it can be used either

1) manually, like this (you need to compile bin2uf2 with cmake first)
./bin2uf2 -s 0x10000000 -f 0xe48bff56 -p main.bin -o main.uf2

2) the Miosix build system can be hacked to also build uf2 files, see the
example Makefile in this directory. It is meant to replace the Miosix top-level
makefile, and it will both build the bin2uf2 program and use it to produce
the .uf2 file of the kernel image
