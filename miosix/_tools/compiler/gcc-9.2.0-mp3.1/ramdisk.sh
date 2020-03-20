#!/bin/bash

# Useful script to speed up compilation and/or save write cycles on an SSD by
# compiling in a ramdisk. Only viable if you have at least 16GByte of RAM.

mkdir -p ramdisk
sudo umount ramdisk 2> /dev/null # If not already mounted it's not an error
sudo mount -t tmpfs -o size=7G,uid=`id -u`,gid=`id -g`,mode=700 tmpfs ramdisk
sudo -k

cp -R downloaded ramdisk
cp -R installers ramdisk
cp -R mx-postlinker ramdisk
cp -R patches ramdisk
cp cleanup.sh ramdisk
cp install-script.sh ramdisk
cp uninstall.sh ramdisk
cp lpc21isp_148_src.zip ramdisk
