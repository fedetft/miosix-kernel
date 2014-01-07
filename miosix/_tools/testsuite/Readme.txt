
To run the testsuite:
1) Run "build.sh" in the mpu_testsuite directory,
2) Run "build.sh testsuite" in the syscall_testsuite directory,
3) Copy testsuite.cpp from this directory into the top
   level directory and modify the Makefile, from

SRC :=                                  \
main.cpp

to

SRC :=                                  \
testsuite.cpp

Run "cleanup.sh" from mpu_testsuite directory to clean compilation files.

For more information, see the Readme.txt files in the subdirectories.