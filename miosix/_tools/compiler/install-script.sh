#!/bin/bash

# Script to build the gcc compiler required for Miosix.
# Usage: ./install-script -j2
# The first parameter is used to speed up compiling for multicore processors,
# use -j2 for a dual core, -j4 for a quad core, ...
#
# Building Miosix is officially supported only through the gcc compiler built
# with this script. This is because this script patches the compiler.
# Starting from Miosix 1.58 the use of the arm-miosix-eabi-gcc compiler built
# by this script has become mandatory due to patches related to posix threads
# in newlib. The kernel *won't* compile unless the correct compiler is used.
#
# Starting from 04/2014 this script is also used to build binary releases
# of the Miosix compiler for both linux and windows. Most users will want to
# download the binary relase from http://miosix.org instead of compiling GCC
# using this script.
#
# This script will install arm-miosix-eabi-gcc in /opt, creating links to
# binaries in /usr/bin.
# It should be run without root privileges, but it will ask for the root
# password when installing files to /opt and /usr/bin

#### Configuration tunables -- begin ####

# Uncomment if installing globally on the system
INSTALL_DIR=/opt
SUDO=sudo
# Uncomment if installing locally, sudo isn't necessary
#INSTALL_DIR=`pwd`/gcc
#SUDO=

# Uncomment if targeting a local install (linux only). This will use
# -march= -mtune= flags to optimize for your processor, but the code
# won't be portable to other architectures, so don't distribute it
HOST=
# Uncomment if targeting linux 32 bit (distributable)
#HOST=i686-linux-gnu
# Uncomment if targeting windows (distributable)
# you have to run this script from Linux anyway (see canadian cross compiling)
#HOST=i686-w64-mingw32

#### Configuration tunables -- end ####

# Libraries are compiled statically, so they are never installed in the system
LIB_DIR=`pwd`/lib

# Program versions
BINUTILS=binutils-2.23.1
GCC=gcc-4.7.3
NEWLIB=newlib-2.0.0
GDB=gdb-7.5
GMP=gmp-6.0.0
MPFR=mpfr-3.1.2
MPC=mpc-1.0.2
NCURSES=ncurses-5.9
MAKE=make-4.0
MAKESELF=makeself-2.1.5

quit() {
	echo $1
	exit 1
}

if [ $SUDO ]; then
	if [ $HOST ]; then
		quit ":: Error global install distributable compiling are mutually exclusive"
	fi
fi

if [ $HOST ]; then
	# Canadian cross compiling requires to have the same version of the
	# arm-miosix-eabi-gcc that we are going to compile installed locally
	# in the Linux system from which we are compiling, so as to build
	# libc, libstdc++, ...
	# Please note that since the already installed version of
	# arm-miosix-eabi-gcc is used to build the standard libraries, it
	# NUST BE THE EXACT SAME VERSION, including miosix-specific patches
	which arm-miosix-eabi-gcc > /dev/null || quit ":: Error must first install arm-miosix-eabi-gcc"

	HOSTCC="$HOST-gcc"
	HOSTSTRIP="$HOST-strip"
	if [[ $HOST == *mingw* ]]; then
		# Canadian cross compiling requires the windows compiler to build
		# arm-miosix-eabi-gcc.exe, ...
		which "$HOST-gcc" > /dev/null || quit ":: Error must have host cross compiler"

		HOSTCXX="$HOST-g++ -static -s" # For windows not to depend on libstdc++.dll
		EXT=".exe"
	else
		# Crosscompiling linux 2 linux to achieve a redistributable build is
		# really a pain. Even worse than doing a redistributable windows
		# version. The reason is that we need to set --host=i686-linux-gnu
		# but there isn't actually a compiler named i686-linux-gnu-gcc.
		# You might be thinking that autotools are smart enough to call
		# the regular gcc with '-m32 -march=i686', but they aren't. I've
		# tried messing with CFLAGS but gone nowhere, since some scripts
		# insist for calling tools such as i686-linux-gnu-ar without
		# first checking for their existence, so here's my "quick fix":
		# make a directory with shell scripts named i686-linux-gnu-gcc, ...
		# that just contain 'gcc -m32 -march=i686 $@' and add it to PATH.
		# It isn't pretty, but it works.
		# Also, these scripts work around gdb needing libncurses and
		# totally ignoring the --with-sysroot flag by adding `pwd`/lib
		# to the compiler search path.
		mkdir quickfix
		cd quickfix
		echo "gcc -m32 -march=i686 -I$LIB_DIR/include -L$LIB_DIR/lib "'"$@"' > "$HOST-gcc"
		echo "g++ -m32 -march=i686 -I$LIB_DIR/include -L$LIB_DIR/lib "'"$@"' > "$HOST-g++"
		echo "ld -m32 -march=i686 -L$LIB_DIR/lib "'"$@"' > "$HOST-ld"
		echo "ar "'"$@"' > "$HOST-ar"
		echo "ranlib "'"$@"' > "$HOST-ranlib"
		echo "strip "'"$@"' > "$HOST-strip"
		chmod +x *
		export PATH=$PATH:`pwd`
		cd ..

		HOSTCXX="$HOST-g++"
		EXT=
	fi
