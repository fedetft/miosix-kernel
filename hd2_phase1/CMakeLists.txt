# Phase-1 bring-up project for the Ailunce HD2 Miosix port.
# Builds a 2-thread LED-blink test to validate the context switch + os_timer.
cmake_minimum_required(VERSION 3.21)

set(MIOSIX_KPATH ${CMAKE_SOURCE_DIR}/../miosix)
# Patched C-SKY Miosix toolchain (csky-miosix-elf, /opt/csky-miosix-elf on llama-farm)
set(CMAKE_TOOLCHAIN_FILE ${MIOSIX_KPATH}/cmake/Toolchains/gcc-csky.cmake)
set(CMAKE_BUILD_TYPE "Release")

project(Hd2Phase1 C CXX ASM)

set(MIOSIX_BOARD hd2)
# board_settings.h + miosix_settings.h live in the in-tree config dir
set(MIOSIX_USER_CONFIG_PATH ${MIOSIX_KPATH}/config)

add_subdirectory(${MIOSIX_KPATH} miosix EXCLUDE_FROM_ALL)

add_executable(main main.cpp)
miosix_link_target(main)
