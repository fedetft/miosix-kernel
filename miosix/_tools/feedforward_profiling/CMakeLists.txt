
cmake_minimum_required(VERSION 2.6)
project(JTAG_PROFILER)

## Targets
set(CMAKE_BUILD_TYPE Release)
set(SRCS jtag_profiler.cpp)
add_executable(jtag_profiler ${SRCS})

## Link libraries
set(BOOST_LIBS regex system)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
target_link_libraries(jtag_profiler ${Boost_LIBRARIES})
#find_package(Threads REQUIRED)
#target_link_libraries(jtag_profiler ${CMAKE_THREAD_LIBS_INIT})
