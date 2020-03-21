#!/bin/bash

echo "Checking if a previous version of the Miosix Toolchain is installed"
echo "and uninstalling it ..."

./uninstall.sh

quit() {
	echo $1
	exit 1
}

echo "Installing the Miosix toolchain ..."
# NOTE: "" around pwd as the directory may contain spaces
sudo cp -R "`pwd`" /opt/arm-miosix-eabi			|| quit "Error: can't install to /opt"
sudo ln -s /opt/arm-miosix-eabi/bin/* /usr/bin	|| quit "Error: can't make symlinks to /usr/bin"
