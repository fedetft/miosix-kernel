#!/usr/bin/env bash

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

# Uncomment if installing globally on this system
PREFIX=/opt/arm-miosix-eabi
DESTDIR=
SUDO=sudo
# Uncomment if installing locally on this system, sudo isn't necessary
#PREFIX=`pwd`/gcc/arm-miosix-eabi
#DESTDIR=
#SUDO=
# Uncomment for producing a package for redistribution. The prefix is set to the
# final install directory, but when this script does "make install" files are
# copied with $DESTDIR as prefix. When doing a redistibutable build you also
# have to specify HOST or (on Mac OS), BUILD, see below.
#PREFIX=/opt/arm-miosix-eabi
#DESTDIR=`pwd`/dist
#SUDO=

# Uncomment if targeting a local install. This will use -march= -mtune= flags
# to optimize for your processor, but the code won't be portable to other
# architectures, so don't distribute it
BUILD=
HOST=
# Uncomment if targeting linux 64 bit (distributable)
#BUILD=
#HOST=x86_64-linux-gnu
# Uncomment if targeting windows 64 bit (distributable)
# you have to run this script from Linux anyway (see canadian cross compiling)
#BUILD=
#HOST=x86_64-w64-mingw32
# Uncomment if targeting macOS 64 bit Intel (distributable), compiling on Linux
# Must first install the osxcross toolchain
#BUILD=
#HOST=x86_64-apple-darwin18
# Uncomment if targeting macOS 64 bit Intel (distributable), compiling on macOS
# The script must be run under macOS and without canadian cross compiling
# because it confuses autotools's configuration scripts. Instead we set the
# compiler options for macOS minimum version and architecture in order to be
# able to deploy the binaries on older machines and OS versions. We also must
# force --build and --host to specify a x86_64 cpu to avoid
# architecture-dependent code.
#BUILD=x86_64-apple-darwin17 # No macs exist with a cpu older than a Core 2
#HOST=
#export CFLAGS='-mmacos-version-min=10.13 -O3'
#export CXXFLAGS='-mmacos-version-min=10.13 -O3'

#### Configuration tunables -- end ####

# Libraries are compiled statically, so they are never installed in the system
LIB_DIR=`pwd`/lib

# Program versions
BINUTILS=binutils-2.32
GCC=gcc-9.2.0
NEWLIB=newlib-3.1.0
GDB=gdb-9.1
GMP=gmp-6.2.1
MPFR=mpfr-4.0.2
MPC=mpc-1.1.0
NCURSES=ncurses-6.1
MAKE=make-4.2.1
MAKESELF=makeself-2.4.5
EXPAT=expat-2.2.10

quit() {
	echo $1
	exit 1
}

# Is it a redistributable build?
if [[ $DESTDIR ]]; then
	if [[ $SUDO ]]; then
		quit ":: Error global install distributable compiling are mutually exclusive"
	fi
	if [[ $(uname -s) == 'Darwin' ]]; then
		if [[ -z $BUILD ]]; then
			quit ":: Error distributable compiling but no BUILD specifed"
		fi
	else
		if [[ -z $HOST ]]; then
			quit ":: Error distributable compiling but no HOST specifed"
		fi
	fi
	# Since we do not install the bootstrapped arm-miosix-eabi-gcc during the
	# build process, for making libc, libstdc++, ... we need the same version of
	# arm-miosix-eabi-gcc that we are going to compile already installed locally
	# in the system from which we are compiling.
	#   Please note that since the already installed version of
	# arm-miosix-eabi-gcc is used to build the standard libraries, it
	# MUST BE THE EXACT SAME VERSION, including miosix-specific patches.
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
		quit ":: Error must first install $GCC$__GCCPATCUR system-wide"
	fi
	if [[ ! -e $PREFIX/bin/arm-miosix-eabi-gcc ]]; then
		quit ":: Error To use \$DESTDIR you must first install $GCC$__GCCPATCUR in the same prefix"
	fi
