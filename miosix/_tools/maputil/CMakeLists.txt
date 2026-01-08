
cmake_minimum_required(VERSION 3.25)
project(MAPUTILS)

add_library(maputils STATIC maputils.cpp)
add_executable(maputil main.cpp)
target_link_libraries(maputil maputils)
