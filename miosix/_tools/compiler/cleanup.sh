#!/bin/sh

# After running install-script.sh, this script will clean up temporary files

rm -rf binutils-2.23.1/ gcc-4.7.2/ gdb-7.5/ newlib-2.0.0/ newlib-obj/ lpc21isp.c

sudo rm -rf objdir/ log/
