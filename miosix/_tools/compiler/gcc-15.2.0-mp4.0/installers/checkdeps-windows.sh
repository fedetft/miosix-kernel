#!/bin/bash

# When making a redistributable windows installation, use this
# to check the required librearies after it's installed on
# another machine

# Meant to be run from the main compiler directory (./installers/checkdeps-windows.sh)
strings gcc/arm-miosix-eabi/bin/*.exe | grep '\.dll' | sort -u
