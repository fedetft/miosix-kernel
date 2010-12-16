This directory contains an attempt at crosscompiling an lpc2000
project with CMake.
This test pointed out some problems:

- Support for the GNU assembler is not yet implemented.

- It is necessary to specify that we are crosscompiling and set the
  compiler name early in the build process by passing the
  -DCMAKE_TOOLCHAIN_FILE= option when running CMake.
  However this means it is not possible to choose the compiler as an
  option when running ccmake.

For now the solution is to remain with makefiles and configuration
files