#!/bin/bash

for i in *;
do
	if test -d $i; then
		cd $i && make clean && make "NAME=$i" && mv prog3.h "../$i.h" && cd ..
	fi;
done;
