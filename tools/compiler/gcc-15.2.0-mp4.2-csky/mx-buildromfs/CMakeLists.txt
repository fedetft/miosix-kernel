
cmake_minimum_required(VERSION 3.16)
project(MX-BUILDROMFS)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)
add_definitions(-UNDEBUG)

include_directories(../shared) # For romfs_types.h and elf_types.h
add_executable(mx-buildromfs buildromfs.cpp)
