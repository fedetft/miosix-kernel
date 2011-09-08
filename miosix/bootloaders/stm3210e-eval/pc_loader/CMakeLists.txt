
cmake_minimum_required(VERSION 2.6)
project(PC_LOADER)

## Target
set(LOADER_SRCS main.cpp serialstream.cpp)
add_executable(pc_loader ${LOADER_SRCS})

## Compiler flags
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS -O2)       ## Optimize
	set(CMAKE_EXE_LINKER_FLAGS -s) ## Strip binary
endif()

## Link libraries
set(BOOST_LIBS thread date_time system)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
target_link_libraries(pc_loader ${Boost_LIBRARIES})
find_package(Threads REQUIRED)
target_link_libraries(pc_loader ${CMAKE_THREAD_LIBS_INIT})
