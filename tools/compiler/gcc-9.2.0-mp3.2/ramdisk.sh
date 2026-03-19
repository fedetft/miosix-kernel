#!/usr/bin/env bash

# Useful script to speed up compilation and/or save write cycles on an SSD by
# compiling in a ramdisk. Only viable if you have at least 16GByte of RAM.

case $(uname -s) in
	Linux)
		mkdir -p ramdisk
		sudo umount ramdisk 2> /dev/null # If not already mounted it's not an error
		sudo mount -t tmpfs -o size=7G,uid=`id -u`,gid=`id -g`,mode=700 tmpfs ramdisk
		sudo -k
		;;
	Darwin)
		ramdisk_size=$(( 7 * 1024 * 1024 ))
		ramdisk_name="miosix_ramdisk_$RANDOM"
		diskutil erasevolume HFS+ "$ramdisk_name" $(hdiutil attach -nobrowse -nomount ram://$(( ramdisk_size ))) > /dev/stderr
		ln -s "/Volumes/$ramdisk_name/" ./ramdisk
		;;
	*)
		echo "error: I don't know how to make a ramdisk on platform " $(uname -s) > /dev/stderr
		exit 1
		;;
esac

ln -s `pwd`/downloaded ramdisk/downloaded
cp -R installers ramdisk
cp -R mx-postlinker ramdisk
cp -R patches ramdisk
cp cleanup.sh ramdisk
cp install-script.sh ramdisk
cp uninstall.sh ramdisk
cp lpc21isp_148_src.zip ramdisk
