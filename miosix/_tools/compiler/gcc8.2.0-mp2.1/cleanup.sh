#!/bin/bash

# After running install-script.sh, this script will clean up temporary files

rm -rf binutils-2.31.1 gcc-8.2.0 gdb-8.1.1 newlib-3.0.0.20180802 newlib-obj \
	gmp-6.1.2 mpfr-4.0.1 mpc-1.1.0 make-4.0 ncurses-5.9 makeself-2.1.5 \
	lib quickfix lpc21isp.c

if [[ `stat -c "%U" objdir` == 'root' ]]; then
    sudo rm -rf objdir/ log/
else
    rm -rf objdir/ log/
fi