else
	export PATH=$INSTALL_DIR/arm-miosix-eabi/bin:$PATH

	HOSTCC=gcc
	HOSTCXX=g++
	HOSTSTRIP=strip
	EXT=
fi

if [ "x$1" = "x" ]; then
	PARALLEL="-j1"
else
	PARALLEL=$1;
fi

#
# Part 1: extract data
#

echo "Extracting files, please wait..."
tar -xjf $BINUTILS.tar.bz2			|| quit ":: Error extracting binutils"
tar -xjf $GCC.tar.bz2				|| quit ":: Error extracting gcc"
tar -xf  $NEWLIB.tar.gz				|| quit ":: Error extracting newlib"
tar -xjf $GDB.tar.bz2				|| quit ":: Error extracing gdb"
tar -xf  $GMP.tar.lz				|| quit ":: Error extracting gmp"
tar -xf  $MPFR.tar.xz				|| quit ":: Error extracting mpfr"
tar -xf  $MPC.tar.gz				|| quit ":: Error extracting mpc"

if [[ $HOST == *mingw* ]]; then
	tar -xf  $MAKE.tar.bz2			|| quit ":: Error extracting make"
fi
if [[ $HOST == *linux* ]]; then
	tar -xf  $NCURSES.tar.gz		|| quit ":: Error extracting ncurses"
fi

unzip lpc21isp_148_src.zip			|| quit ":: Error extracting lpc21isp"
mkdir log

#
# Part 2: applying patches
#

patch -p0 < patches/gcc.patch		|| quit ":: Failed patching gcc 1"
patch -p0 < patches/force-got.patch	|| quit ":: Failed patching gcc 2"
patch -p0 < patches/newlib.patch	|| quit ":: Failed patching newlib"

#
# Part 3: compile libraries
#

cd $GMP

./configure \
	--build=`./config.guess` \
	--host=$HOST \
	--prefix=$LIB_DIR \
	--enable-static --disable-shared \
	2> ../log/z.gmp.a.txt					|| quit ":: Error configuring gmp"

make all $PARALLEL 2>../log/z.gmp.b.txt		|| quit ":: Error compiling gmp"

if [[ $HOST != *mingw* ]]; then
	# FIXME: test fails (t-scanf.exe)
	make check $PARALLEL 2> ../log/z.gmp.c.txt	|| quit ":: Error testing gmp"
fi

make install 2>../log/z.gmp.d.txt			|| quit ":: Error installing gmp"

cd ..

cd $MPFR

./configure \
	--build=`./config.guess` \
	--host=$HOST \
	--prefix=$LIB_DIR \
	--enable-static --disable-shared \
	--with-gmp=$LIB_DIR \
	2> ../log/z.mpfr.a.txt					|| quit ":: Error configuring mpfr"

