This is the readme for installing the arm-miosix-eabi-gcc compiler,
required to build Miosix.
Currently this can only be done on Linux, even when compiling a
compiler that will work for Windows.
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
gcc, g++, make, ncurses, byacc, flex, texinfo, patch, tar, unzip, lzip, libelf perl libexpat

For example, for Ubuntu/Kubuntu open a shell and type:
sudo apt-get install gcc g++ make libncurses5-dev byacc flex texinfo patch tar unzip lzip libelf-dev perl libexpat1-dev

While on Fedora:
sudo yum intall gcc gcc-c++ make ncurses-devel byacc flex texinfo patch tar unzip lzip elfutils-libelf-devel perl libexpat-devel

Note: these scripts require "sudo" unless you want to intall the compiler locally.
If you use a distro like Fedora where sudo is not enabled by default, use "visudo" to enable sudo for your account.

Step 3
------
Download the the required sources with the download script:

./download.sh


Step 4
------
After meeting these prerequisites, install:

./install-script.sh -j`nproc`
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
If required, also install OpenOCD for in circuit debugging.
There are no scripts for doing that, since it is rather independent on the
gcc version. On many distros it is also available thrugh package managers,
for example on Ubuntu/Kubuntu you can install it with

sudo apt-get install openocd

Uninstalling the compiler
=========================
In case you need to uninstall the compiler (perhaps because you need to install
an upgraded version as part of a new Miosix release) you can run the

./uninstall.sh

script.
