
To run the testsuite, run build.sh in the mpu_apps directory,
then copy mpu_testsuite.cpp from this directory into the top
level directory and modify the Makefile, from

SRC :=                                  \
main.cpp

to

SRC :=                                  \
mpu_testsuite.cpp

Run cleanup.sh from mpu_apps directory to clean compilation files.