else
	if [[ $HOST || $BUILD ]]; then
		# NOTE: doing a non redistributable build but specifying HOST or BUILD
		# may work, but is untested. Remove this line if you want to try.
		quit ":: Specifying either HOST or BUILD without DESTDIR is not supported"
	fi
	# For local builds assume there's no existing miosix compiler installed,
	# add the install prefix to the path in order to ensure tools are available
	# as soon as we build them.
	export PATH=$PREFIX/bin:$PATH
fi

# Are we canadian cross compiling?
if [[ $HOST ]]; then
	# Canadian cross compiling requires the compiler for the host machine
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
# Part 1/2: extract data, apply patches
#

extract()
{
	label=$1
	filename=$2
	patchfile=$3
	directory=${filename%.tar*}
	
	if [[ -e $directory ]]; then
		echo "Skipping extraction/patching of $label, directory $directory exists"
	else
		echo "Extracting $label..."
		tar -xf downloaded/$2 || quit ":: Error extracting $label"
		if [[ ! -z "$patchfile" ]]; then
			patch -p0 < "$patchfile" || quit ":: Failed patching $label"
		fi
	fi
}

extract 'binutils' $BINUTILS.tar.xz patches/binutils.patch
extract 'gcc' $GCC.tar.xz patches/gcc.patch
extract 'newlib' $NEWLIB.tar.gz patches/newlib.patch
extract 'gdb' $GDB.tar.xz
extract 'gmp' $GMP.tar.xz
extract 'mpfr' $MPFR.tar.xz
extract 'mpc' $MPC.tar.gz

if [[ $HOST == *mingw* ]]; then
	extract 'make' $MAKE.tar.gz
fi
if [[ $HOST == *linux* ]]; then
	extract 'ncurses' $NCURSES.tar.gz
fi
if [[ $DESTDIR ]]; then
	extract 'expat' $EXPAT.tar.xz
fi

unzip -o lpc21isp_148_src.zip || quit ":: Error extracting lpc21isp"
mkdir log

#
# Part 3: compile libraries
#

cd $GMP

./configure \
	--build=$BUILD \
	--host=$HOST \
	--prefix=$LIB_DIR \
	--enable-static --disable-shared \
	2> ../log/z.gmp.a.txt					|| quit ":: Error configuring gmp"

make all $PARALLEL 2>../log/z.gmp.b.txt		|| quit ":: Error compiling gmp"

if [[ ! $HOST ]]; then
	# Don't check if cross-compiling
	make check $PARALLEL 2> ../log/z.gmp.c.txt	|| quit ":: Error testing gmp"
fi

make install 2>../log/z.gmp.d.txt			|| quit ":: Error installing gmp"

cd ..

cd $MPFR

./configure \
	--build=$BUILD \
	--host=$HOST \
	--prefix=$LIB_DIR \
	--enable-static --disable-shared \
	--with-gmp=$LIB_DIR \
	2> ../log/z.mpfr.a.txt					|| quit ":: Error configuring mpfr"

make all $PARALLEL 2>../log/z.mpfr.b.txt	|| quit ":: Error compiling mpfr"

if [[ ! $HOST ]]; then
	# Don't check if cross-compiling
	make check $PARALLEL 2> ../log/z.mpfr.c.txt	|| quit ":: Error testing mpfr"
fi

make install 2>../log/z.mpfr.d.txt			|| quit ":: Error installing mpfr"

cd ..

cd $MPC

./configure \
	--build=$BUILD \
	--host=$HOST \
	--prefix=$LIB_DIR \
	--enable-static --disable-shared \
	--with-gmp=$LIB_DIR \
	--with-mpfr=$LIB_DIR \
	2> ../log/z.mpc.a.txt					|| quit ":: Error configuring mpc"

make all $PARALLEL 2>../log/z.mpc.b.txt		|| quit ":: Error compiling mpc"

if [[ ! $HOST ]]; then
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
	--build=$BUILD \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$PREFIX \
	--enable-interwork \
	--enable-multilib \
	--enable-lto \
	--disable-werror 2>../log/a.txt			|| quit ":: Error configuring binutils"

make all $PARALLEL 2>../log/b.txt			|| quit ":: Error compiling binutils"

$SUDO make install DESTDIR=$DESTDIR 2>../log/c.txt || quit ":: Error installing binutils"

