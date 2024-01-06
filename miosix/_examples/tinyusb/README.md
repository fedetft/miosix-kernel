# TinyUSB example

This example shows how to use TinyUSB (https://www.tinyusb.org/) with
Miosix, implementing a simple application which exposes via USB a virtual
serial port which echoes incoming characters.

As is, the example is made for a stm32f4discovery board, but it can be adapted
for other boards.

## Structure of the example

The `tinyusb` directory contains a separate Makefile just for building
TinyUSB separately, creating a static library called `libtinyusb.a`. This is
done to ensure the TinyUSB sources are compiled as C++, which simplifies
the integration with Miosix.

The TinyUSB source code should be cloned (either using git normally or by
adding a submodule) inside the `tinyusb` directory. Thus, the final directory
structure will be:

```
main.cpp
Makefile
tinyusb\
  Makefile
  tusb_config.h
  tusb_os_custom.h
  cmsis_stubs\
    stm32f2xx.h
    ...
  tinyusb\          <- Root of the TinyUSB source
    docs\
    examples\
    hw\
    lib\
    src\
    test\
    tools\
    LICENSE
    README.rst
    ...
```

The example was developed based on TinyUSB 0.15.0 (commit SHA
86c416d4c0fb38432460b3e11b08b9de76941bf5) and may not work with newer versions.
When updating TinyUSB, always remember to also update the Makefile to include
any new source code file in TinyUSB (or remove any file that was removed).

The `tinyusb/tusb_os_custom.h` file contains the Miosix-specific implementations
for the concurrency primitives required by TinyUSB.

The `tinyusb/tusb_config.h` file configures which class drivers and ports are
enabled, and which device driver should be used by TinyUSB. Always make sure
that the definition of `CFG_TUSB_MCU` is consistent with the board selection
made in Miosix's `Makefile.inc`.

The `cmsis_stubs` directory contains files named in the same way as the
STM32 CMSIS device headers, for use by TinyUSB which includes these files
directly in its device drivers. Unfortunately, due to the directory structure
of Miosix, these files cannot be included by TinyUSB directly without issues;
one is supposed to include `interfaces/arch_registers.h` instead. Using these
files we allow TinyUSB to include `interfaces/arch_registers.h` without
needing to modify TinyUSB's source code. Only a few example stub files have been
included; add new stubs as needed for your project.

## Compiling this example

To try out this example, get a stm32f4-discovery board, then follow these steps:

1. Copy and paste the `tinyusb` directory, the `main.cpp` and `Makefile` in the
   root of the repository
2. Select the `stm32f407vg_stm32f4discovery` board in `Makefile.inc`
3. Disable the safety guard in `miosix_settings.h`
4. Perform the following commands in the root of the repository:
   ```
   cd tinyusb
   git clone https://github.com/hathach/tinyusb.git
   cd tinyusb
   git checkout 86c416d4c0fb38432460b3e11b08b9de76941bf5
   ```
5. Compile normally by running `make clean; make` in the root of the repository.

## Using this example as a template

To use TinyUSB in a new Miosix project, follow these steps:

1. Copy and paste the `tinyusb` directory of this example in your project
2. In your Makefile, add the following subdirectories, libraries and includes:
   ```
   SUBDIRS      := ... tinyusb
   LIBS         := ... tinyusb/libtinyusb.a
   INCLUDE_DIRS := ... -I./tinyusb/tinyusb/src -I./tinyusb
   ```
3. Clone TinyUSB in the `tinyusb` directory
4. Copy and paste the interrupt handle definition from the example `main.cpp`
   in your main
5. Customize `tusb_config.h` to select the class driver, target device, and add
   callbacks in your main to configure the USB descriptors. Follow TinyUSB's
   documentation for performing this step.
6. Done!
