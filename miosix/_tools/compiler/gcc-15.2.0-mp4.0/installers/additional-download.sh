#!/bin/sh

# This simple script will download additional sources required to make a
# distributable release build for linux/windows

# Meant to be run from the main compiler directory (./installers/additional-download.sh)
cd downloaded || exit

# macOS does not ship with wget, check if it exists and otherwise use curl
if command -v wget > /dev/null; then
	WGET=wget
else
	WGET='curl -LO'
fi

# Linux
$WGET https://ftpmirror.gnu.org/ncurses/ncurses-6.5.tar.gz
$WGET https://github.com/megastep/makeself/releases/download/release-2.6.0/makeself-2.6.0.run

# Windows
$WGET https://ftpmirror.gnu.org/make/make-4.4.1.tar.gz
$WGET https://jrsoftware.org/download.php/is.exe
mv is.exe innosetup.exe

# All
$WGET https://github.com/libexpat/libexpat/releases/download/R_2_7_3/expat-2.7.3.tar.xz
