#!/bin/bash

# Script to build the gcc compiler required for Miosix.
#
# Building Miosix is officially supported only through the gcc compiler built
# with this script. This is because this script patches the compiler.
# Currently the patches are simple and not essential (even if they help
# reducing RAM footprint), but this will change in the future.
#
# This script will install arm-miosix-eabi-gcc in /opt, creating links to
# binaries in /usr/bin.
# It should be run without root privileges, but it will ask for the root
# password when installing files to /opr and /usr/local

function quit {
	echo $1
	exit 1
}

# Part 1: extract data

echo "Extracting files, please wait..."
tar --bzip2 -xf binutils-2.19.1.tar.bz2	|| quit ":: Error extracting binutils"
tar --bzip2 -xf gcc-4.4.2.tar.bz2		|| quit ":: Error extracting gcc"
tar -xf newlib-1.18.0.tar.gz			|| quit ":: Error extracting newlib"
tar --bzip2 -xf gdb-7.0.tar.bz2			|| quit ":: Error extracing gdb"
unzip lpc21isp_148_src.zip				|| quit ":: Error extracting lpc21isp"

# Part 2: patching gcc

patch -p0 < gcc-patches/eh_alloc.patch	|| quit ":: Failed patching eh_alloc.cc"
patch -p0 < gcc-patches/t-arm-elf.patch || quit ":: Failed patching t-arm-elf"

# Part 3: compile and install binutils

cd binutils-2.19.1
./configure --target=arm-eabi --prefix=/opt/arm-miosix-eabi --program-prefix=arm-miosix-eabi- --enable-interwork --enable-multilib --with-float=soft --disable-werror 2>../a.txt || quit ":: Error configuring binutils"

make all 2>../b.txt						|| quit ":: Error compiling binutils"

sudo make install 2>../c.txt			|| quit ":: Error installing binutils"

sudo ln -s /opt/arm-miosix-eabi/bin/* /usr/local/bin

cd ..

# Part 4: compile and install gcc-start

mkdir objdir
cd objdir
sudo ../gcc-4.4.2/configure --target=arm-eabi --prefix=/opt/arm-miosix-eabi --program-prefix=arm-miosix-eabi- --disable-shared --disable-libmudflap --disable-libssp --disable-nls --disable-libgomp --disable-libstdcxx-pch --with-float=soft --enable-threads --enable-languages="c,c++" --disable-wchar_t --with-newlib --with-headers=../newlib-1.18.0/newlib/libc/include 2>../d.txt || quit ":: Error configuring gcc-start"

sudo make all-gcc 2>../e.txt			|| quit ":: Error compiling gcc-start"

sudo make install-gcc 2>../f.txt		|| quit ":: Error installing gcc-start"

sudo ln -s /opt/arm-miosix-eabi/bin/* /usr/local/bin

cd ..

# Part 5: compile and install newlib

mkdir newlib-obj
cd newlib-obj

../newlib-1.18.0/configure --target=arm-eabi --prefix=/opt/arm-miosix-eabi --enable-interwork --enable-multilib --with-float=soft --disable-newlib-io-pos-args --disable-newlib-mb --enable-newlib-multithread CC_FOR_TARGET=arm-miosix-eabi-gcc CXX_FOR_TARGET=arm-miosix-eabi-g++ GCC_FOR_TARGET=arm-miosix-eabi-gcc AR_FOR_TARGET=arm-miosix-eabi-ar AS_FOR_TARGET=arm-miosix-eabi-as LD_FOR_TARGET=arm-miosix-eabi-ld NM_FOR_TARGET=arm-miosix-eabi-nm RANLIB_FOR_TARGET=arm-miosix-eabi-ranlib 2>../g.txt || quit ":: Error configuring newlib"

make 2>../h.txt							|| quit ":: Error compiling newlib"

sudo make install 2>../i.txt			|| quit ":: Error installing newlib"

cd ..

# Part 6: compile and install gcc-end

cd objdir

sudo make all 2>../j.txt				|| quit ":: Error compiling gcc-end"

sudo make install 2>../k.txt			|| quit ":: Error installing gcc-end"

cd ..

# Part 7: compile and install lpc21isp.c

gcc -o lpc21isp lpc21isp.c						|| quit ":: Error compiling lpc21isp"

sudo mv lpc21isp /opt/arm-miosix-eabi/bin		|| quit ":: Error installing lpc21isp"

sudo ln -s /opt/arm-miosix-eabi/bin/* /usr/local/bin

# Part 9: compile and install gdb

cd gdb-7.0

./configure --target=arm-eabi --prefix=/opt/arm-miosix-eabi --program-prefix=arm-miosix-eabi- --enable-interwork --enable-multilib --disable-werror 2>../l.txt || quit ":: Error configuring gdb"

make all 2>../m.txt						|| quit ":: Error compiling gdb"

sudo make install 2>../n.txt			|| quit ":: Error installing gdb"

sudo ln -s /opt/arm-miosix-eabi/bin/* /usr/local/bin

cd ..

# Last thing, remove this since its name is not arm-miosix-eabi-
sudo rm /usr/local/bin/arm-eabi-gcc-4.4.2

# The end.

echo ":: Successfully installed"

