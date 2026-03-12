
# The Miosix toolchain

The Miosix toolchain is a set of scripts and patches to build a GCC-based cross-compiler able to compile the Miosix fluid kernel. For the ARM architecture, the various GNU-based tools (gcc, g++, gdb, ld, as, ...) are prefixed with `arm-miosix-eabi-` (so for example the C compiler will be `arm-miosix-eabi-gcc`). Additionally, to support building userspace programs the installation scripts also build the `mx-postlinker` utility program and the libsyscalls library (since in a fluid kernel the libc is linked to both the kernel and userspace programs, it does **not** contain syscalls, which instead live in a separate library linked only when compiling programs. When compiling the kernel, undefined references to syscall functions are resolved by linking to kercalls in the kernel).

## Pre-compiled release builds

We provide a [pre-compiled release](https://miosix.org/wiki/index.php?title=Miosix_Toolchain) of the Miosix toolchain, and expect most users will just want to install that. We provide builds for Linux (x86_64), Windows (x86_64) and Mac OS (ARM and x86_64). If you instead prefer to build the toolchain yourself, keep on reading.

## Builing the Miosix toolchain for Linux

Building the Miosix toolchain requires about 20GB of free disk space for temporary files, 8GB of RAM or more, and about one hour of build time, on a relatively modern computer. The build time mostly depends on the number of CPU cores available. To speed up compilation further it is possible to do the build in a ramdisk (`ramdisk.sh` script) but this requires 32GB of RAM.

### Step 1: Prerequisites

Make sure this directory (the one containing `install-script.sh`) is in a path without spaces, or compiling will fail.
Example:

- `/home/foo/temp`                          OK
- `/home/foo/directory with spaces/temp`    NO!!

Also make sure you ***do not*** have a previous version of the Miosix toolchain installed system-wide (or in your `PATH`), either compiled from source or installed pre-compiled.
If you type `arm-miosix-eabi-gcc` in your shell you should get a no such program error.

If you have a previous toolchain installed system-wide, here's how to uninstall it:

```
cd /opt/arm-miosix-eabi
./uninstall.sh
```

### Step 2: Dependencies

Install the following dependencies:
xz-utils, curl, gcc, g++, make, ncurses, byacc, flex, texinfo, patch, tar, unzip, lzip, libelf perl libexpat.

For example, for Ubuntu/Kubuntu open a shell and type:

```
sudo apt install xz-utils curl gcc g++ make libncurses5-dev byacc flex texinfo patch tar unzip lzip libelf-dev perl libexpat1-dev
```

While on Fedora:

```
sudo dnf install xz-utils curl gcc gcc-c++ make ncurses-devel byacc flex texinfo patch tar unzip lzip elfutils-libelf-devel perl expat-devel
```

Note: the install scripts require `sudo`, unless you want to intall the compiler locally.
If you use a distro like Fedora where sudo is not enabled by default, you need to enable sudo for your account.

Note: some recent version of Linux operating systems no longer provide an `/opt` directory. If this is the case in your system, create it with `sudo mkdir /opt`.

### Step 3: Download sources

Download the the required sources with the download script:

```
./download.sh
```

### Step 4: Choose how to compile the toolchain

The `install-script.sh` file includes some configuration options. For a personal (non-redistributable) build, you can choose whether to install the compiler system-wide (in the `/opt` directory, with symlinks to `/usr/bin`) or locally (in an `arm-miosix-eabi` subdirectory of the directory you're building the compiler from). In the latter case, `sudo` is not required, but you will have to add the `arm-miosix-eabi/bin` subdirectory to your `PATH` variable to be able to use the compiler.

The configuration is done by commenting/uncommenting lines in the `install-script.sh` file.
The default is a system-wide build, with those lines uncommented:

```
PREFIX=/opt/arm-miosix-eabi
DESTDIR=
SUDO=sudo
```

If you prefer a local build, comment the lines above and uncomment these lines:

```
PREFIX=`pwd`/arm-miosix-eabi
DESTDIR=
SUDO=
```

Finally, if you want to take advantage of the ramdisk (this makes sense only for a system-wide install, as the locally-installed compiler will be deleted together with the ramdisk when you shut down the computer...), do a

```
./ramdisk.sh
cd ramdisk
```
and proceed the installation from the `ramdisk` directory intead of the parent directory.

### Step 5: Build

Compile and install with

```
./install-script.sh -j`nproc`
./cleanup.sh
```

If you're installing system-wide, both scripts will prompt for root password at
some point. It is normal, since they need to write in `/opt` and `/usr/bin`.
The cleanup script won't remove the compressed files downloaded with the
download script. You might want to do so manually to save space on your disk.


### Step 6: Testing

Test the compiler by typing in a shell

```
arm-miosix-eabi-gcc --version
```

If you get something like

```
bash: arm-miosix-eabi-gcc: command not found
```

it means something did not work.


### Step 7: Additional tools

The Miosix toolchain is just the compiler/debugger. For developing for embedded systems you'll likely need some other tools such as OpenOCD for in circuit debugging, and tools for flashing microcontrollers such as `st-flash`, `dfu-util`, etc.

There are no scripts for doing that, since there are no dependencies with Miosix. On many distros these tools also available through package managers, for example on Ubuntu/Kubuntu you can install them with

```
sudo apt-get install openocd stlink-tools dfu-uitl`
```

The above command is just an example, the additional tools you'll need depend on the microcontroller you're targeting.

## Builing the Miosix toolchain for Mac OS

TODO: This needs documenting.

## Builing the Miosix toolchain for Windows

Building the Miosix toolchain ***for*** Windows ***from*** Windows is not supported. You'll need a Linux machine (or virtual machine) and follow the instructions for making a redistributable Windows build from Linux. For Windows it's much easier to install a pre-compiled build you can dowload from [miosix.org](https://miosix.org).



# Making redistributable builds

This part of the readme is mostly of interest to maintainers to produce the redistributable builds that are uploaded to [miosix.org](https://miosix.org).

## Making a Linux redistributable build

The main issue when making Linux redistributable builds is that the glibc C library is not backwards compatible. This means that if you make the build, let's say on Ubuntu 24.04, the produced binaries will most likely not work on previous versions of Ubuntu or on other distros that use older versions of glibc, throwing glibc-related errors and refusing to run. We work around this issue by making redistributable builds on older operating systems releases, though at every release we are forced to drop support for some older system.

Compiling a redistributable for Linux requires the following additional steps:

### Download additional files

```
./installers/additional-download.sh
```

### Select a redistributable Linux build

Comment the defaults in `install-script.sh` and uncomment the following options:

```
PREFIX=/opt/arm-miosix-eabi
DESTDIR=`pwd`/dist
SUDO=
```

and

```
BUILD=
HOST=x86_64-linux-gnu
```

## Making a Mac OS redistributable build

TODO: This needs documenting.

## Making a Windows redistributable build

Redistributable Windows build are done through "Canadian cross compiling", where you're building on a platform (Linux) a compiler that will run on a second platform (Windows) and will produce binaries for a third platfom (Miosix).

Since you'll need to build the standard libraries (libgcc, libc, libstdc++, ...) too, and the compiler you're building does not run on the platform you're building from, you'll need to install the Linux compiler system-wide first.
However, you'll need a ***special*** version of the Linux compiler, one where `stubs.o` has not been removed from libc.

What's `stubs.o`, you may ask? Here's the long version of the explanation. The standard libraries are built with autotools, whose configuration system relies on compiling and linking small programs to test for features.
That's a problem because the kernelspace multilibs of the standard libraries are meant to be linked with the Miosix kernel, not to produce standalone binaries.
Attempting to link a program using those multilibs will just fail with undefined references to syscalls functions.
As a result the linking of the autotools-generated programs will fail, halting the build. We work around this issue by adding to libc a special file, `stubs.o` that contains stubs for all syscalls (this is part of the newlib patches).
This file is compiled as part of libc, and is only used while the compiler is built to make the autoconf tests work when building the rest of the standard libraries beyond libc (libgcc, libstdc++, libatomic, libgomp).
This stubs file is then removed by `install-script.sh`, so after the compiler is built, libc no longer contains those stubs.

This solution works transparently on all platforms, except when making a Windows redistributable build. In this case, the system-wide installed Linux compiler needs to still have the `stubs.o` in libc as these are used for feature checks. We tried to convince the build scripts of gcc to use the version of the libc we're about to bundle in the Windows compiler (which still has `stubs.o`), instead of the one installed system-wide, but they insist on using the wrong version (that's also the reason why you ***must*** uninstall an older Miosix toolchain when builing the Mioix toolchain). So, as a workaround, we install system-wide a special version of the Linux compiler with `stubs.o` still present just for the purpose of building the Windows compiler. This Linux compiler should ***not*** be used for building the Miosix kernel (where the stubs would cause issues), it is only useful as an intermediary tool for building the Windows compiler.

After this rather long explanation about internal details of the build process, here's how to build a Windows redistribtuable compiler.

### Install dependencies

```
./installers/additional-download.sh
```

Install Wine, mingw and the innosetup that has been downloaded at the previous step.

```
sudo apt install wine mingw-w64
sudo -k
wine downloaded/innosetup.exe
```

### Build the system-wide Linux compiler with stubs

In `install-script.sh` change

```
STRIP_STUBS_FROM_LIBC=true
```

to

```
STRIP_STUBS_FROM_LIBC=false
```

and make sure the system-wide Linux installation is selected

```
PREFIX=/opt/arm-miosix-eabi
DESTDIR=
SUDO=sudo
```
and

```
BUILD=
HOST=
```

Then build the local compiler as usual. It should end up in `/opt`. Don't forget to `./cleanup.sh`

Finally, get ready for building the Windows compiler: revert `install-scripts.sh` to stripping stubs.

```
STRIP_STUBS_FROM_LIBC=true
```

and select the Windows redistributable

```
PREFIX=/opt/arm-miosix-eabi
DESTDIR=`pwd`/dist
SUDO=
```

and

```
BUILD=
HOST=x86_64-w64-mingw32
```

Then run

```
./install-script.sh -j`nproc`
```

At the end the compiler will be placed in `installers/windows/Output`.
