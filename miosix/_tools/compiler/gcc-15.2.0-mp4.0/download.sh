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

$WGET https://ftp.gnu.org/gnu/binutils/binutils-2.45.tar.xz
$WGET https://ftp.gnu.org/gnu/gcc/gcc-15.2.0/gcc-15.2.0.tar.xz
$WGET https://sourceware.org/pub/newlib/newlib-4.6.0.20260123.tar.gz
$WGET https://ftp.gnu.org/gnu/gdb/gdb-16.3.tar.xz
$WGET https://ftp.gnu.org/gnu/gmp/gmp-6.3.0.tar.xz
$WGET https://ftp.gnu.org/gnu/mpfr/mpfr-4.2.2.tar.xz
$WGET https://ftp.gnu.org/gnu/mpc/mpc-1.3.1.tar.gz
