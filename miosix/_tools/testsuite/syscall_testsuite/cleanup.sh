#!/bin/bash

for i in *;
do
	if test -d $i; then
		rm "$i.h"
		cd $i && make clean && cd ..
	fi;
done;
