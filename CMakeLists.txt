set(PROJECT_NAME ProjectName)

project(${PROJECT_NAME} C CXX ASM)
cmake_minimum_required(VERSION 3.7)

add_subdirectory(miosix)

miosix_add_executable(Test main.cpp)
