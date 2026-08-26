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

TODO: document how to attach with GDB to start a debug session.
