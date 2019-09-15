#!/bin/bash

echo > includes.h
echo "#ifndef _APP_MPU_TESTS_" >> includes.h
echo "#define _APP_MPU_TESTS_" >> includes.h

for i in *;
do
	if test -d $i; then
		cd $i && make clean && make && mv mpuTest.h "../$i.h" && cd ..
		echo "#include \"$i.h\"" >> includes.h
		sed "s/test_elf/$i\_elf/" $i.h -i
	fi;
done;

echo "#endif //_APP_MPU_TESTS_" >> includes.h
