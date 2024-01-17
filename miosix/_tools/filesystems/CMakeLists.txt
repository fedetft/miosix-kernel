
cmake_minimum_required(VERSION 3.1)
project(MIOSIX_FILESYSTEM_UTIL)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)

include_directories(../../filesystem/romfs)
add_executable(buildromfs buildromfs.cpp)
