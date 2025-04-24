#!/bin/sh

# This simple script will download all the required source files
# for compiling arm-miosix-eabi-gcc

mkdir downloaded || exit
cd downloaded

# macOS does not ship with wget, check if it exists and otherwise use curl
if command -v wget > /dev/null; then
	WGET=wget
else
	WGET='curl -LO'
fi

$WGET https://ftpmirror.gnu.org/binutils/binutils-2.44.tar.xz
$WGET https://ftpmirror.gnu.org/gcc/gcc-14.2.0/gcc-14.2.0.tar.xz
$WGET https://sourceware.org/pub/newlib/newlib-3.1.0.tar.gz
$WGET https://ftpmirror.gnu.org/gdb/gdb-16.2.tar.xz
$WGET https://ftpmirror.gnu.org/gmp/gmp-6.3.0.tar.xz
$WGET https://ftpmirror.gnu.org/mpfr/mpfr-4.2.1.tar.xz
$WGET https://ftpmirror.gnu.org/mpc/mpc-1.3.1.tar.gz
