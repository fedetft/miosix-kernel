#!/bin/bash

# Script to build the gcc compiler required for Miosix.
#
# Building Miosix is officially supported only through the gcc compiler built
# with this script. This is because this script patches the compiler.
# Starting form Miosix 1.58 the use of the arm-miosix-eabi-gcc compiler built
# by this script has become mandatory due to patches related to posix threads
# in newlib. The kernel *won't* compile unless the correct compiler is used.
#
# This script will install arm-miosix-eabi-gcc in /opt, creating links to
# binaries in /usr/bin.
# It should be run without root privileges, but it will ask for the root
# password when installing files to /opt and /usr/bin

# Uncomment if installing globally on the system
INSTALL_DIR=/opt
SUDO=sudo
# Uncomment if installing locally, sudo isn't necessary
#INSTALL_DIR=`pwd`/gcc 
#SUDO=

# Program versions
BINUTILS=binutils-2.21.1
GCC=gcc-4.5.2
NEWLIB=newlib-1.19.0
GDB=gdb-7.0.1

export PATH=$INSTALL_DIR/arm-miosix-eabi/bin:$PATH

function quit {
	echo $1
	exit 1
}

#
# Part 1: extract data
#

echo "Extracting files, please wait..."
tar -xjf $BINUTILS.tar.bz2	|| quit ":: Error extracting binutils"
tar -xjf $GCC.tar.bz2		|| quit ":: Error extracting gcc"
tar -xf  $NEWLIB.tar.gz		|| quit ":: Error extracting newlib"
tar -xjf $GDB.tar.bz2		|| quit ":: Error extracing gdb"
unzip lpc21isp_148_src.zip	|| quit ":: Error extracting lpc21isp"
mkdir log

#
# Part 2: applying patches
#

patch -p0 < gcc-patches/svc.patch		|| quit ":: Failed patching binutils"
patch -p0 < gcc-patches/gcc.patch		|| quit ":: Failed patching gcc"
patch -p0 < gcc-patches/newlib.patch	|| quit ":: Failed patching newlib"

#
# Part 3: compile and install binutils
#

cd $BINUTILS
./configure \
	--target=arm-eabi \
	--prefix=$INSTALL_DIR/arm-miosix-eabi \
	--program-prefix=arm-miosix-eabi- \
	--enable-interwork \
	--enable-multilib \
	--with-float=soft \
	--disable-werror 2>../log/a.txt || quit ":: Error configuring binutils"

make all 2>../log/b.txt					|| quit ":: Error compiling binutils"

$SUDO make install 2>../log/c.txt		|| quit ":: Error installing binutils"

cd ..

#
# Part 4: compile and install gcc-start
#

mkdir objdir
cd objdir
# Note: despite --enable-lto, lto does not yet work. We'll wait for 4.6.x 
$SUDO ../$GCC/configure \
	--target=arm-eabi \
	--prefix=$INSTALL_DIR/arm-miosix-eabi \
	--program-prefix=arm-miosix-eabi- \
	--disable-shared \
	--disable-libmudflap \
	--disable-libssp \
	--disable-nls \
	--disable-libgomp \
	--disable-libstdcxx-pch \
	--with-float=soft \
	--enable-threads=miosix \
	--enable-languages="c,c++" \
	--enable-lto \
	--disable-wchar_t \
	--with-newlib \
	--with-headers=../$NEWLIB/newlib/libc/include \
	2>../log/d.txt || quit ":: Error configuring gcc-start"

$SUDO make all-gcc 2>../log/e.txt			|| quit ":: Error compiling gcc-start"

$SUDO make install-gcc 2>../log/f.txt		|| quit ":: Error installing gcc-start"

# Remove the sys-include directory
# There are two reasons why to remove it: first because it is unnecessary,
# second because it is harmful. Apparently GCC needs the C headers of the target
# to build the compiler itself, therefore when configured --with-newlib and
# --with-headers=[...] it copies those headers in the sys-include folder.
# After gcc is compiled, the installation of newlib places the headers in the
# include dirctory and at that point the sys-include headers aren't necessary anymore
# Now, to see why the're harmful, consider the header newlib.h It is initially
# empty and is filled in by the newlib's ./configure with the appropriate options
# Now, since the configure process happens after, the newlib.h in sys-include
# is the wrong (empty) one, while the one in include is the correct one.
# This causes troubles because newlib.h contains the _WANT_REENT_SMALL used to
# select the appropriate _Reent struct. This error is visible to user code since
# GCC seems to take the wrong newlib.h and user code gets the wrong _Reent struct
$SUDO rm -rf $INSTALL_DIR/arm-miosix-eabi/arm-eabi/sys-include

