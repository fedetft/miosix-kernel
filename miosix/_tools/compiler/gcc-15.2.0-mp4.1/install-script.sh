#!/usr/bin/env bash

# Script to build the gcc compiler required for Miosix.
# Usage: ./install-script -j`nproc`
# The -j parameter is passed to make for parallel compilation
#
# Starting from Miosix 1.58 the use of the arm-miosix-eabi-gcc compiler built
# by this script has become mandatory due to patches related to posix threads
# in newlib. The kernel *won't* compile unless the correct compiler is used.
#
# Starting from 04/2014 this script is also used to build binary releases
# of the Miosix compiler for both linux and windows. Most users will want to
# download the binary relase from https://miosix.org instead of compiling GCC
# using this script.

#### Configuration tunables -- begin ####

__GCCPATCUR='4.1' # Can't autodetect this one easily from gcc.patch

# This should be set to true unless you're installing locally on your Linux
# machine the compiler that will be used to do canadian cross compiling for
# making a Windows redistributable build. The compiler built with this variable
# set to false should only be used for this purpose, as it will contain the
# stubs.o in libc with dummy implementations of some core functionality which
# sometimes get linked in resulting in kernel builds being subtly non-functional
STRIP_STUBS_FROM_LIBC=true

# Uncomment if installing globally on this system
PREFIX=/opt/arm-miosix-eabi
DESTDIR=
SUDO=sudo
# Uncomment if installing locally on this system, sudo isn't necessary
#PREFIX=`pwd`/arm-miosix-eabi
#DESTDIR=
#SUDO=
# Uncomment for producing a package for redistribution. The prefix is set to the
# final install directory, but when this script does "make install" files are
# copied with $DESTDIR as prefix. When doing a redistibutable build you also
# have to specify HOST or (on Mac OS), BUILD, see below.
# When compiling the Windows installer, do not change the default values!
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
# You have to run this script from Linux anyway (see canadian cross compiling).
# Must first install the mingw-w64 toolchain.
#BUILD=
#HOST=x86_64-w64-mingw32
# Uncomment if targeting macOS 64 bit Intel (distributable), compiling on Linux
# Must first install the osxcross toolchain
#BUILD=
#HOST=x86_64-apple-darwin18
# Uncomment if targeting macOS 64 bit Intel (distributable), compiling on macOS.
# The script must be run under macOS and without canadian cross compiling
# because it confuses autotools's configuration scripts. Instead we set the
# compiler options for macOS minimum version and architecture in order to be
# able to deploy the binaries on older machines and OS versions. We also must
# force --build and --host to specify a x86_64 cpu to avoid
# architecture-dependent code.
#BUILD=x86_64-apple-darwin17
#HOST=
#export CFLAGS='-mmacos-version-min=10.13 -O3'
#export CXXFLAGS='-mmacos-version-min=10.13 -O3'
# Uncomment if targeting macOS 64 bit ARM64 (distributable).
# Run the script under arm64 macOS.
#BUILD=aarch64-apple-darwin20
#HOST=
#export CFLAGS='-mmacos-version-min=11.0 -O3'
#export CXXFLAGS='-mmacos-version-min=11.0 -O3'

#### Configuration tunables -- end ####

# Libraries are compiled statically, so they are never installed in the system
LIB_DIR=`pwd`/lib

# Program versions
BINUTILS=binutils-2.45
GCC=gcc-15.2.0
NEWLIB=newlib-4.6.0.20260123
GDB=gdb-16.3
GMP=gmp-6.3.0
MPFR=mpfr-4.2.2
MPC=mpc-1.3.1
NCURSES=ncurses-6.5
MAKE=make-4.4.1
MAKESELF=makeself-2.6.0
EXPAT=expat-2.7.3

quit() {
	echo $1
	exit 1
}

