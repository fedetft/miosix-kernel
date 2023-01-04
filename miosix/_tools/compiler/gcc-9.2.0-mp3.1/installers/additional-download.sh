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
$WGET https://ftpmirror.gnu.org/ncurses/ncurses-6.1.tar.gz
$WGET https://github.com/megastep/makeself/releases/download/release-2.4.5/makeself-2.4.5.run

# Windows
$WGET https://ftpmirror.gnu.org/make/make-4.2.1.tar.gz
$WGET https://github.com/fpoussin/QStlink2/releases/download/v1.2.2/qstlink2_1.2.2.exe
mv qstlink2_1.2.2.exe qstlink2.exe
$WGET https://jrsoftware.org/download.php/is.exe
mv is.exe innosetup.exe

# All
$WGET https://github.com/libexpat/libexpat/releases/download/R_2_2_10/expat-2.2.10.tar.xz
