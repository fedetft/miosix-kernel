
cmake_minimum_required(VERSION 3.16)
project(MX-MAPUTIL)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)
add_definitions(-UNDEBUG)

add_executable(mx-maputil main.cpp maputils.cpp)