# Is it a redistributable build?
if [[ $DESTDIR ]]; then
	if [[ $SUDO ]]; then
		quit ":: Error global install and distributable compiling are mutually exclusive"
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
	# Clean up PATH to avoid finding system-installed libraries that
	# won't be present when actually installing the compiler.
	# This is mostly relevant for macOS, because Linux distributions
	# install all packages (system-required and user-requested) in the same
	# place, making redistributable builds basically impossible without
	# using a clean purpose-built VM.
	export PATH=/usr/bin:/bin:/usr/sbin:/sbin
	if [[ -d $PREFIX/arm-miosix-eabi ]]; then
		# If arm-miosix-eabi-gcc is already installed system-wide, make sure
		# it's the same version, otherwise we may by mistake build part of the
		# standard libraries with the old compiler
		__GCCVER=`arm-miosix-eabi-gcc --version | perl -ne \
			'next unless(/(\d+\.\d+.\d+)/); print "gcc-$1\n";'`;
		__GCCPAT=`arm-miosix-eabi-gcc -dM -E - < /dev/null | perl -e \
			'my $M, my $m;
			 while(<>) {
			 	$M=$1 if(/_MIOSIX_GCC_PATCH_MAJOR (\d+)/);
			 	$m=$1 if(/_MIOSIX_GCC_PATCH_MINOR (\d+)/);
			 }
			 print "mp$M.$m";'`;
		if [[ ($__GCCVER != $GCC) || ($__GCCPAT != "mp${__GCCPATCUR}") ]]; then
			quit ":: Uninstall the previous compiler version first"
		fi
	else
		# When building a redistributable build, we use DESTDIR. Thus, the
		# compiler is "installed" to $DESTDIR$PREFIX even though it is meant to
		# be run from $PREFIX. There is an issue though: building the standard
		# libraries requires the to-be-built compiler, which isn't found, so
		# building fails at the newlib stage. We wish the fix would just be a
		# export PATH=$DESTDIR$PREFIX/bin:$PATH
		# but turns out that isn't enough, as after newlib is built, the
		# subsequent libraries (part of gcc-end) don't just require the compiler,
		# they require the libc too that is installed in $DESTDIR$PREFIX, but
		# the compiler looks for it in $PREFIX only...
		# As a workaround, we temporarily do a symlink to make the compiler
		# and standard libraries available from their final path, $PREFIX during
		# the compilation process.
		# Moreover, just symlinking /opt/arm-miosix-eabi fails, so we need to
		# symlink only /opt/arm-miosix-eabi/arm-miosix-eabi
		export PATH=$DESTDIR$PREFIX/bin:$PATH
		mkdir -p $DESTDIR$PREFIX/arm-miosix-eabi
		# This must unconditionally be done with sudo so it's important we use
		# sudo and not $SUDO.
		sudo mkdir -p $PREFIX
		sudo ln -s $DESTDIR$PREFIX/arm-miosix-eabi $PREFIX/arm-miosix-eabi
	fi
else
	if [[ $HOST || $BUILD ]]; then
		# NOTE: doing a non redistributable build but specifying HOST or BUILD
		# may work, but is untested. Remove this line if you want to try.
		quit ":: Specifying either HOST or BUILD without DESTDIR is not supported"
	fi
	# Ensure the install destination is clean. If it is not, old and new
	# compiler files will mix up, potentially making the compilation fail.
	if [[ -d $PREFIX ]]; then
		quit ":: Uninstall (or move away) the existing compiler first"
	fi
	# Add the install prefix to the path in order to ensure tools are
	# available as soon as we build them.
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
	if command -v nproc > /dev/null; then
		PARALLEL="-j$(nproc)"
	elif [[ $(uname -s) == 'Darwin' ]]; then
		PARALLEL="-j$(sysctl -n hw.logicalcpu)"
	else
		PARALLEL="-j1";
	fi
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
	shift 2
	directory=${filename%.tar*}
	
	if [[ -e $directory ]]; then
		echo "Skipping extraction/patching of $label, directory $directory exists"
	else
		echo "Extracting $label..."
		tar -xf "downloaded/$filename" || quit ":: Error extracting $label"
		for patchfile in $@; do
			echo "Applying ${patchfile}..."
			patch -p0 < "$patchfile" || quit ":: Failed patching $label"
		done
	fi
}

extract 'binutils' $BINUTILS.tar.xz patches/binutils.patch
extract 'gcc' $GCC.tar.xz patches/gcc.patch
extract 'newlib' $NEWLIB.tar.gz patches/newlib.patch
extract 'gdb' $GDB.tar.xz patches/gdb.patch
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

mkdir log

#
# Part 3: compile libraries
#

