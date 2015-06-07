This example shows a multibuffered way to stream data to an SD card.
Writing a large 32KB buffer at a time helps improve write speed.

This example allocates a 48KB fifo buffer as a global variable and
two 32KB buffers on the heap, so make sure you have enough RAM, or
reduce the buffers. The stm32f4discovery the board where it was developed.

The program streams a 64Byte record every millisecond, resulting in
a 64KB/s data rate. On some boards it is also possible to achieve much faster
data rates.
