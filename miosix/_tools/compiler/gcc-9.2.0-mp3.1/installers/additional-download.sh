#!/bin/sh

# This simple script will download additional sources required to make a
# distributable release build for linux/windows

# Meant to be run from the main compiler directory (./installers/additional-download.sh)
cd downloaded || exit

# Linux
wget https://ftpmirror.gnu.org/ncurses/ncurses-6.1.tar.gz
wget https://github.com/megastep/makeself/releases/download/release-2.4.0/makeself-2.4.0.run

# Windows
wget https://ftpmirror.gnu.org/make/make-4.2.1.tar.gz
wget https://github.com/fpoussin/QStlink2/releases/download/v1.2.2/qstlink2_1.2.2.exe
mv qstlink2_1.2.2.exe qstlink2.exe
wget https://jrsoftware.org/download.php/is.exe
mv is.exe innosetup.exe

# Both
wget https://github.com/libexpat/libexpat/releases/download/R_2_2_9/expat-2.2.9.tar.xz