cd $GMP

if [[ $HOST ]]; then
	# GMP's configure script is bugged and does not properly handle canadian cross
	# compiling, so we need to properly inform it manually by setting these
	# environment variables. See also: https://gmplib.org/list-archives/gmp-discuss/2020-July/006519.html
	export CC_FOR_BUILD='gcc'
	export CPP_FOR_BUILD='g++'
fi

if [[ $(uname -s) == 'Darwin' ]]; then
	# On macOS, the assembly implementations in GMP intermittently cause
	# either compilation failures or broken builds. Disable them
	MX_GMP_ASSEMBLY="--disable-assembly"
else
	MX_GMP_ASSEMBLY=""
fi

echo "Configuring $GMP..."
./configure \
	--build=$BUILD \
	--host=$HOST \
	--prefix=$LIB_DIR \
	--enable-static --disable-shared \
	$MX_GMP_ASSEMBLY \
	&> ../log/03_1_gmp_1_configure.txt \
	|| quit ":: Error configuring gmp"

echo "Building $GMP..."
make all $PARALLEL &> ../log/03_1_gmp_2_build.txt \
	|| quit ":: Error compiling gmp"

echo "Testing $GMP..."
if [[ ! $HOST ]]; then
	# Don't check if cross-compiling
	make check $PARALLEL &> ../log/03_1_gmp_3_check.txt \
	|| quit ":: Error testing gmp"
fi

echo "Installing $GMP..."
make install &>../log/03_1_gmp_4_install.txt \
	|| quit ":: Error installing gmp"

if [[ $HOST ]]; then
	unset CC_FOR_BUILD
	unset CPP_FOR_BUILD
fi

cd ..

cd $MPFR

echo "Configuring $MPFR..."
./configure \
	--build=$BUILD \
	--host=$HOST \
	--prefix=$LIB_DIR \
	--enable-static --disable-shared \
	--with-gmp=$LIB_DIR \
	&> ../log/03_2_mpfr_1_configure.txt \
	|| quit ":: Error configuring mpfr"

echo "Building $MPFR..."
make all $PARALLEL &> ../log/03_2_mpfr_2_build.txt \
	|| quit ":: Error compiling mpfr"

echo "Testing $MPFR..."
if [[ ! $HOST ]]; then
	# Don't check if cross-compiling
	make check $PARALLEL &> ../log/03_2_mpfr_3_check.txt \
	|| quit ":: Error testing mpfr"
fi

echo "Installing $MPFR..."
make install &>../log/03_2_mpfr_4_install.txt \
	|| quit ":: Error installing mpfr"

cd ..

cd $MPC

echo "Configuring $MPC..."
./configure \
	--build=$BUILD \
	--host=$HOST \
	--prefix=$LIB_DIR \
	--enable-static --disable-shared \
	--with-gmp=$LIB_DIR \
	--with-mpfr=$LIB_DIR \
	&> ../log/03_3_mpc_1_configure.txt \
	|| quit ":: Error configuring mpc"

echo "Building $MPC..."
make all $PARALLEL &> ../log/03_3_mpc_2_build.txt \
	|| quit ":: Error compiling mpc"

echo "Testing $MPC..."
if [[ ! $HOST ]]; then
	# Don't check if cross-compiling for windows
	make check $PARALLEL &>../log/03_3_mpc_3_check.txt \
	|| quit ":: Error testing mpc"
fi

echo "Installing $MPC..."
make install &>../log/03_3_mpc_4_install.txt \
	|| quit ":: Error installing mpc"

cd ..

#
# Part 4: compile and install binutils
#

mkdir binutils_build
cd binutils_build

echo "Configuring $BINUTILS..."
../$BINUTILS/configure \
	--build=$BUILD \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$PREFIX \
	--enable-interwork \
	--enable-multilib \
	--enable-lto \
	--disable-werror &>../log/04_binutils_1_configure.txt \
	|| quit ":: Error configuring binutils"

echo "Building $BINUTILS..."
make all $PARALLEL &>../log/04_binutils_2_build.txt \
	|| quit ":: Error compiling binutils"

echo "Installing $BINUTILS..."
$SUDO make install DESTDIR=$DESTDIR &>../log/04_binutils_3_install.txt \
	|| quit ":: Error installing binutils"

