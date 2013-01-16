#!/bin/bash


PREFIX="$1_"

for i in $1_*/; do
    if [ -d "$i" ]; then
		echo "Building $i"
		
		len=${#i}
		name=${i:0:len - 1}
		
		cd $i &&
		make clean &&
		make $2 "NAME=$name" &&
		cp prog3.h "../$name.h" &&
		cd ..
	fi
done