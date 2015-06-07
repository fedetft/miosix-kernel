#!/bin/bash

# Uninstall script: removes the arm-miosix-eabi-gcc compiler

PREFIX="arm-miosix-eabi-"
FILES="addr2line ar as c++ c++filt cpp elfedit g++ gcc gcc-ar gcc-nm gcc-ranlib gccbug gcov gdb gdbtui gprof ld ld.bfd nm objcopy objdump ranlib readelf run size strings strip"

# Remove symlinks to the compiler
for i in $FILES; do
	# New install-script.sh installs links in /usr/bin
	# Using -h because the file must be a symlink
	if [ -h "/usr/bin/$PREFIX$i" ]; then
		sudo rm "/usr/bin/$PREFIX$i"
	fi
	# Old install-script.sh used to install links in /usr/local/bin,
	# so remove also those links for backward compatibility
	if [ -h "/usr/local/bin/$PREFIX$i" ]; then
		sudo rm "/usr/local/bin/$PREFIX$i"
	fi
done

# Remove lpc21isp
if [ -h "/usr/bin/lpc21isp" ]; then
	sudo rm "/usr/bin/lpc21isp"
fi
if [ -h "/usr/local/bin/lpc21isp" ]; then
	sudo rm "/usr/local/bin/lpc21isp"
fi

# Remove mx-postlinker
if [ -h "/usr/bin/mx-postlinker" ]; then
	sudo rm "/usr/bin/mx-postlinker"
fi
if [ -h "/usr/local/bin/mx-postlinker" ]; then
	sudo rm "/usr/local/bin/mx-postlinker"
fi

# Remove the compiler
cd /opt
sudo rm -rf arm-miosix-eabi/
