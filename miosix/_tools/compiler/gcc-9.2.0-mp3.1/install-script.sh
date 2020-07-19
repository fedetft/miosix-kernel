#!/bin/bash

# Script to build the gcc compiler required for Miosix.
# Usage: ./install-script -j`nproc`
# The -j parameter is passed to make for parallel compilation
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
INSTALL_DIR=/opt/arm-miosix-eabi
SUDO=sudo
# Uncomment if installing locally, sudo isn't necessary
#INSTALL_DIR=`pwd`/gcc/arm-miosix-eabi
#SUDO=

# Uncomment if targeting a local install (linux only). This will use
# -march= -mtune= flags to optimize for your processor, but the code
# won't be portable to other architectures, so don't distribute it
HOST=
# Uncomment if targeting linux 64 bit (distributable)
#HOST=x86_64-linux-gnu
# Uncomment if targeting windows 64 bit (distributable)
# you have to run this script from Linux anyway (see canadian cross compiling)
#HOST=x86_64-w64-mingw32

#### Configuration tunables -- end ####

# Libraries are compiled statically, so they are never installed in the system
LIB_DIR=`pwd`/lib

# Program versions
BINUTILS=binutils-2.32
GCC=gcc-9.2.0
NEWLIB=newlib-3.1.0
GDB=gdb-9.1
GMP=gmp-6.1.2
MPFR=mpfr-4.0.2
MPC=mpc-1.1.0
NCURSES=ncurses-6.1
MAKE=make-4.2.1
MAKESELF=makeself-2.4.0
EXPAT=expat-2.2.9

quit() {
	echo $1
	exit 1
}

if [[ $SUDO ]]; then
	if [[ $HOST ]]; then
		quit ":: Error global install distributable compiling are mutually exclusive"
	fi
fi

if [[ $HOST ]]; then
	# Canadian cross compiling requires to have the same version of the
	# arm-miosix-eabi-gcc that we are going to compile installed locally
	# in the Linux system from which we are compiling, so as to build
	# libc, libstdc++, ...
	# Please note that since the already installed version of
	# arm-miosix-eabi-gcc is used to build the standard libraries, it
	# MUST BE THE EXACT SAME VERSION, including miosix-specific patches
	__GCCVER=`arm-miosix-eabi-gcc --version | perl -ne \
		'next unless(/(\d+\.\d+.\d+)/); print "gcc-$1\n";'`;
	__GCCPAT=`arm-miosix-eabi-gcc -dM -E - < /dev/null | perl -e \
		'my $M, my $m;
		 while(<>) {
		 	$M=$1 if(/_MIOSIX_GCC_PATCH_MAJOR (\d+)/);
		 	$m=$1 if(/_MIOSIX_GCC_PATCH_MINOR (\d+)/);
		 }
		 print "mp$M.$m";'`;
    __GCCPATCUR='mp3.1'; # Can't autodetect this one easily from gcc.patch
	if [[ ($__GCCVER != $GCC) || ($__GCCPAT != $__GCCPATCUR) ]]; then
		quit ":: Error must first install $GCC$__GCCPATCUR system-wide (or . ./env.sh it)"
	fi

	# Canadian cross compiling requires the windows compiler to build
	# arm-miosix-eabi-gcc.exe, ...
	which "$HOST-gcc" > /dev/null || quit ":: Error must have host cross compiler"

	HOSTCC="$HOST-gcc"
	HOSTSTRIP="$HOST-strip"
	if [[ $HOST == *mingw* ]]; then
		HOSTCXX="$HOST-g++ -static -s" # For windows not to depend on libstdc++.dll
		EXT=".exe"
	else
		HOSTCXX="$HOST-g++"
		EXT=
	fi
else
	export PATH=$INSTALL_DIR/bin:$PATH

	HOSTCC=gcc
	HOSTCXX=g++
	HOSTSTRIP=strip
	EXT=
fi

if [[ $1 == '' ]]; then
	PARALLEL="-j1"
else
	PARALLEL=$1;
fi

#
# Part 1: extract data
#

echo "Extracting files, please wait..."
tar -xf downloaded/$BINUTILS.tar.xz			|| quit ":: Error extracting binutils"
tar -xf downloaded/$GCC.tar.xz				|| quit ":: Error extracting gcc"
tar -xf downloaded/$NEWLIB.tar.gz			|| quit ":: Error extracting newlib"
tar -xf downloaded/$GDB.tar.xz				|| quit ":: Error extracing gdb"
tar -xf downloaded/$GMP.tar.xz				|| quit ":: Error extracting gmp"
tar -xf downloaded/$MPFR.tar.xz				|| quit ":: Error extracting mpfr"
tar -xf downloaded/$MPC.tar.gz				|| quit ":: Error extracting mpc"

