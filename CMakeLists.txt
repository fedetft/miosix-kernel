##
## Makefile for Miosix embedded OS
##
set(PROJECT_NAME ProjectName)

## Path to toolchain directory
set(TOOLCHAIN_PATH "/opt/arm-miosix-eabi")

## Path to kernel directory
set(KPATH miosix)

## List here additional static libraries with relative path
set(LIBS "")

## List here additional include directories (in the form -Iinclude_dir)
set(INCLUDE_DIRS "")

##
## List here your source files (both .s, .c and .cpp)
##
file(GLOB SOURCES main.cpp)



##############################################################################
## You should not need to modify anything below                             ##
##############################################################################

project(${PROJECT_NAME} C CXX ASM)
cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(KPATH_REL ${KPATH})

file(GLOB KPATH ${KPATH})

if (KPATH STREQUAL "")
    message(FATAL_ERROR "Kernel not found")
endif()

file(GLOB CONFPATH ${KPATH}/config)

if (CONFPATH EQUAL "")
    message(FATAL_ERROR "Kernel configuration folder not found")
endif()

if ("${SOURCES}" STREQUAL "")
    message(FATAL_ERROR "No sources specified")
endif()

add_subdirectory(${KPATH})
include(${CONFPATH}/CMakeLists.txt)

include_directories(
        ${TOOLCHAIN_PATH}/arm-miosix-eabi/include
        ${TOOLCHAIN_PATH}/lib/gcc/arm-miosix-eabi/4.7.3/include
        ${TOOLCHAIN_PATH}/lib/gcc/arm-miosix-eabi/4.7.3/include-fixed
        ${TOOLCHAIN_PATH}/arm-miosix-eabi/include/c++/4.7.3/
        ${TOOLCHAIN_PATH}/arm-miosix-eabi/include/c++/4.7.3/arm-miosix-eabi
        ${TOOLCHAIN_PATH}/arm-miosix-eabi/include/c++/4.7.3/backward
        ${CONFPATH}
        ${CONFPATH}/${BOARD_INC}
        ${KPATH}
        ${KPATH}/arch/common
        ${KPATH}/${ARCH_INC}
        ${KPATH}/${BOARD_INC}
        ${INCLUDE_DIRS}
)

set(DFLAGS "-MMD -MP")
set(LFLAGS ${LFLAGS_BASE})

set(LINK_LIBS "${LIBS} -L${CMAKE_CURRENT_BINARY_DIR}/${KPATH_REL} -Wl,--start-group -lmiosix -lstdc++ -lc -lm -lgcc -Wl,--end-group")

set(CMAKE_C_FLAGS "${DFLAGS} ${CFLAGS_BASE}")
set(CMAKE_CXX_FLAGS "${DFLAGS} ${CXXFLAGS_BASE}")
set(CMAKE_ASM_FLAGS ${AFLAGS_BASE})

set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_COMPILER} ${LFLAGS} -o ${PROJECT_SOURCE_DIR}/main.elf <OBJECTS> ${LINK_LIBS}")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_COMPILER} ${LFLAGS} -o ${PROJECT_SOURCE_DIR}/main.elf <OBJECTS> ${LINK_LIBS}")

add_executable(main.bin ${SOURCES} ${LINKER_SCRIPT})

set(HEX_FILE ${PROJECT_SOURCE_DIR}/main.hex)
set(BIN_FILE ${PROJECT_SOURCE_DIR}/main.bin)

add_custom_command(TARGET main.bin POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_SOURCE_DIR}/main.elf ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_SOURCE_DIR}/main.elf ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}"
        COMMENT "Building ${BIN_FILE}")

add_dependencies(main.bin miosix)