# Another fix, looks like export PATH isn't enough for newlib, it fails
# running arm-miosix-eabi-runlib when installing
if [ $SUDO ]; then
	$SUDO ln -s $INSTALL_DIR/arm-miosix-eabi/bin/* /usr/bin
fi

cd ..

#
# Part 5: compile and install newlib
#

mkdir newlib-obj
cd newlib-obj

../$NEWLIB/configure \
	--target=arm-eabi \
	--prefix=$INSTALL_DIR/arm-miosix-eabi \
	--enable-interwork \
	--enable-multilib \
	--with-float=soft \
	--enable-newlib-reent-small \
	--enable-newlib-multithread \
	--enable-newlib-io-long-long \
	--disable-newlib-io-c99-formats \
	--disable-newlib-io-long-double \
	--disable-newlib-io-pos-args \
	--disable-newlib-mb \
	--disable-newlib-iconv \
	CC_FOR_TARGET=arm-miosix-eabi-gcc \
	CXX_FOR_TARGET=arm-miosix-eabi-g++ \
	GCC_FOR_TARGET=arm-miosix-eabi-gcc \
	AR_FOR_TARGET=arm-miosix-eabi-ar \
	AS_FOR_TARGET=arm-miosix-eabi-as \
	LD_FOR_TARGET=arm-miosix-eabi-ld \
	NM_FOR_TARGET=arm-miosix-eabi-nm \
	RANLIB_FOR_TARGET=arm-miosix-eabi-ranlib \
	2>../log/g.txt || quit ":: Error configuring newlib"

make 2>../log/h.txt							|| quit ":: Error compiling newlib"

$SUDO make install 2>../log/i.txt			|| quit ":: Error installing newlib"

cd ..

#
# Part 6: compile and install gcc-end
#

cd objdir

$SUDO make all 2>../log/j.txt				|| quit ":: Error compiling gcc-end"

$SUDO make install 2>../log/k.txt			|| quit ":: Error installing gcc-end"

cd ..

#
# Part 7: check that all multilibs have been built.
# This check has been added after an attempt to build arm-miosix-eabi-gcc on Fedora
# where newlib's multilibs were not built. Gcc produced binaries that failed on
# Cortex M3 because the first call to a libc function was a blx into ARM instruction
# set, but since Cortex M3 only has the thumb2 instruction set, the CPU locked.
# By checking that all multilibs are correctly built, this error can be spotted
# immediately instead of leaving a gcc that produces wrong code in the wild. 
#

function check_multilibs {
	if [ ! -f $1/libc.a ]; then
		quit "::Error, $1/libc.a not installed"
	fi
	if [ ! -f $1/libm.a ]; then
		quit "::Error, $1/libm.a not installed"
	fi
	if [ ! -f $1/libg.a ]; then
		quit "::Error, $1/libg.a not installed"
	fi
	if [ ! -f $1/libstdc++.a ]; then
		quit "::Error, $1/libstdc++.a not installed"
	fi
	if [ ! -f $1/libsupc++.a ]; then
		quit "::Error, $1/libsupc++.a not installed"
	fi 
}

check_multilibs $INSTALL_DIR/arm-miosix-eabi/arm-eabi/lib
check_multilibs $INSTALL_DIR/arm-miosix-eabi/arm-eabi/lib/thumb
check_multilibs $INSTALL_DIR/arm-miosix-eabi/arm-eabi/lib/thumb/thumb2
echo "::All multilibs have been built. OK"

#
# Part 8: compile and install lpc21isp.c
#

gcc -o lpc21isp lpc21isp.c							|| quit ":: Error compiling lpc21isp"

$SUDO mv lpc21isp $INSTALL_DIR/arm-miosix-eabi/bin	|| quit ":: Error installing lpc21isp"

#
# Part 9: compile and install gdb
#

cd $GDB

./configure \
	--target=arm-eabi \
	--prefix=$INSTALL_DIR/arm-miosix-eabi \
	--program-prefix=arm-miosix-eabi- \
	--enable-interwork \
	--enable-multilib \
	--disable-werror 2>../log/l.txt || quit ":: Error configuring gdb"

make all 2>../log/m.txt						|| quit ":: Error compiling gdb"

$SUDO make install 2>../log/n.txt			|| quit ":: Error installing gdb"

cd ..

# Last thing, remove this since its name is not arm-miosix-eabi-
$SUDO rm $INSTALL_DIR/arm-miosix-eabi/bin/arm-eabi-$GCC

# If sudo not an empty variable, make symlinks to /usr/bin
# else make a script to override PATH
if [ $SUDO ]; then
	$SUDO ln -s $INSTALL_DIR/arm-miosix-eabi/bin/* /usr/bin
else
	echo '# Used when installing the compiler locally to test it' > env.sh
	echo '# usage: $ . ./env.sh' >> env.sh
	echo '# or     $ source ./env.sh' >> env.sh
	echo "export PATH=`pwd`/gcc/arm-miosix-eabi/bin:"'$PATH' >> env.sh
	chmod +x env.sh
fi

#
# The end.
#

echo ":: Successfully installed"
