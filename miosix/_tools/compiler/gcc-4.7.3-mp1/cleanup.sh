#!/bin/sh

# After running install-script.sh, this script will clean up temporary files

rm -rf binutils-2.23.1 gcc-4.7.3 gdb-7.5 newlib-2.0.0 newlib-obj \
	gmp-6.0.0 mpfr-3.1.2 mpc-1.0.2 make-4.0 ncurses-5.9 makeself-2.1.5 \
	lib quickfix lpc21isp.c

sudo rm -rf objdir/ log/