if [[ $HOST == *mingw* ]]; then
	tar -xf downloaded/$MAKE.tar.gz			|| quit ":: Error extracting make"
fi
if [[ $HOST == *linux* ]]; then
	tar -xf downloaded/$NCURSES.tar.gz		|| quit ":: Error extracting ncurses"
fi
if [[ $HOST ]]; then
	tar -xf downloaded/$EXPAT.tar.xz		|| quit ":: Error extracting expat"
fi

unzip lpc21isp_148_src.zip					|| quit ":: Error extracting lpc21isp"
mkdir log

#
# Part 2: applying patches
#

patch -p0 < patches/binutils.patch	|| quit ":: Failed patching binutils"
patch -p0 < patches/gcc.patch		|| quit ":: Failed patching gcc"
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
	# Don't check if cross-compiling for windows
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
	# Don't check if cross-compiling for windows
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

if [[ $HOST != *mingw* ]]; then
	# Don't check if cross-compiling for windows
	make check $PARALLEL 2> ../log/z.mpc.c.txt	|| quit ":: Error testing mpc"
fi

make install 2>../log/z.mpc.d.txt			|| quit ":: Error installing mpc"

cd ..

#
# Part 4: compile and install binutils
#

cd $BINUTILS

./configure \
	--build=`./config.guess` \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$INSTALL_DIR \
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
	--prefix=$INSTALL_DIR \
	--disable-shared \
	--disable-libssp \
	--disable-nls \
	--disable-libgomp \
	--disable-libstdcxx-pch \
	--disable-libstdcxx-dual-abi \
	--disable-libstdcxx-filesystem-ts \
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
$SUDO rm -rf $INSTALL_DIR/arm-miosix-eabi/sys-include

