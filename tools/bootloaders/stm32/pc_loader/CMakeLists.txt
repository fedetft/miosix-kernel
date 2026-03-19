
cmake_minimum_required(VERSION 3.1)
project(PC_LOADER)

## Target
set(CMAKE_CXX_STANDARD 11)
set(LOADER_SRCS main.cpp serialstream.cpp)
add_executable(pc_loader ${LOADER_SRCS})

## Compiler flags
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS -O2)       ## Optimize
	set(CMAKE_EXE_LINKER_FLAGS -s) ## Strip binary
endif()

## Link libraries
set(BOOST_LIBS system)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
target_include_directories(pc_loader PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(pc_loader ${Boost_LIBRARIES})
find_package(Threads REQUIRED)
target_link_libraries(pc_loader ${CMAKE_THREAD_LIBS_INIT})
