# Copyright (C) 2024 by Skyward
#
# This program is free software; you can redistribute it and/or
# it under the terms of the GNU General Public License as published
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# As a special exception, if other files instantiate templates or use
# macros or inline functions from this file, or you compile this file
# and link it with other works to produce a work based on this file,
# this file does not by itself cause the resulting work to be covered
# by the GNU General Public License. However the source code for this
# file must still be made available in accordance with the GNU
# Public License. This exception does not invalidate any other
# why a work based on this file might be covered by the GNU General
# Public License.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>

# Add the miosix/cmake path to find the Miosix.cmake platform file
# that defines the Miosix system name
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/..)

# Tell CMake that we are building for an embedded ARM system
set(CMAKE_SYSTEM_NAME Miosix)

set(MIOSIX_PREFIX      llvm)

# Names of the compiler and other tools
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_AR           ${MIOSIX_PREFIX}-ar)
set(CMAKE_RANLIB       ${MIOSIX_PREFIX}-ranlib)
set(CMAKE_OBJCOPY      ${MIOSIX_PREFIX}-objcopy)
set(CMAKE_OBJDUMP      ${MIOSIX_PREFIX}-objdump)
set(CMAKE_SIZE         ${MIOSIX_PREFIX}-size)

# Optimization flags for each language and build configuration
set(CMAKE_ASM_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_DEBUG "-g -gdwarf-4 -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-g -gdwarf-4 -O0")
set(CMAKE_ASM_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -gdwarf-4 -O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -gdwarf-4 -O2")
set(CMAKE_ASM_FLAGS_MINSIZEREL "")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")

# Setting the target for crosscompilation
# https://stackoverflow.com/questions/54539682/how-to-set-up-cmake-to-cross-compile-with-clang-for-arm-embedded-on-windows
set(CLANG_TARGET_TRIPLE arm-none-eabi)
set(CMAKE_ASM_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})
set(CMAKE_C_COMPILER_TARGET   ${CLANG_TARGET_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})

# Miosix llvm compiler path
find_program(MIOSIX_LLVM_PATH NAMES ${CMAKE_CXX_COMPILER})
string(REGEX REPLACE "/bin/[^/]+$" "" MIOSIX_LLVM_PATH ${MIOSIX_LLVM_PATH})
set(CMAKE_SYSROOT ${MIOSIX_LLVM_PATH})

# Miosix gcc compiler path
set(MIOSIX_GCC_PATH /opt/arm-miosix-eabi CACHE PATH "Path to the miosix gcc compiler")

# gcc multilib include paths (multilib link directories are set in miosix/CMakeLists.txt, after knowing the target board)
include_directories(
    ${MIOSIX_GCC_PATH}/arm-miosix-eabi/include/c++/9.2.0/arm-miosix-eabi
    ${MIOSIX_GCC_PATH}/arm-miosix-eabi/include/c++/9.2.0
    ${MIOSIX_GCC_PATH}/arm-miosix-eabi/include
)

# Set gcc linker for clang
add_link_options(-fuse-ld=${MIOSIX_GCC_PATH}/bin/arm-miosix-eabi-ld)

# We want to test for a static library and not an executable
# reference: https://stackoverflow.com/questions/53633705/cmake-the-c-compiler-is-not-able-to-compile-a-simple-test-program
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# Defines used by the Miosix patches to the libraries/opt/arm-miosix-eabi/arm-miosix-eabi/lib/thumb/cm4/hardfp/fpv4sp
add_compile_definitions($<$<COMPILE_LANGUAGE:C,CXX>:__GXX_TYPEINFO_EQUALITY_INLINE=0>)
add_compile_definitions($<$<COMPILE_LANGUAGE:C,CXX>:_MIOSIX>)
add_compile_definitions($<$<COMPILE_LANGUAGE:C,CXX>:_MIOSIX_GCC_PATCH_MAJOR=3>)
add_compile_definitions($<$<COMPILE_LANGUAGE:C,CXX>:_MIOSIX_GCC_PATCH_MINOR=1>)
