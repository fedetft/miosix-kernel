#!/usr/bin/env bash

# Script to build the llvm compiler required for Miosix.
# Usage: ./install-script
#
# Building Miosix is officially supported only through the llvm compiler built
# with this script. This is because this script patches the compiler.
# The kernel *won't* compile unless the correct compiler is used.
#
# This script will install miosix-llvm in /opt, creating links to
# binaries in /usr/bin.
# It should be run without root privileges, but it will ask for the root
# password when installing files to /opt and /usr/bin

#### Configuration tunables -- begin ####

# Uncomment if installing globally on this system
PREFIX=/opt/miosix-llvm
SUDO=sudo
# Uncomment if installing locally on this system, sudo isn't necessary
#PREFIX=`pwd`/miosix-llvm
#SUDO=

# Choose llvm build type
BUILD_TYPE=Release
#BUILD_TYPE=Debug

#### Configuration tunables -- end ####

quit() {
	echo $1
	exit 1
}

# Ask for sudo if needed
if [[ ! -z "$SUDO" ]]; then
    echo "System-wide install selected: sudo rights are needed to continue"
    $SUDO -v || quit "Cannot continue without sudo rights"
fi

#
# Part 1, apply patches
#

PATCHES_DIR=`pwd`/patches
apply_patch() {
    local patch_file="$1"
    local patch_path="${PATCHES_DIR}/${patch_file}"

    if patch --dry-run -p1 < "$patch_path" > /dev/null 2>&1; then
        patch -p1 < "$patch_path" || quit "Failed to apply patch ${patch_file}"
    else
        echo "Patch ${patch_file} has already been applied: skipping"
    fi
}

echo "Applying patches"
cd llvm-project
apply_patch "Add-GCC-s-spare-dynamic-tags.patch"
apply_patch "Implemented-single-pic-base.patch"
apply_patch "libomp.patch"
echo "Successfully applied patches"

#
# Part 2: compile llvm
#

echo "Building llvm"
cd llvm
cmake -Bbuild -GNinja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DLLVM_ENABLE_PROJECTS="clang;lld" \
    -DLLVM_BUILD_LLVM_DYLIB=ON \
    -DLLVM_LINK_LLVM_DYLIB=ON \
    -DLLVM_OPTIMIZED_TABLEGEN=ON \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_PARALLEL_LINK_JOBS=1 || quit "Failed to configure llvm build"

cmake --build build || quit "Failed to build llvm"
$SUDO cmake --build build --target install || quit "Failed to install llvm"

# create links with miosix- prefix
if [[ ! -z "$SUDO" ]]; then
    symlink_path="$PREFIX/libexec/miosix-llvm"
    if [[ -e "$symlink_path" ]]; then
        $SUDO rm -rf "$symlink_path"
    fi
    $SUDO mkdir -p "$symlink_path"
    for src_path in "$PREFIX/bin/"*; do
        $SUDO ln -s "$src_path" "$symlink_path"/miosix-$(basename "${src_path}") \
            || quit "Failed to link binary ${src_path}"
    done
    # install symlinks in /usr/bin only on Linux
    if [[ $(uname -s) != 'Darwin' ]]; then
        $SUDO ln -s "$symlink_path"/* /usr/bin || quit "Failed install symlinks in /usr/bin"
    fi
fi

echo "Successfully built and installed llvm"
cd ..

# export the newly compiled compiler to use it to compile libraries even if not installed system-wide
export "PATH=${PREFIX}/bin:${PATH}"
# miosix clang toolchain file
TOOLCHAIN=`pwd`/../../../../cmake/Toolchains/clang.cmake

# TODO compile multilibs

#
# Part 3: compile libomp
#

# TODO crosscompile libomp also for other architectures
echo "Building libomp"
cd openmp
cmake -Bbuild -GNinja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DCMAKE_C_FLAGS="-mcpu=cortex-m0plus -mthumb" \
    -DCMAKE_CXX_FLAGS="-mcpu=cortex-m0plus -mthumb" \
    -DLIBOMP_ENABLE_SHARED=OFF || quit "Failed to configure libomp build"

cmake --build build || quit "Failed to build libomp"
$SUDO cmake --build build --target install || quit "Failed to install libomp"
echo "Successfully built and installed libomp"
cd ..

echo "Successfully installed miosix-llvm at ${PREFIX}"
