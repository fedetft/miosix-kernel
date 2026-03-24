#!/bin/bash

# When making a redistributable macOS package, use this
# to check the required libraries after it's installed on
# another machine

# Meant to be run from the main compiler directory (./installers/checkdeps.sh)

bad=0
for f in dist/opt/arm-miosix-eabi/bin/*; do
	printf 'Checking %s...\n' "$f"
	otool -L "$f" | tail -n +2 | grep -Ev '/usr/lib/[^/]*.dylib'
	if [[ $? == 0 ]]; then
		bad=$(( bad + 1 ))
	fi
done
if [[ $bad != 0 ]]; then
	printf 'There are %d files that link to non-system libraries!\n' "$bad"
else
	echo 'Everything seems to be OK'
fi
