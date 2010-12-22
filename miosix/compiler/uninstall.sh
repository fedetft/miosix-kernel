#!/bin/bash

# Uninstall script: removes the arm-miosix-eabi-gcc compiler

cd /usr/bin

sudo rm -f                                          \
arm-miosix-eabi-addr2line  arm-miosix-eabi-gprof    \
arm-miosix-eabi-ar         arm-miosix-eabi-ld       \
arm-miosix-eabi-as         arm-miosix-eabi-nm       \
arm-miosix-eabi-c++        arm-miosix-eabi-objcopy  \
arm-miosix-eabi-c++filt    arm-miosix-eabi-objdump  \
arm-miosix-eabi-cpp        arm-miosix-eabi-ranlib   \
arm-miosix-eabi-g++        arm-miosix-eabi-readelf  \
arm-miosix-eabi-gcc        arm-miosix-eabi-run      \
arm-miosix-eabi-gccbug     arm-miosix-eabi-size     \
arm-miosix-eabi-gcov       arm-miosix-eabi-strings  \
arm-miosix-eabi-gdb        arm-miosix-eabi-strip    \
arm-miosix-eabi-gdbtui     lpc21isp

cd /opt

sudo rm -rf arm-miosix-eabi/
