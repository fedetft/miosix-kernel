#!/bin/sh

# This simple script will download all the required source files
# for compiling arm-miosix-eabi-gcc

wget http://ftp.gnu.org/gnu/binutils/binutils-2.23.1.tar.bz2
wget ftp://ftp.fu-berlin.de/unix/languages/gcc/releases/gcc-4.7.3/gcc-4.7.3.tar.bz2
wget ftp://sourceware.org/pub/newlib/newlib-2.0.0.tar.gz
wget ftp://bo.mirror.garr.it/mirrors/sourceware.org/gdb/releases/gdb-7.5.tar.bz2
wget https://gmplib.org/download/gmp/gmp-6.0.0a.tar.lz
mv gmp-6.0.0a.tar.lz gmp-6.0.0.tar.lz
wget http://www.mpfr.org/mpfr-current/mpfr-3.1.2.tar.xz
wget http://mirror2.mirror.garr.it/mirrors/gnuftp/gnu/mpc/mpc-1.0.2.tar.gz
