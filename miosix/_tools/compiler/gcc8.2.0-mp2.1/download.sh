#!/bin/sh

# This simple script will download all the required source files
# for compiling arm-miosix-eabi-gcc

wget http://ftp.gnu.org/gnu/binutils/binutils-2.31.1.tar.xz
wget ftp://ftp.fu-berlin.de/unix/languages/gcc/releases/gcc-8.2.0/gcc-8.2.0.tar.xz
wget ftp://sourceware.org/pub/newlib/newlib-3.0.0.20180802.tar.gz
wget https://mirror2.mirror.garr.it/mirrors/gnuftp/gdb/gdb-8.1.1.tar.xz
wget https://ftp.gnu.org/gnu/gmp/gmp-6.1.2.tar.xz
wget https://www.mpfr.org/mpfr-current/mpfr-4.0.1.tar.xz
wget https://ftp.gnu.org/gnu/mpc/mpc-1.1.0.tar.gz
