#!/bin/bash

# When making a redistributable linux installation, use this
# to check the required librearies after it's installed on
# another machine

# Meant to be run from the main compiler directory (./installers/checkdeps.sh)

ldd gcc/arm-miosix-eabi/bin/* | perl -ne 'next unless(/\s+(\S+.so(\S+))\s+/);print "$1\n";' | sort -u
