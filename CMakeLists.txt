##
## Makefile for Miosix embedded OS
##
set(PROJECT_NAME ProjectName)

## List here additional static libraries with relative path
set(LIBS "")

## List here additional include directories (in the form -Iinclude_dir)
set(INCLUDE_DIRS "")



##############################################################################
## You should not need to modify anything below                             ##
##############################################################################

project(${PROJECT_NAME} C CXX ASM)
cmake_minimum_required(VERSION 3.7)

#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_SYSTEM_NAME Generic)
#set(CMAKE_SYSTEM_VERSION 1)

add_subdirectory(miosix)

miosix_add_executable(Test main.cpp)
