#!/bin/sh

# This simple script will download additional surces required to make a
# distributable release build for windows

wget http://mirror2.mirror.garr.it/mirrors/gnuftp/gnu/make/make-4.0.tar.bz2
wget https://qstlink2.googlecode.com/files/qstlink2_1.0.3.exe
mv qstlink2_1.0.3.exe qstlink2.exe
wget http://www.jrsoftware.org/download.php/is.exe
mv is.exe innosetup.exe
