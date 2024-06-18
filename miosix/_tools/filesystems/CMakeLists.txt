
cmake_minimum_required(VERSION 3.5)
project(MIOSIX_FILESYSTEM_UTIL)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)

add_definitions(-UNDEBUG)

include_directories(../../filesystem/romfs) # For romfs_types.h
include_directories(../../kernel)           # For elf_types.h
add_executable(buildromfs buildromfs.cpp)

# put binary in the same directory of the source code
set_target_properties(buildromfs PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