cd ..

#
# Part 5: compile and install gcc-start
#

mkdir objdir
cd objdir

# GCC needs the C headers of the target to configure and build the C++ standard
# library, therefore when configured --with-headers=[...] the configure script
# unconditionally copies those headers in the
# $PREFIX/arm-miosix-eabi/sys-include folder.
#   This is fine for local installs, (up to a certain point, see later
# comments), but for distributable builds we already have the headers in
# $PREFIX/arm-miosix-eabi/include (not in sys-include!) and we don't want to
# touch the existing install.
#   However the GCC makefiles are not clever enough to search in `include`
# rather than `sys-include` when checking if limits.h exists to decide whether
# to "fix" it. Since `sys-include` does not exists, GCC does not find limits.h
# and replaces it with its own, which does not include all definitions made
# by newlib's one. This incorrect file ends up installed and used by the
# built GCC causing build failures usually related to missing defines used
# by dirent.h.
#   Therefore we must differentiate the case in which we are installing
# or building for redistribution. When we build for redistribution we use
# --with-sysroot and --with-native-system-header-dir to instruct the GCC
# configure script to set CROSS_SYSTEM_HEADER_DIR to the place where we already
# have our headers available, without attempting to copy stuff in $PREFIX.
if [[ $DESTDIR ]]; then
	__GCC_CONF_HEADERS_PARAM="--with-sysroot=$PREFIX/arm-miosix-eabi --with-native-system-header-dir=/include"
else
	__GCC_CONF_HEADERS_PARAM=--with-headers=../$NEWLIB/newlib/libc/include
fi

$SUDO ../$GCC/configure \
	--build=$BUILD \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--with-gmp=$LIB_DIR \
	--with-mpfr=$LIB_DIR \
	--with-mpc=$LIB_DIR \
	MAKEINFO=missing \
	--prefix=$PREFIX \
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
	${__GCC_CONF_HEADERS_PARAM} \
	2>../log/d.txt							|| quit ":: Error configuring gcc-start"

$SUDO make all-gcc $PARALLEL 2>../log/e.txt || quit ":: Error compiling gcc-start"

$SUDO make install-gcc DESTDIR=$DESTDIR 2>../log/f.txt || quit ":: Error installing gcc-start"

