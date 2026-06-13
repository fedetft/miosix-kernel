#!/bin/sh

# This simple script will download all the required source files
# for compiling the Miosix gcc toolchain

mkdir -p downloaded
cd downloaded

# macOS does not ship with wget, check if it exists and otherwise use curl
if command -v wget > /dev/null; then
	WGET=wget
else
	WGET='curl -LO'
fi

# Idempotent: skip a file that is already present (lets a pre-populated
# downloaded/ dir be reused, e.g. cached across Docker builds).
fetch() {
	url="$1"
	file="${url##*/}"
	if [ -s "$file" ]; then
		echo "Already have $file, skipping"
	else
		$WGET "$url"
	fi
}

fetch https://ftp.gnu.org/gnu/binutils/binutils-2.45.tar.xz
fetch https://ftp.gnu.org/gnu/gcc/gcc-15.2.0/gcc-15.2.0.tar.xz
fetch https://sourceware.org/pub/newlib/newlib-4.6.0.20260123.tar.gz
fetch https://ftp.gnu.org/gnu/gdb/gdb-16.3.tar.xz
fetch https://ftp.gnu.org/gnu/gmp/gmp-6.3.0.tar.xz
fetch https://ftp.gnu.org/gnu/mpfr/mpfr-4.2.2.tar.xz
fetch https://ftp.gnu.org/gnu/mpc/mpc-1.3.1.tar.gz
