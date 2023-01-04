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

$WGET https://ftpmirror.gnu.org/binutils/binutils-2.32.tar.xz
$WGET https://ftpmirror.gnu.org/gcc/gcc-9.2.0/gcc-9.2.0.tar.xz
$WGET ftp://sourceware.org/pub/newlib/newlib-3.1.0.tar.gz
$WGET https://ftpmirror.gnu.org/gdb/gdb-9.1.tar.xz
$WGET https://ftpmirror.gnu.org/gmp/gmp-6.2.1.tar.xz
$WGET https://ftpmirror.gnu.org/mpfr/mpfr-4.0.2.tar.xz
$WGET https://ftpmirror.gnu.org/mpc/mpc-1.1.0.tar.gz