cd ..

#
# Part 5: compile and install gcc-start
#

mkdir gcc_build
cd gcc_build

# GCC needs the C headers of the target to configure and build the C++ standard
# library, therefore when configured --with-headers=[...] the configure script
# unconditionally copies those headers in the
# $PREFIX/arm-miosix-eabi/sys-include folder.
#   This is fine for local installs, (up to a certain point, see later
# comments).
#   However the GCC makefiles are not clever enough to search in `include`
# rather than `sys-include` when checking if limits.h exists to decide whether
# to "fix" it. Since `sys-include` does not exists, GCC does not find limits.h
# and replaces it with its own, which does not include all definitions made
# by newlib's one. This incorrect file ends up installed and used by the
# built GCC causing build failures usually related to missing defines used
# by dirent.h.
__GCC_CONF_HEADERS_PARAM=--with-headers=../$NEWLIB/newlib/libc/include

# About --enable-libstdcxx-static-eh-pool, --with-libstdcxx-eh-pool-obj-count=3:
# we used to patch eh_alloc.cc to reduce the C++ emergency exception allocation
# pool to limit the amount of RAM it uses. In GCC 15.2.0 it is no longer needed
# as it can be configured. The formula for the used RAM is:
# pool_size = EMERGENCY_OBJ_COUNT * (EMERGENCY_OBJ_SIZE * P + R + D)
# EMERGENCY_OBJ_COUNT is set via --with-libstdcxx-eh-pool-obj-count, so is 3
# EMERGENCY_OBJ_SIZE is fixed to 6, a reasonable exception size
# P is sizeof(void*), 4 on ARM
# R is sizeof(__cxa_refcounted_exception), 128 on ARM
# D is sizeof(__cxa_dependent_exception), 120 on RAM
# This is about as small as it can get in a multithreaded system.
echo "Configuring $GCC (start)..."
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
	--enable-libgomp \
	--disable-libstdcxx-pch \
	--disable-libstdcxx-dual-abi \
	--disable-libstdcxx-filesystem-ts \
	--enable-libstdcxx-static-eh-pool \
	--with-libstdcxx-eh-pool-obj-count=3 \
	--enable-threads=miosix \
	--enable-languages="c,c++" \
	--enable-lto \
	--disable-wchar_t \
	--with-newlib \
	${__GCC_CONF_HEADERS_PARAM} \
	--with-pkgversion="GCC_mp${__GCCPATCUR}" \
	&>../log/05_gcc-start_1_configure.txt \
	|| quit ":: Error configuring gcc-start"

echo "Building $GCC (start)..."
$SUDO make all-gcc $PARALLEL &>../log/05_gcc-start_2_build.txt \
	|| quit ":: Error compiling gcc-start"

echo "Installing $GCC (start)..."
$SUDO make install-gcc DESTDIR=$DESTDIR &>../log/05_gcc-start_3_install.txt \
	|| quit ":: Error installing gcc-start"

# Remove the sys-include directory if we are installing locally.
# There are two reasons why to remove it: first because it is unnecessary,
# second because it is harmful.
# After gcc is compiled, the installation of newlib places the headers in the
# include dirctory and at that point the sys-include headers aren't necessary anymore
# Now, to see why the're harmful, consider the header newlib.h It is initially
# empty and is filled in by the newlib's ./configure with the appropriate options
# Now, since the configure process happens after, the newlib.h in sys-include
# is the wrong (empty) one, while the one in include is the correct one.
# This causes troubles because newlib.h contains configuration options that are
# used by other headers in libc, and the misconfiguration becomes visible to
# user code since GCC seems to take the wrong newlib.h
$SUDO rm -rf $DESTDIR$PREFIX/arm-miosix-eabi/sys-include

cd ..

#
# Part 6: compile and install newlib
#

mkdir newlib_build
cd newlib_build

echo "Configuring $NEWLIB..."
../$NEWLIB/configure \
	--build=$BUILD \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$PREFIX \
	--enable-multilib \
	--enable-newlib-multithread \
	--enable-newlib-io-long-long \
	--enable-newlib-use-malloc-in-execl \
	--disable-newlib-io-c99-formats \
	--disable-newlib-io-long-double \
	--disable-newlib-io-pos-args \
	--disable-newlib-mb \
	--disable-newlib-supplied-syscalls \
	&>../log/06_newlib_1_configure.txt \
	|| quit ":: Error configuring newlib"