# Another fix, looks like export PATH isn't enough for newlib, it fails
# running arm-miosix-eabi-ranlib when installing
if [[ $SUDO ]]; then
	# This is actually done also later, but we don't want to add a symlink too
	$SUDO rm $INSTALL_DIR/bin/arm-miosix-eabi-$GCC$EXT

	$SUDO ln -s $INSTALL_DIR/bin/* /usr/bin
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
	--prefix=$INSTALL_DIR \
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
	if [[ ! -f $1/libc.a ]]; then
		quit "::Error, $1/libc.a not installed"
	fi
	if [[ ! -f $1/libm.a ]]; then
		quit "::Error, $1/libm.a not installed"
	fi
	if [[ ! -f $1/libg.a ]]; then
		quit "::Error, $1/libg.a not installed"
	fi
	if [[ ! -f $1/libatomic.a ]]; then
		quit "::Error, $1/libatomic.a not installed"
	fi
	if [[ ! -f $1/libstdc++.a ]]; then
		quit "::Error, $1/libstdc++.a not installed"
	fi
	if [[ ! -f $1/libsupc++.a ]]; then
		quit "::Error, $1/libsupc++.a not installed"
	fi 
}

check_multilibs $INSTALL_DIR/arm-miosix-eabi/lib
check_multilibs $INSTALL_DIR/arm-miosix-eabi/lib/thumb/cm0
check_multilibs $INSTALL_DIR/arm-miosix-eabi/lib/thumb/cm3
check_multilibs $INSTALL_DIR/arm-miosix-eabi/lib/thumb/cm4/hardfp/fpv4sp
check_multilibs $INSTALL_DIR/arm-miosix-eabi/lib/thumb/cm7/hardfp/fpv5
check_multilibs $INSTALL_DIR/arm-miosix-eabi/lib/thumb/cm3/pie/single-pic-base
check_multilibs $INSTALL_DIR/arm-miosix-eabi/lib/thumb/cm4/hardfp/fpv4sp/pie/single-pic-base
check_multilibs $INSTALL_DIR/arm-miosix-eabi/lib/thumb/cm7/hardfp/fpv5/pie/single-pic-base
echo "::All multilibs have been built. OK"

#
# Part 9: compile and install gdb
#

# GDB on linux/windows needs expat
if [[ $HOST ]]; then
	cd $EXPAT

	./configure \
		--host=$HOST \
		--prefix=$LIB_DIR \
		--enable-static=yes \
		--enable-shared=no \
		2> ../log/z.expat.a.txt					|| quit ":: Error configuring expat"

	make all $PARALLEL 2>../log/z.expat.b.txt	|| quit ":: Error compiling expat"

	make install 2>../log/z.expat.d.txt			|| quit ":: Error installing expat"

	cd ..
fi

# GDB on linux requires ncurses, and not to depend on them when doing a
# redistributable linux build we build a static version
# Based on previous gdb that when run with --tui reported as error
# "Error opening terminal: xterm-256color" we now build this terminal as
# fallback within ncurses itself.
if [[ $HOST == *linux* ]]; then
	cd $NCURSES

	./configure \
		--build=`./config.guess` \
		--host=$HOST \
		--prefix=$LIB_DIR \
		--with-normal --without-shared \
		--without-ada --without-cxx-binding --without-debug \
		--with-fallbacks='xterm-256color' \
		--without-manpages --without-progs --without-tests \
		2> ../log/z.ncurses.a.txt				|| quit ":: Error configuring ncurses"

	make all $PARALLEL 2>../log/z.ncurses.b.txt	|| quit ":: Error compiling ncurses"

	make install 2>../log/z.ncurses.d.txt		|| quit ":: Error installing ncurses"

	cd ..
fi

mkdir gdb-obj
cd gdb-obj

# CXX=$HOSTCXX to avoid having to distribute libstdc++.dll on windows
CXX=$HOSTCXX ../$GDB/configure \
	--build=`../$GDB/config.guess` \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$INSTALL_DIR \
	--with-libmpfr-prefix=$LIB_DIR \
	--with-expat-prefix=$LIB_DIR \
	--with-system-zlib=no \
	--with-lzma=no \
	--with-python=no \
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
	INSTALL_DIR=$INSTALL_DIR/bin \
											|| quit ":: Error installing mx-postlinker"
make CXX="$HOSTCXX" SUFFIX=$EXT clean
cd ..

#
# Part 11: compile and install lpc21isp.c
#

$HOSTCC -o lpc21isp$EXT lpc21isp.c						|| quit ":: Error compiling lpc21isp"

$SUDO mv lpc21isp$EXT $INSTALL_DIR/bin					|| quit ":: Error installing lpc21isp"

#
# Part 12: install GNU make and rm (windows release only)
#

if [[ $HOST == *mingw* ]]; then

	cd $MAKE

	./configure \
	--host=$HOST \
	--prefix=$INSTALL_DIR 2> z.make.a.txt					|| quit ":: Error configuring make"

	make all $PARALLEL 2>../log/z.make.b.txt				|| quit ":: Error compiling make"

	make install 2>../log/z.make.c.txt						|| quit ":: Error installing make"

	cd ..

	# FIXME get a better rm to distribute for windows
	$HOSTCC -o rm$EXT -O2 installers/windows/rm.c			|| quit ":: Error compiling rm"

	mv rm$EXT $INSTALL_DIR/bin								|| quit ":: Error installing rm"

	cp downloaded/qstlink2.exe $INSTALL_DIR/bin				|| quit ":: Error installing qstlink2"
fi

#
# Part 13: Final fixups
#

# Remove this since its name is not arm-miosix-eabi-
$SUDO rm $INSTALL_DIR/bin/arm-miosix-eabi-$GCC$EXT

# Strip stuff that is very large when having debug symbols to save disk space
# This simple thing can easily save 500+MB
find $INSTALL_DIR -name cc1$EXT     | $SUDO xargs $HOSTSTRIP
find $INSTALL_DIR -name cc1plus$EXT | $SUDO xargs $HOSTSTRIP
find $INSTALL_DIR -name lto1$EXT    | $SUDO xargs $HOSTSTRIP
$SUDO strip $INSTALL_DIR/bin/*



# Installers, env variables and other stuff
if [[ $HOST ]]; then
	if [[ $HOST == *mingw* ]]; then
		cd installers/windows
		wine "C:\Program Files (x86)\Inno Setup 6\Compil32.exe" /cc MiosixInstaller.iss
		cd ../..
	else
		chmod +x installers/linux/installer.sh uninstall.sh
		# Distribute the installer and uninstaller too
		cp installers/linux/installer.sh uninstall.sh $INSTALL_DIR
		sh downloaded/$MAKESELF.run
		# NOTE: --keep-umask otherwise the installer extracts files setting to 0
		# permissions to group and other, resulting in an unusable installation
		./$MAKESELF/makeself.sh --xz --keep-umask          \
			$INSTALL_DIR                                   \
			MiosixToolchainInstaller9.2.0mp3.1.run         \
			"Miosix toolchain for Linux (GCC 9.2.0-mp3.1)" \
			"./installer.sh"
	fi
else
	# Install the uninstaller too
	chmod +x uninstall.sh
	$SUDO cp uninstall.sh $INSTALL_DIR
	# If sudo not an empty variable, make symlinks to /usr/bin
	# else make a script to override PATH
	if [[ $SUDO ]]; then
		$SUDO ln -s $INSTALL_DIR/bin/* /usr/bin
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
