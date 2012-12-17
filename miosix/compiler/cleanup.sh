#!/bin/bash

# After running install-script.sh, this script will clean up temporary files

rm -rf binutils-2.21.1/ gcc-4.5.2/ gdb-7.0.1/ newlib-1.19.0/ newlib-obj/ lpc21isp.c

sudo rm -rf objdir/ log/
