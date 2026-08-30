This example demonstrates the kernel gdbserver feature that can be used to
debug userspace processes.

To test it select a supported board, e.g. the stm3220g-eval or stm32f4discovery,
then uncomment the following options:
#define WITH_FILESYSTEM
#define WITH_PROCESSES
#define PROCESS_DEBUGGER

And in the board option enable a processes compatible linker script, and enable
the AUX_SERIAL, as two serial ports are needed, one for the kernel printf/scanf
and one to connect via GDB.

To start a debug session, launch GDB specifying the unstripped ELF to load debug
symbols, in this example the file is generated as "process_template/bin/hello":
$ arm-miosix-eabi-gdb --baud115200 process_template/bin/hello

Then attach to the server running on miosix using the aux serial adapter,
e.g. aux serial on /dev/ttyUSB0:
(gdb) target extended-remote /dev/ttyUSB0

To attach to an existing user process, refer to it using its process ID:
(gdb) attach <pid>

To spawn a new process and debug it, specify the executable's path,
e.g. /bin/hello, then launch it with the start command to stop it at the first
instruction of main:
(gdb) set remote exec-file /bin/hello
(gdb) start [arg1 arg2 ...]

Alternatively, set break/watchpoints and run the program until one is
encountered:
(gdb) breakpoint main.cpp:5
(gdb) run [arg1 arg2 ...]

To end a debug session, detach from a process use the detach command:
(gdb) detach