make all $PARALLEL 2>../log/z.mpfr.b.txt	|| quit ":: Error compiling mpfr"

if [[ $HOST != *mingw* ]]; then
	# printf/scanf tests fail due to wine unimplemented features, this is known
	# http://readlist.com/lists/gcc.gnu.org/gcc/10/53348.html
	make check $PARALLEL 2> ../log/z.mpfr.c.txt	|| quit ":: Error testing mpfr"
fi

make install 2>../log/z.mpfr.d.txt			|| quit ":: Error installing mpfr"

cd ..

cd $MPC

./configure \
	--build=`./config.guess` \
	--host=$HOST \
	--prefix=$LIB_DIR \
	--enable-static --disable-shared \
	--with-gmp=$LIB_DIR \
	--with-mpfr=$LIB_DIR \
	2> ../log/z.mpc.a.txt					|| quit ":: Error configuring mpc"

make all $PARALLEL 2>../log/z.mpc.b.txt		|| quit ":: Error compiling mpc"

make check $PARALLEL 2> ../log/z.mpc.c.txt	|| quit ":: Error testing mpc"

make install 2>../log/z.mpc.d.txt			|| quit ":: Error installing mpc"

cd ..

#
# Part 4: compile and install binutils
#

#To enable gold (currently does not work) instead of ld, add
#	--enable-gold=yes \
#	--enable-ld=no \
cd $BINUTILS

./configure \
	--build=`./config.guess` \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$INSTALL_DIR/arm-miosix-eabi \
	--enable-interwork \
	--enable-multilib \
	--enable-lto \
	--disable-werror 2>../log/a.txt			|| quit ":: Error configuring binutils"

make all $PARALLEL 2>../log/b.txt			|| quit ":: Error compiling binutils"

$SUDO make install 2>../log/c.txt			|| quit ":: Error installing binutils"

cd ..

#
# Part 5: compile and install gcc-start
#

mkdir objdir
cd objdir

$SUDO ../$GCC/configure \
	--build=`../$GCC/config.guess` \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--with-gmp=$LIB_DIR \
	--with-mpfr=$LIB_DIR \
	--with-mpc=$LIB_DIR \
	MAKEINFO=missing \
	--prefix=$INSTALL_DIR/arm-miosix-eabi \
	--disable-shared \
	--disable-libmudflap \
	--disable-libssp \
	--disable-nls \
	--disable-libgomp \
	--disable-libstdcxx-pch \
	--enable-threads=miosix \
	--enable-languages="c,c++" \
	--enable-lto \
	--disable-wchar_t \
	--with-newlib \
	--with-headers=../$NEWLIB/newlib/libc/include \
	2>../log/d.txt							|| quit ":: Error configuring gcc-start"

$SUDO make all-gcc $PARALLEL 2>../log/e.txt	|| quit ":: Error compiling gcc-start"

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
$SUDO rm -rf $INSTALL_DIR/arm-miosix-eabi/arm-miosix-eabi/sys-include