echo "Building $NEWLIB..."
make MAKEINFO=/usr/bin/true $PARALLEL &>../log/06_newlib_2_build.txt \
	|| quit ":: Error compiling newlib"

echo "Installing $NEWLIB..."
$SUDO make install MAKEINFO=/usr/bin/true PATH=$PATH DESTDIR=$DESTDIR &>../log/06_newlib_3_install.txt \
	|| quit ":: Error installing newlib"

cd ..

#
# Part 7: compile and install gcc-end
#

# Install the linker file for processes
echo "Installing process linker script..."
cd libsyscalls
PREFIX=$PREFIX SUDO=$SUDO DESTDIR=$DESTDIR ./install_linkerscript.sh \
	|| quit ":: Error installing process linker script"
cd ..

# Build and install GCC's libraries
cd gcc_build
echo "Building $GCC (end)..."
$SUDO make all $PARALLEL PATH=$PATH &> ../log/07_gcc-end_1_build.txt \
	|| quit ":: Error compiling gcc-end"
echo "Installing $GCC (end)..."
$SUDO make install PATH=$PATH DESTDIR=$DESTDIR &>../log/07_gcc-end_2_install.txt \
	|| quit ":: Error installing gcc-end"
cd ..

# Install the real libsyscalls
echo "Installing libsyscalls..."
cd libsyscalls
PREFIX=$PREFIX SUDO=$SUDO DESTDIR=$DESTDIR ./install_multilibs.sh \
	|| quit ":: Error installing libsyscalls"
cd ..

#
# Part 8: Fixup and verify multilibs
#

