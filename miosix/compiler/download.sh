#!/bin/sh

# This simple script will download all the required source files
# for compiling arm-miosix-eabi-gcc

wget http://ftp.gnu.org/gnu/binutils/binutils-2.23.1.tar.bz2
wget ftp://gd.tuwien.ac.at/gnu/gcc/releases/gcc-4.7.2/gcc-4.7.2.tar.bz2
wget ftp://sourceware.org/pub/newlib/newlib-2.0.0.tar.gz
wget ftp://bo.mirror.garr.it/mirrors/sourceware.org/gdb/releases/gdb-7.5.tar.bz2