# Another fix, looks like export PATH isn't enough for newlib, it fails
# running arm-miosix-eabi-ranlib when installing
if [ $SUDO ]; then
	$SUDO ln -s $INSTALL_DIR/arm-miosix-eabi/bin/* /usr/bin
fi

cd ..

#
# Part 6: compile and install newlib
#

mkdir newlib-obj
cd newlib-obj

../$NEWLIB/configure \
	--build=`../$GCC/config.guess` \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$INSTALL_DIR/arm-miosix-eabi \
	--enable-multilib \
	--enable-newlib-reent-small \
	--enable-newlib-multithread \
	--enable-newlib-io-long-long \
	--disable-newlib-io-c99-formats \
	--disable-newlib-io-long-double \
	--disable-newlib-io-pos-args \
	--disable-newlib-mb \
	--disable-newlib-supplied-syscalls \
	2>../log/g.txt							|| quit ":: Error configuring newlib"

make $PARALLEL 2>../log/h.txt				|| quit ":: Error compiling newlib"

$SUDO make install 2>../log/i.txt			|| quit ":: Error installing newlib"

cd ..

#
# Part 7: compile and install gcc-end
#

cd objdir

# This fails with windows crosscompiling because GCC seems to have a bug when
# compiling crtstuff.c, the following error is found:
# sys/types.h:130:16: error: expected identifier or '(' before 'char'
# the line in question is 'typedef	char *	caddr_t;', but a look at the
# compiler output after the preprocessor (-E option) it becomes
# 'typedef	char *	char *;' so someone is doing a '#define caddr_t char *'
# A look at objdir/gcc/config.log shows
# configure:9173: checking for caddr_t
# configure:9173: i686-w64-mingw32-gcc -c -g -D__USE_MINGW_ACCESS  conftest.c >&5
# conftest.c: In function 'main':
# conftest.c:108:13: error: 'caddr_t' undeclared (first use in this function)
# so there's likely a bug in GCC configure scripts that define caddr_t for the
# TARGET compiler if the HOST compiler doesn't have it. This seems to explain
# why while building for linux this bug was never found.
# The #define seems to be in 'objdir/gcc/auto-host.h', and sure enough having a
# look at line 53 of crtstuff.c we have this comment:
# /* FIXME: Including auto-host is incorrect, but until we have
#   identified the set of defines that need to go into auto-target.h,
#   this will have to do.  */
# To fix this we add an '#undef caddr_t' to crtstuff.c
sed -i 's/#include "auto-host.h"/#include "auto-host.h"\n#undef caddr_t/' ../$GCC/libgcc/crtstuff.c

$SUDO make all $PARALLEL 2>../log/j.txt		|| quit ":: Error compiling gcc-end"

$SUDO make install 2>../log/k.txt			|| quit ":: Error installing gcc-end"

cd ..

#
# Part 8: check that all multilibs have been built.
# This check has been added after an attempt to build arm-miosix-eabi-gcc on Fedora
# where newlib's multilibs were not built. Gcc produced binaries that failed on
# Cortex M3 because the first call to a libc function was a blx into ARM instruction
# set, but since Cortex M3 only has the thumb2 instruction set, the CPU locked.
# By checking that all multilibs are correctly built, this error can be spotted
# immediately instead of leaving a gcc that produces wrong code in the wild. 