if [[ -z $DESTDIR ]]; then
	# Remove the sys-include directory if we are installing locally.
	# There are two reasons why to remove it: first because it is unnecessary,
	# second because it is harmful.
	# After gcc is compiled, the installation of newlib places the headers in the
	# include dirctory and at that point the sys-include headers aren't necessary anymore
	# Now, to see why the're harmful, consider the header newlib.h It is initially
	# empty and is filled in by the newlib's ./configure with the appropriate options
	# Now, since the configure process happens after, the newlib.h in sys-include
	# is the wrong (empty) one, while the one in include is the correct one.
	# This causes troubles because newlib.h contains the _WANT_REENT_SMALL used to
	# select the appropriate _Reent struct. This error is visible to user code since
	# GCC seems to take the wrong newlib.h and user code gets the wrong _Reent struct
	$SUDO rm -rf $PREFIX/arm-miosix-eabi/sys-include
	
	# Another fix, looks like export PATH isn't enough for newlib, it fails
	# running arm-miosix-eabi-ranlib when installing
	if [[ $SUDO ]]; then
		# This is actually done also later, but we don't want to add a symlink too
		$SUDO rm $PREFIX/bin/arm-miosix-eabi-$GCC$EXT

		# Linking to /usr/bin does not work on macOS because of SIP, but newlib
		# appears to build just fine with export PATH on macOS...
		if [[ ! ( $(uname -s) == 'Darwin' ) ]]; then
			$SUDO ln -s $DESTDIR$PREFIX/bin/* /usr/bin
		fi
	fi
fi

cd ..

#
# Part 6: compile and install newlib
#

mkdir newlib-obj
cd newlib-obj

../$NEWLIB/configure \
	--build=$BUILD \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$PREFIX \
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

$SUDO make install DESTDIR=$DESTDIR 2>../log/i.txt || quit ":: Error installing newlib"

cd ..

#
# Part 7: compile and install gcc-end
#

cd objdir

$SUDO make all $PARALLEL 2>../log/j.txt		|| quit ":: Error compiling gcc-end"

$SUDO make install DESTDIR=$DESTDIR 2>../log/k.txt || quit ":: Error installing gcc-end"

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

check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/cm0
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/cm3
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/cm4/hardfp/fpv4sp
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/cm7/hardfp/fpv5
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/cm3/pie/single-pic-base
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/cm4/hardfp/fpv4sp/pie/single-pic-base
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/cm7/hardfp/fpv5/pie/single-pic-base
echo "::All multilibs have been built. OK"

#
# Part 9: compile and install gdb
#

# GDB on linux/windows needs expat
if [[ $DESTDIR ]]; then
	cd $EXPAT

	./configure \
		--build=$BUILD \
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
		--build=$BUILD \
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
	--build=$BUILD \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$PREFIX \
	--with-libmpfr-prefix=$LIB_DIR \
	--with-expat-prefix=$LIB_DIR \
	--with-system-zlib=no \
	--with-lzma=no \
	--with-python=no \
	--enable-interwork \
	--enable-multilib \
	--disable-werror 2>../log/l.txt			|| quit ":: Error configuring gdb"

# Specify a dummy MAKEINFO binary to work around an issue in the gdb makefiles
# where compilation fails if MAKEINFO is not installed.
# https://sourceware.org/bugzilla/show_bug.cgi?id=14678
make all MAKEINFO=/usr/bin/true $PARALLEL 2>../log/m.txt || quit ":: Error compiling gdb"

$SUDO make install MAKEINFO=/usr/bin/true DESTDIR=$DESTDIR 2>../log/n.txt || quit ":: Error installing gdb"

cd ..

#
# Part 10: install the postlinker
#
cd mx-postlinker
make CXX="$HOSTCXX" SUFFIX=$EXT				|| quit ":: Error compiling mx-postlinker"
$SUDO make install CXX="$HOSTCXX" SUFFIX=$EXT \
	INSTALL_DIR=$DESTDIR$PREFIX/bin \
											|| quit ":: Error installing mx-postlinker"
make CXX="$HOSTCXX" SUFFIX=$EXT clean
cd ..

#
# Part 11: compile and install lpc21isp.c
#

$HOSTCC -o lpc21isp$EXT lpc21isp.c						|| quit ":: Error compiling lpc21isp"

$SUDO mv lpc21isp$EXT $DESTDIR$PREFIX/bin || quit ":: Error installing lpc21isp"

#
# Part 12: install GNU make and rm (windows release only)
#

if [[ $HOST == *mingw* ]]; then

	cd $MAKE

	./configure \
		--build=$BUILD \
		--host=$HOST \
		--prefix=$PREFIX 2> z.make.a.txt || quit ":: Error configuring make"

	make all $PARALLEL 2>../log/z.make.b.txt || quit ":: Error compiling make"

	make install DESTDIR=$DESTDIR 2>../log/z.make.c.txt || quit ":: Error installing make"

	cd ..

	# FIXME get a better rm to distribute for windows
	$HOSTCC -o rm$EXT -O2 installers/windows/rm.c || quit ":: Error compiling rm"

	mv rm$EXT $DESTDIR$PREFIX/bin || quit ":: Error installing rm"

	cp downloaded/qstlink2.exe $DESTDIR$PREFIX/bin || quit ":: Error installing qstlink2"
fi

#
# Part 13: Final fixups
#

# Remove this since its name is not arm-miosix-eabi-
$SUDO rm $DESTDIR$PREFIX/bin/arm-miosix-eabi-$GCC$EXT

# Strip stuff that is very large when having debug symbols to save disk space
# This simple thing can easily save 500+MB
find $DESTDIR$PREFIX -name cc1$EXT     | $SUDO xargs $HOSTSTRIP
find $DESTDIR$PREFIX -name cc1plus$EXT | $SUDO xargs $HOSTSTRIP
find $DESTDIR$PREFIX -name lto1$EXT    | $SUDO xargs $HOSTSTRIP
$SUDO $HOSTSTRIP $DESTDIR$PREFIX/bin/*



# Installers, env variables and other stuff
if [[ $DESTDIR ]]; then
	if [[ ( $(uname -s) == 'Linux' ) && ( $HOST == *linux* ) ]]; then
		# Build a makeself installer
		# Distribute the installer and uninstaller too
		sed -E "s|/opt/arm-miosix-eabi|$PREFIX|g" installers/linux/installer.sh > $DESTDIR$PREFIX/installer.sh
		sed -E "s|/opt/arm-miosix-eabi|$PREFIX|g" uninstall.sh > $DESTDIR$PREFIX/uninstall.sh
		chmod +x $DESTDIR$PREFIX/installer.sh $DESTDIR$PREFIX/uninstall.sh
		sh downloaded/$MAKESELF.run
		# NOTE: --keep-umask otherwise the installer extracts files setting to 0
		# permissions to group and other, resulting in an unusable installation
		./$MAKESELF/makeself.sh --xz --keep-umask \
			$DESTDIR$PREFIX \
			MiosixToolchainInstaller9.2.0mp3.1.run \
			"Miosix toolchain for Linux (GCC 9.2.0-mp3.1)" \
			"./installer.sh"
	elif [[ ( $(uname -s) == 'Linux' ) && ( $HOST == *mingw* ) ]]; then
		# Build an executable installer for Windows
		cd installers/windows
		wine "C:\Program Files (x86)\Inno Setup 6\Compil32.exe" /cc MiosixInstaller.iss
		cd ../..
	elif [[ ( $(uname -s) == 'Linux' ) && ( $HOST == *darwin* ) ]]; then
		echo "TODO: there seems to be no way to produce a .pkg mac installer"
		echo "from Linux as the pkgbuild/productbuild tools aren't available"
	elif [[ $(uname -s) == 'Darwin' ]]; then
		# Build a .pkg installer for macOS if we are on macOS and we are building for it
		cp uninstall.sh $DESTDIR$PREFIX
		# Prepare the postinstall script by replacing the correct prefix
		mkdir -p installers/macos/Scripts
		cat installers/macos/ScriptsTemplates/postinstall | \
			sed -e 's|PREFIX=|PREFIX='"$PREFIX"'|' > \
				installers/macos/Scripts/postinstall
		chmod +x installers/macos/Scripts/postinstall
		# Build a standard macOS package.
		# The wizard steps are configured by the Distribution.xml file.
		# Documentation:
		#   https://developer.apple.com/library/archive/documentation/
		#   DeveloperTools/Reference/DistributionDefinitionRef/Chapters/
		#   Introduction.html#//apple_ref/doc/uid/TP40005370-CH1-SW1
		# Also see `man productbuild` and `man pkgbuild`.
		pkgbuild \
			--identifier 'org.miosix.toolchain.gcc-9.2.0-mp3.1' \
			--version '9.2.0.3.1' \
			--install-location / \
			--scripts installers/macos/Scripts \
			--root $DESTDIR \
			'gcc-9.2.0-mp3.1.pkg'
		productbuild \
			--distribution installers/macos/Distribution.xml \
			--resources installers/macos/Resources \
			--package-path ./ \
			'./MiosixToolchainInstaller9.2.0mp3.1.pkg'
	fi
else
	# Install the uninstaller too
	chmod +x uninstall.sh
	$SUDO cp uninstall.sh $DESTDIR$PREFIX
	# If sudo not an empty variable and we are not on macOS, make symlinks to
	# /usr/bin. else make a script to override PATH
	if [[ ( $(uname -s) != 'Darwin' ) && $SUDO ]]; then
		$SUDO ln -s $DESTDIR$PREFIX/bin/* /usr/bin
	else
		echo '# Used when installing the compiler locally to test it' > env.sh
		echo '# usage: $ . ./env.sh' >> env.sh
		echo '# or     $ source ./env.sh' >> env.sh
		echo "export PATH=$PREFIX/bin:"'$PATH' >> env.sh
		chmod +x env.sh
	fi
fi

#
# The end.
#

echo ":: Successfully installed"
