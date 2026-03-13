#!/bin/bash

quit() {
	echo $1
	exit 1
}

which sudo > /dev/null || quit "Error: sudo is missing, please install it and try again"

echo "Checking if a previous version of the Miosix Toolchain is installed"
echo "and uninstalling it ..."

./uninstall.sh

echo "Installing the Miosix toolchain ..."
# NOTE: some distros may not have /opt anymore
[[ -d /opt ]] || sudo mkdir /opt
# NOTE: "" around pwd as the directory may contain spaces
sudo cp -R "`pwd`" /opt/arm-miosix-eabi			|| quit "Error: can't install to /opt/arm-miosix-eabi"
sudo ln -s /opt/arm-miosix-eabi/bin/* /usr/bin	|| quit "Error: can't make symlinks to /usr/bin"
