
This example shows how to synchronize between a thread and a DMA peripheral.
To run this example, copy the content of this directory into the top level
directory.

This example supports the following two boards:

1) stm32f4discovery
===================

This board has a builting I2S audio DAC, just configure the kernel for
this board (OPT_BOARD := stm32f407vg_stm32f4discovery in Makefile.inc),
make; make program and plug headphone jack to the board's connector.

2) stm32vldiscovery
===================

Follow the kernel configuration procedure for this board here:

http://miosix.org/boards/stm32vldiscovery-config.html

To be able to hear the audio file, build one of the two circuits found in
the circuit.jpeg file. The top one is a speaker amplifier, the bottom one
is a filter whose output is suitable to drive an amplified speaker
(such as a PC speaker).
