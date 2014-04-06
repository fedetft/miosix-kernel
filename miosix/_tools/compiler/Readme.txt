This is the readme for installing the arm-miosix-eabi-gcc compiler,
required to build Miosix.
Currently this can only be done on Linux.
Instruction for Windows will be made available later.
===================================================================


Step 1
------
Copy this folder in a path without spaces, or compiling will fail.
Example:
/home/foo/temp                          OK
/home/foo/directory with spaces/temp    NO!!


Step 2
------
Install the following dependencies:
gcc, g++, make, ncurses, byacc, flex, texinfo, patch, gmp, mpfr, mpc, tar, unzip, lzip, libelf perl

For example, for Ubuntu/Kubuntu open a shell and type:
sudo apt-get install gcc g++ make libncurses5-dev byacc flex texinfo patch libgmp3-dev libmpfr-dev libmpc-dev tar unzip lzip libelf-dev perl

While on Fedora:
sudo yum intall gcc gcc-c++ make ncurses-devel byacc flex texinfo patch gmp-devel mpfr-devel libmpc-devel tar unzip lzip elfutils-libelf-devel perl

Note: these scripts require "sudo". If you use a distro like Fedora where sudo
is not enabled by default, use "visudo" to enable sudo for your account. You
can find information to do so on the Internet.


Step 3
------
Download the the required sources with the download script:

./download.sh


Step 4
------
After meeting these prerequisites, install:

./install-script.sh -j2
./cleanup.sh

Both scripts will prompt for root password at some point. It is normal,
since they need to write in /opt and /usr/bin.
The cleanup script won't remove the compressed files downloaded with the
download script. You might want to do so manually to save space on your disk.


Step 5
------
Test the compiler by typing in a shell

arm-miosix-eabi-gcc -v

If you get something like

bash: arm-miosix-eabi-gcc: command not found

it means something did not work.


Step 6
------
If required, also install lbftdi and OpenOCD for in circuit debugging.
There are no scripts for doing that, since it is rather independent on the
gcc version. On many distros it is also available thrugh package managers,
for example on Ubuntu/Kubuntu you can install it with

sudo apt-get install openocd

Note: openocd scripts shipped with Miosix were tested against openocd v0.6.1
and since the script syntax has the bad habit of changing incompatibly, they
might non work with newer or older versions of openocd. Either fix the scripts
or install version 0.6.1 if this happens.


Uninstalling the compiler
=========================
In case you need to uninstall the compiler (perhaps because you need to install
an upgraded version as part of a new Miosix release) you can run the

./uninstall.sh

script.