all_multilibs=$(arm-miosix-eabi-gcc --print-multi-lib)
for libspec in $all_multilibs; do
    libspec_parts=(${libspec//;/ })
    MULTILIB_PATH=${libspec_parts[0]}
    echo "Multilib path $MULTILIB_PATH"
    FULL_PATH=$DESTDIR$PREFIX/arm-miosix-eabi/lib/$MULTILIB_PATH
    if [[ -f $FULL_PATH/libc.a ]]; then
        ## stubs.c was added to newlib as part of the Miosix patches to make
        ## it possible to compile binaries before libsyscalls is installed.
        ## This is necessary as configure scripts in gcc-end like to build and
        ## link binaries to see if certain features are present.
        ## However, after gcc-end is done, stubs.o should never be linked as
        ## the functions it provides are either provided by libsyscalls for
        ## userspace applications, or the kernel itself for kernelspace
        ## applications. Since it was found that sometimes the linker selected
        ## stubs.o, it is harmful to keep it, so remove it from libc.a
        ## NOTE: check after every compiler release, for example from newlib
        ## 3.1.0 to 4.6.0 the name changed from lib_a-stubs.o to libc_a-stubs.o
        if [[ $STRIP_STUBS_FROM_LIBC = true ]]; then
            $SUDO arm-miosix-eabi-ar d $FULL_PATH/libc.a libc_a-stubs.o
            $SUDO arm-miosix-eabi-ranlib $FULL_PATH/libc.a
        fi
        ## All those files aren't needed, so remove them. TODO: try to convince
        ## newlib to not produce them in the first place
        $SUDO rm -f $FULL_PATH/cpu-init/rdimon-aem.o
        $SUDO rmdir $FULL_PATH/cpu-init
        $SUDO rm -f $FULL_PATH/libgloss-linux.a
        $SUDO rm -f $FULL_PATH/libnosys.a
        $SUDO rm -f $FULL_PATH/crt0.o
        $SUDO rm -f $FULL_PATH/nosys.specs
        $SUDO rm -f $FULL_PATH/librdimon.a
        $SUDO rm -f $FULL_PATH/rdimon-crt0.o
        $SUDO rm -f $FULL_PATH/rdimon.specs
        $SUDO rm -f $FULL_PATH/librdpmon.a
        $SUDO rm -f $FULL_PATH/rdpmon-crt0.o
        $SUDO rm -f $FULL_PATH/rdpmon.specs
        $SUDO rm -f $FULL_PATH/librdimon-v2m.a
        $SUDO rm -f $FULL_PATH/rdimon-crt0-v2m.o
        $SUDO rm -f $FULL_PATH/rdimon-v2m.specs
        $SUDO rm -f $FULL_PATH/linux-crt0.o
        $SUDO rm -f $FULL_PATH/linux.specs
        $SUDO rm -f $FULL_PATH/pid.specs
        $SUDO rm -f $FULL_PATH/redboot-crt0.o
        $SUDO rm -f $FULL_PATH/redboot-syscalls.o
        $SUDO rm -f $FULL_PATH/redboot.ld
        $SUDO rm -f $FULL_PATH/redboot.specs
        $SUDO rm -f $FULL_PATH/nano.specs
        $SUDO rm -f $FULL_PATH/iq80310.specs
        $SUDO rm -f $FULL_PATH/aprofile-validation-v2m.specs
        $SUDO rm -f $FULL_PATH/aprofile-validation.specs
        $SUDO rm -f $FULL_PATH/aprofile-ve-v2m.specs
        $SUDO rm -f $FULL_PATH/aprofile-ve.specs
    fi
done

# 8A: remove root multilib.
# GCC apparently assumes that when no appropriate multilib is found, it is
# always safe to link without multilibs (i.e. with the libraries found directly
# in /lib). However, for the ARM architecture this assumption is completely
# wrong, due to (1) the presence of 3 mutually incompatible instruction sets
# (ARM, Thumb and Thumb2) and (2) the fact that the default compilation options
# build for an extremely old configuration (-mcpu=arm7tdmi -marm) which produces
# code not runnable on any modern ARM microcontroller CPU.
# This line removes all the libraries not included in a multilib to prevent
# GCC from producing broken binaries silently.
# As a result, the use of compilation options not supported by the toolchain
# will result in a link-time failure to find the libraries, hinting that
# something is wrong.

echo "Deleting root multilibs..."
$SUDO rm "$DESTDIR$PREFIX/arm-miosix-eabi/lib"/*.specs
$SUDO rm "$DESTDIR$PREFIX/arm-miosix-eabi/lib"/*.o
$SUDO rm "$DESTDIR$PREFIX/arm-miosix-eabi/lib"/*.a
$SUDO rm "$DESTDIR$PREFIX/arm-miosix-eabi/lib"/*.ld
$SUDO rm -rf "$DESTDIR$PREFIX/arm-miosix-eabi/lib/cpu-init"
$SUDO rm "$DESTDIR$PREFIX/lib/gcc/arm-miosix-eabi/15.2.0"/*.o
$SUDO rm "$DESTDIR$PREFIX/lib/gcc/arm-miosix-eabi/15.2.0"/*.a


# 8B: check that all multilibs have been built.
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

check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/arm/v4t/nofp/kernel
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/arm/v4t/nofp/kernel/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v4t/nofp/kernel
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v4t/nofp/kernel/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v6-m/nofp
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v6-m/nofp/kernel
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v6-m/nofp/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v6-m/nofp/kernel/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7-m/nofp
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7-m/nofp/kernel
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7-m/nofp/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7-m/nofp/kernel/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7e-m+fp/hard
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7e-m+fp/hard/kernel
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7e-m+fp/hard/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7e-m+fp/hard/kernel/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7e-m+dp/hard
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7e-m+dp/hard/kernel
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7e-m+dp/hard/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v7e-m+dp/hard/kernel/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.base/nofp
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.base/nofp/kernel
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.base/nofp/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.base/nofp/kernel/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.main+fp/hard
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.main+fp/hard/kernel
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.main+fp/hard/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.main+fp/hard/kernel/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.main+dp/hard
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.main+dp/hard/kernel
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.main+dp/hard/noexceptions
check_multilibs $DESTDIR$PREFIX/arm-miosix-eabi/lib/thumb/v8-m.main+dp/hard/kernel/noexceptions
echo "Checked multilibs: all have been built!"

#
# Part 9: compile and install gdb
#

# GDB on linux/windows needs expat
if [[ $DESTDIR ]]; then
	cd $EXPAT
	
	echo "Configuring $EXPAT..."
	./configure \
		--build=$BUILD \
		--host=$HOST \
		--prefix=$LIB_DIR \
		--enable-static=yes \
		--enable-shared=no \
		&> ../log/09_expat_1_configure.txt \
		|| quit ":: Error configuring expat"

	echo "Building $EXPAT..."
	make all $PARALLEL &>../log/09_expat_2_build.txt \
		|| quit ":: Error compiling expat"

	echo "Installing $EXPAT..."
	make install &>../log/09_expat_3_install.txt \
		|| quit ":: Error installing expat"

	cd ..
fi

# GDB on linux requires ncurses, and not to depend on them when doing a
# redistributable linux build we build a static version
# Based on previous gdb that when run with --tui reported as error
# "Error opening terminal: xterm-256color" we now build this terminal as
# fallback within ncurses itself.
if [[ $HOST == *linux* ]]; then
	cd $NCURSES

	echo "Configuring $NCURSES..."
	./configure \
		--build=$BUILD \
		--host=$HOST \
		--prefix=$LIB_DIR \
		--with-normal --without-shared \
		--without-ada --without-cxx-binding --without-debug \
		--with-fallbacks='xterm-256color' \
		--without-manpages --without-progs --without-tests \
		&> ../log/10_ncurses_1_configure.txt \
		|| quit ":: Error configuring ncurses"

	echo "Building $NCURSES..."
	make all $PARALLEL &>../log/10_ncurses_2_build.txt \
		|| quit ":: Error compiling ncurses"

	echo "Installing $NCURSES..."
	make install &>../log/10_ncurses_3_install.txt \
		|| quit ":: Error installing ncurses"

	cd ..
fi

mkdir gdb_build
cd gdb_build

# CXX=$HOSTCXX to avoid having to distribute libstdc++.dll on windows
echo "Configuring $GDB..."
CXX=$HOSTCXX ../$GDB/configure \
	--build=$BUILD \
	--host=$HOST \
	--target=arm-miosix-eabi \
	--prefix=$PREFIX \
	--with-gmp=$LIB_DIR \
	--with-mpfr=$LIB_DIR \
	--with-libmpfr-prefix=$LIB_DIR \
	--with-libexpat-prefix=$LIB_DIR \
	--with-system-zlib=no \
	--with-lzma=no \
	--with-python=no \
	--enable-interwork \
	--enable-multilib \
	--disable-werror &>../log/11_gdb_1_configure.txt \
	|| quit ":: Error configuring gdb"

# Specify a dummy MAKEINFO binary to work around an issue in the gdb makefiles
# where compilation fails if MAKEINFO is not installed.
# https://sourceware.org/bugzilla/show_bug.cgi?id=14678
echo "Building $GDB..."
make all MAKEINFO=/usr/bin/true $PARALLEL &>../log/11_gdb_2_build.txt \
	|| quit ":: Error compiling gdb"

echo "Installing $GDB..."
$SUDO make install MAKEINFO=/usr/bin/true PATH=$PATH DESTDIR=$DESTDIR &>../log/11_gdb_3_install.txt \
	|| quit ":: Error installing gdb"

cd ..

#
# Part 10: install the postlinker
#
cd mx-postlinker
echo "Installing mx-postlinker..."
make CXX="$HOSTCXX" SUFFIX=$EXT \
	|| quit ":: Error compiling mx-postlinker"
$SUDO make install CXX="$HOSTCXX" SUFFIX=$EXT \
	INSTALL_DIR=$DESTDIR$PREFIX/bin \
	|| quit ":: Error installing mx-postlinker"
make CXX="$HOSTCXX" SUFFIX=$EXT clean
cd ..

#
# Part 11: install GNU make and rm (windows release only)
#

if [[ $HOST == *mingw* ]]; then

	cd $MAKE

	echo "Configuring $MAKE..."
	./configure \
		--build=$BUILD \
		--host=$HOST \
		--prefix=$PREFIX &> z.make.a.txt \
		|| quit ":: Error configuring make"

	echo "Building $MAKE..."
	make all $PARALLEL &>../log/z.make.b.txt \
		|| quit ":: Error compiling make"

	echo "Installing $MAKE..."
	make install DESTDIR=$DESTDIR &>../log/z.make.c.txt \
		|| quit ":: Error installing make"

	cd ..

	# FIXME get a better rm to distribute for windows
	echo "Installing rm..."
	$HOSTCC -o rm$EXT -O2 installers/windows/rm.c \
		|| quit ":: Error compiling rm"

	mv rm$EXT $DESTDIR$PREFIX/bin \
		|| quit ":: Error installing rm"
fi

#
# Part 12: Final fixups
#

# Remove this since its name is not arm-miosix-eabi-
$SUDO rm $DESTDIR$PREFIX/bin/arm-miosix-eabi-$GCC$EXT
# Remove this since it is useless when cross-compiling
$SUDO rm $DESTDIR$PREFIX/bin/arm-miosix-eabi-gstack

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
		echo "Building Linux makeself installer..."
		sed -E "s|/opt/arm-miosix-eabi|$PREFIX|g" installers/linux/installer.sh > $DESTDIR$PREFIX/installer.sh
		sed -E "s|/opt/arm-miosix-eabi|$PREFIX|g" uninstall.sh > $DESTDIR$PREFIX/uninstall.sh
		chmod +x $DESTDIR$PREFIX/installer.sh $DESTDIR$PREFIX/uninstall.sh
		sh downloaded/$MAKESELF.run
		# NOTE: --keep-umask otherwise the installer extracts files setting to 0
		# permissions to group and other, resulting in an unusable installation
		./$MAKESELF/makeself.sh --xz --keep-umask \
			$DESTDIR$PREFIX \
			MiosixToolchainInstaller15.2.0mp4.1.run \
			"Miosix toolchain for Linux (GCC 15.2.0-mp4.1)" \
			"./installer.sh"
	elif [[ ( $(uname -s) == 'Linux' ) && ( $HOST == *mingw* ) ]]; then
		# Build an executable installer for Windows
		echo "Building Windows InnoSetup installer..."
		cd installers/windows
		wine "C:\Program Files (x86)\Inno Setup 6\Compil32.exe" /cc MiosixInstaller.iss
		cd ../..
	elif [[ ( $(uname -s) == 'Linux' ) && ( $HOST == *darwin* ) ]]; then
		echo "TODO: there seems to be no way to produce a .pkg mac installer"
		echo "from Linux as the pkgbuild/productbuild tools aren't available"
	elif [[ $(uname -s) == 'Darwin' ]]; then
		# Build a .pkg installer for macOS if we are on macOS and we are building for it
		echo "Building macOS package..."
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
		distr_script='installers/macos/Distribution_Intel.xml'
		suffix='Intel'
		if [[ $BUILD == aarch64* ]]; then
		  distr_script='installers/macos/Distribution_ARM.xml'
		  suffix='ARM'
		fi
		# Detect selected minimum OS version from $CFLAGS
		min_os_ver=$(echo "$CFLAGS" | sed -E 's/.*-mmacos-version-min=([^ ]+).*/\1/g')
		if [[ -z "${min_os_ver}" ]]; then
		  # Not specified in $CFLAGS: use the OS version associated to the SDK
		  # we are using
		  min_os_ver="$(xcrun --show-sdk-version)"
		fi
		
		pkgbuild \
			--identifier 'org.miosix.toolchain.gcc' \
			--version "15.2.0.${__GCCPATCUR}" \
			--min-os-version "${min_os_ver}" \
			--install-location / \
			--scripts installers/macos/Scripts \
			--root $DESTDIR \
			"gcc.pkg"
		productbuild \
			--distribution ${distr_script} \
			--resources installers/macos/Resources \
			--package-path ./ \
			"./MiosixToolchainInstaller15.2.0mp${__GCCPATCUR}_${suffix}.pkg"
	fi
else
	# Install the uninstaller too
	echo "Installing uninstall script..."
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

if [[ -h $PREFIX/arm-miosix-eabi ]]; then
	# Symlink of $DESTDIR$PREFIX/arm-miosix-eabi to $PREFIX/arm-miosix-eabi no
	# longer required.
	# This must unconditionally be done with sudo so it's important we use
	# sudo and not $SUDO.
	sudo rm $PREFIX/arm-miosix-eabi
	sudo rmdir $PREFIX
fi

#
# The end.
#

echo "Successfully installed!"
