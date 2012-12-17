#!/bin/bash

# This simple script will download all the required source files
# for compiling arm-miosix-eabi-gcc

wget http://ftp.gnu.org/gnu/binutils/binutils-2.21.1.tar.bz2
wget ftp://gd.tuwien.ac.at/gnu/gcc/releases/gcc-4.5.2/gcc-4.5.2.tar.bz2
wget ftp://sources.redhat.com/pub/newlib/newlib-1.19.0.tar.gz
wget ftp://sourceware.org/pub/gdb/releases/gdb-7.0.1a.tar.bz2

mv gdb-7.0.1a.tar.bz2 gdb-7.0.1.tar.bz2 #Quirk on gdb file name