check_multilibs() {
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

check_multilibs $INSTALL_DIR/arm-miosix-eabi/arm-miosix-eabi/lib
check_multilibs $INSTALL_DIR/arm-miosix-eabi/arm-miosix-eabi/lib/thumb/cm3
check_multilibs $INSTALL_DIR/arm-miosix-eabi/arm-miosix-eabi/lib/thumb/cm4/hardfp/fpv4
check_multilibs $INSTALL_DIR/arm-miosix-eabi/arm-miosix-eabi/lib/pie/single-pic-base
check_multilibs $INSTALL_DIR/arm-miosix-eabi/arm-miosix-eabi/lib/thumb/cm3/pie/single-pic-base
check_multilibs $INSTALL_DIR/arm-miosix-eabi/arm-miosix-eabi/lib/thumb/cm4/hardfp/fpv4/pie/single-pic-base
echo "::All multilibs have been built. OK"

#
# Part 9: compile and install gdb
#

# GDB on linux requires ncurses, and not to depend on them when doing a
# redistributable linux build we build a static version
if [[ $HOST == *linux* ]]; then
	cd $NCURSES

	./configure \
		--build=`./config.guess` \
		--host=$HOST \
		--prefix=$LIB_DIR \
		--enable-static --disable-shared \
		--without-cxx-binding --without-ada \
		--without-progs --without-tests \
		2> ../log/z.ncurses.a.txt				|| quit ":: Error configuring ncurses"

	make all $PARALLEL 2>../log/z.ncurses.b.txt	|| quit ":: Error compiling ncurses"

	make install 2>../log/z.ncursesp.d.txt		|| quit ":: Error installing ncurses"

	cd ..
fi

cd $GDB

./configure \
	--build=`../$GCC/config.guess` \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$INSTALL_DIR/arm-miosix-eabi \
	--enable-interwork \
	--enable-multilib \
	--disable-werror 2>../log/l.txt			|| quit ":: Error configuring gdb"

make all $PARALLEL 2>../log/m.txt			|| quit ":: Error compiling gdb"

$SUDO make install 2>../log/n.txt			|| quit ":: Error installing gdb"

cd ..

#
# Part 10: install the postlinker
#
cd mx-postlinker
make CXX="$HOSTCXX" SUFFIX=$EXT				|| quit ":: Error compiling mx-postlinker"
$SUDO make install CXX="$HOSTCXX" SUFFIX=$EXT \
	INSTALL_DIR=$INSTALL_DIR/arm-miosix-eabi/bin \
											|| quit ":: Error installing mx-postlinker"
make CXX="$HOSTCXX" SUFFIX=$EXT clean
cd ..

#
# Part 11: compile and install lpc21isp.c
#

$HOSTCC -o lpc21isp$EXT lpc21isp.c						|| quit ":: Error compiling lpc21isp"

$SUDO mv lpc21isp$EXT $INSTALL_DIR/arm-miosix-eabi/bin	|| quit ":: Error installing lpc21isp"

#
# Part 12: install GNU make and rm (windows release only)
#

if [[ $HOST == *mingw* ]]; then

	cd $MAKE

	./configure \
	--host=$HOST \
	--prefix=$INSTALL_DIR/arm-miosix-eabi 2> z.make.a.txt	|| quit ":: Error configuring make"

	make all $PARALLEL 2>../log/z.make.b.txt				|| quit ":: Error compiling make"

	make install 2>../log/z.make.c.txt						|| quit ":: Error installing make"

	cd ..

	# FIXME get a better rm to distribute for windows
	$HOSTCC -o rm$EXT -O2 windows-installer/rm.c			|| quit ":: Error compiling rm"

	mv rm$EXT $INSTALL_DIR/arm-miosix-eabi/bin				|| quit ":: Error installing rm"

	cp qstlink2.exe $INSTALL_DIR/arm-miosix-eabi/bin		|| quit ":: Error installing qstlink2"
fi

#
# Part 13: Final fixups
#

# Remove this since its name is not arm-miosix-eabi-
$SUDO rm $INSTALL_DIR/arm-miosix-eabi/bin/arm-miosix-eabi-$GCC$EXT

# Installers, env variables and other stuff
if [ $HOST ]; then
	# Strip stuff that is very large when having debug symbols to save disk space
	# This simple thing can easily save 100+MB
	find $INSTALL_DIR -name cc1$EXT | xargs $HOSTSTRIP
	find $INSTALL_DIR -name cc1plus$EXT | xargs $HOSTSTRIP
	find $INSTALL_DIR -name lto1$EXT | xargs $HOSTSTRIP

	if [[ $HOST == *mingw* ]]; then
		cd windows-installer
		wine "C:\Program Files\Inno Setup 5\Compil32.exe" /cc MiosixInstaller.iss
		cd ..
	else
		chmod +x linux-installer/installer.sh uninstall.sh
		# Distribute the installer and uninstaller too
		cp linux-installer/installer.sh uninstall.sh $INSTALL_DIR/arm-miosix-eabi
		sh $MAKESELF.run
		./$MAKESELF/makeself.sh --bzip2 \
			$INSTALL_DIR/arm-miosix-eabi \
			MiosixToolchainInstaller.run \
			"Miosix toolchain for Linux" \
			"./installer.sh"
	fi
else
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
fi

#
# The end.
#

echo ":: Successfully installed"
