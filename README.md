# Welcome to the Miosix Kernel

Miosix (Minimal Operating System with POSIX) is a real-time operating system
for 32-bit microcontrollers. It implements a new paradigm, the
**fluid kernel** [1]: applications can be written to run in kernel space
(*unikernel*-like), or in userspace with the same set of APIs.

Miosix additionally features:

- Focus on **C++** support, not just C
- Support for **standard APIs**:
  - C and C++ standard libraries (STL included)
  - POSIX thread API
  - C++ exception handling
- **SMP Multicore** support
- **Memory protection** for processes on microcontrollers with MPUs
- Built-in **virtual filesystem** (VFS), integrated with the libc, supporting:
  - FAT32 for external drives like SD cards
  - LittleFs for non-volatile internal storage, like SPI Flash chips
  - RomFs for read-only on-code-flash file storage
- **Flexible scheduler subsystem** with the ability to choose between
  - Priority scheduling
  - Earliest Deadline First (EDF) scheduling
  - Control-based scheduling [2]
- **Scalable code size**, configurable by removing features you don't need
  (~100KiB as a fluid kernel with userspace processes support and virtual filesystem, down to ~10KiB as a unikernel with all optional features removed)

## Getting started

You can find information on how to configure and use the kernel
at the following url: https://miosix.org

Miosix development goes on in the testing and unstable branches.
The master branch points to the latest stable release.

## Directory structure

Directories marked with `[x]` contain code or configuration files that are used
to build the kernel while directories marked with `[ ]` contain other stuff
(documentation, examples).

```
[x]                          //Root directory, contains the application code
 |
 +--[ ] doc                  //Kernel documentation (doxygen + pdf + txt)
 |
 +--[ ] examples             //Contains some example applications
 |
 +--[ ] templates            //Contains application templates
 |   |
 |   +--[ ] simple           //Template for kernel-mode applications
 |   |
 |   +--[ ] processes        //Template for kernel and user-mode applications
 |
 +--[ ] tools                //Miscellaneous tools for Miosix users
 |   |
 |   +--[ ] bootloaders      //Custom bootloaders for some boards
 |   |                       //Mostly used for running Miosix in RAM without a
 |   |                       //debugger
 |   |
 |   +--[ ] compiler         //Contains scripts used to build GCC with the
 |   |                       //patches required to compile the kernel
 |   |
 |   +--[ ] testsuite        //Contains the kernel testsuite
 |   .
 |   .
 |
 +--[x] miosix               //Contains all Miosix OS code
     |
     +--[x] cmake            //CMake files used by the Miosix CMake build system
     |
     +--[x] config           //Default template configuration makefile fragments
     |   |                   //and header files
     |   |
     |   +--[x] board        //Board-specific configuration
     |
     +--[x] e20              //Contains the kernel's event driven API
     |
     +--[x] filesystem       //Filesystem code
     |
     +--[x] interfaces       //Contains interfaces between kernel and
     |                       //architecture dependent code
     |
     +--[x] interfaces_private //Private interfaces for the kernel
     |
     +--[x] kercalls         //Implementation of libc primitives for kernel mode
     |
     +--[x] kernel           //Contains the architecture independent kernel part
     |   |
     |   +--[x] scheduler    //Contains the various schedulers
     |       |
     |       +--[x] priority
     |       |
     |       +--[x] control_based
     |       |
     |       +--[x] edf
     |
     +--[x] ldscripts        //Base linker script files
     |                       //Boards include linker script fragments from here
     |                       //to make full linker scripts
     |
     +--[x] tools            //Scripts used by the Miosix build system
     |
     +--[x] util             //Contains architecture independent utility code
     |
     +--[x] arch             //Contains architecture dependent code
         |
         +--[x] board        //Board-specific code and definitions
         |   |
         |   +--[x] lpc2138_miosix_board //Name of folder is the board name
         |   |   |
         |   |   +--[x] interfaces-impl  //Implmentation of miosix/interfaces
         |   |   |
         |   |   +--[x] Makefile.inc     //Build system configuration
         |   |   |
         |   |   +--[x] ...              //Linker scripts,
         |   .                           //openocd scripts
         |   .   Other boards
         |
         +--[x] chip         //Chip-family-specific code and definitions
         |   |
         |   +--[x] lpc2000  //Name of folder is the chip family name
         |   |   |
         |   |   +--[x] interfaces-impl  //Implmentation of miosix/interfaces
         |   |   |
         |   |   +--[x] Makefile.inc     //Build system configuration
         |   .
         |   .   Other chip families
         |
         +--[x] cpu          //CPU-architecture-specific code and definitions
         |   |
         |   +--[x] armv4    //Name of folder is the CPU architecture
         |   |   |
         |   |   +--[x] interfaces-impl  //Implmentation of miosix/interfaces
         |   .
         |   .   Other architectures
         |
         +--[x] drivers      //Collection of drivers
         |
         +--[x] CMSIS
```

## References

[1] F. Terraneo and D. Cattaneo, "Fluid Kernels: Seamlessly Conquering the
Embedded Computing Continuum," in IEEE Transactions on Computers, vol. 74,
no. 12, Dec. 2025, doi: https://doi.org/10.1109/TC.2025.3605745.
[2] M. Maggio, F. Terraneo, and A. Leva, "Task scheduling: A control-theoretical
viewpoint for a general and flexible solution," in ACM Trans. Embed. Comput.
Syst., vol. 13, no. 4, Nov. 2014, doi: https://doi.org/10.1145/2560015.
