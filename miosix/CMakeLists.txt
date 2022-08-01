cmake_minimum_required(VERSION 3.16)

project(Miosix
        DESCRIPTION "OS kernel designed to run on 32bit microcontrollers"
        HOMEPAGE_URL "https://miosix.org/"
        LANGUAGES C CXX ASM)

set(MIOSIX_BOARD "" CACHE STRING "The board to be targeted")

set(MIOSIX_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(ARCH_FOLDER arch)
set(ARCH_ALL_COMMON_PATH ${MIOSIX_PATH}/${ARCH_FOLDER}/common)
set(CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config)

# List of all Miosix OS source files that have no special requirements
# and that must be built for each architecture (both .c and .cpp).
# These files will end up in libmiosix.a.

set(MIOSIX_SOURCES
        ${MIOSIX_PATH}/kernel/kernel.cpp
        ${MIOSIX_PATH}/kernel/sync.cpp
        ${MIOSIX_PATH}/kernel/error.cpp
        ${MIOSIX_PATH}/kernel/pthread.cpp
        ${MIOSIX_PATH}/kernel/stage_2_boot.cpp
        ${MIOSIX_PATH}/kernel/elf_program.cpp
        ${MIOSIX_PATH}/kernel/process.cpp
        ${MIOSIX_PATH}/kernel/process_pool.cpp
        ${MIOSIX_PATH}/kernel/timeconversion.cpp
        ${MIOSIX_PATH}/kernel/SystemMap.cpp
        ${MIOSIX_PATH}/kernel/scheduler/priority/priority_scheduler.cpp
        ${MIOSIX_PATH}/kernel/scheduler/control/control_scheduler.cpp
        ${MIOSIX_PATH}/kernel/scheduler/edf/edf_scheduler.cpp
        ${MIOSIX_PATH}/filesystem/file_access.cpp
        ${MIOSIX_PATH}/filesystem/file.cpp
        ${MIOSIX_PATH}/filesystem/stringpart.cpp
        ${MIOSIX_PATH}/filesystem/console/console_device.cpp
        ${MIOSIX_PATH}/filesystem/mountpointfs/mountpointfs.cpp
        ${MIOSIX_PATH}/filesystem/devfs/devfs.cpp
        ${MIOSIX_PATH}/filesystem/fat32/fat32.cpp
        ${MIOSIX_PATH}/filesystem/fat32/ff.cpp
        ${MIOSIX_PATH}/filesystem/fat32/diskio.cpp
        ${MIOSIX_PATH}/filesystem/fat32/wtoupper.cpp
        ${MIOSIX_PATH}/filesystem/fat32/ccsbcs.cpp
        ${MIOSIX_PATH}/stdlib_integration/libc_integration.cpp
        ${MIOSIX_PATH}/stdlib_integration/libstdcpp_integration.cpp
        ${MIOSIX_PATH}/e20/e20.cpp
        ${MIOSIX_PATH}/e20/unmember.cpp
        ${MIOSIX_PATH}/util/util.cpp
        ${MIOSIX_PATH}/util/unicode.cpp
        ${MIOSIX_PATH}/util/version.cpp
        ${MIOSIX_PATH}/util/crc16.cpp
        ${MIOSIX_PATH}/util/lcd44780.cpp)

# Declare an architecture into the Miosix build system.
# A folder with the same name of the architecture must exist within the folder
# declaring the architecture.
function(miosix_add_arch NAME)
    set(ARCH_NAME ${NAME})
    set(ARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH_NAME})

    # The file to be included to set the compilation options for this
    # architecture. It must be included after all the board-related options
    # have been defined.
    set(ARCH_OPTIONS ${ARCH_PATH}/options.cmake)

    # Base directory with header files for the architecture
    set(ARCH_INC ${ARCH_FOLDER}/${ARCH_NAME}/common)

    add_subdirectory(${NAME})
endfunction()

function(miosix_set_properties TARGET)
    # Set the compilation options
    target_compile_features(${TARGET} PUBLIC cxx_std_14)

    set(DFLAGS -MMD -MP)

    target_compile_options(${TARGET} PUBLIC
            ${DFLAGS}
            $<$<COMPILE_LANGUAGE:ASM>:${AFLAGS_BASE}>
            $<$<COMPILE_LANGUAGE:C>:${CFLAGS_BASE}>
            $<$<COMPILE_LANGUAGE:CXX>:${CXXFLAGS_BASE}>)

    # Set the link options
    #set_property(TARGET ${TARGET} PROPERTY INTERFACE_LINK_DEPENDS ${LINKER_SCRIPT})

    target_link_options(${TARGET} INTERFACE ${LFLAGS_BASE})
    target_link_libraries(${TARGET} INTERFACE stdc++ c m gcc atomic)

    # Add the 'DEBUG' and 'COMPILING_MIOSIX' definitions while compiling the sources of Miosix
    target_compile_definitions(${TARGET} PUBLIC $<$<CONFIG:Debug>:DEBUG> PRIVATE COMPILING_MIOSIX)

    # Set the include directories
    target_include_directories(${TARGET} PUBLIC
            ${MIOSIX_PATH}
            ${ARCH_ALL_COMMON_PATH}
            ${MIOSIX_PATH}/${ARCH_INC}
            ${MIOSIX_PATH}/${BOARD_INC}
            ${CONFIG_PATH}
            ${CONFIG_PATH}/${BOARD_INC})
endfunction()

# Declare a board into the Miosix build system.
# A folder with the same name of the board must exist within the folder
# declaring the board.
function(miosix_add_board NAME)
    if (${MIOSIX_BOARD} STREQUAL ${NAME})
        set(BOARD_NAME ${NAME})
        set(BOARD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${BOARD_NAME})

        # Base directory with header files for the board
        set(BOARD_INC ${ARCH_FOLDER}/${ARCH_NAME}/${BOARD_NAME})

        # The files to be included to set the compilation options for this
        # board. The `BOARD_OPTIONS` file must be included before the
        # `BOARD_CONFIG_OPTIONS` one, in order to allow the user to overwrite
        # options in case of need.

        set(BOARD_CONFIG_OPTIONS ${CONFIG_PATH}/${BOARD_INC}/options.cmake)
        set(BOARD_OPTIONS ${CMAKE_CURRENT_SOURCE_DIR}/${BOARD_NAME}/options.cmake)

        # Base compilation and link flags
        set(AFLAGS_BASE "")
        set(CFLAGS_BASE -D_MIOSIX_BOARDNAME=\"${BOARD_NAME}\" -ffunction-sections -Wno-unused-but-set-variable -Wall -g)
        set(CXXFLAGS_BASE -D_MIOSIX_BOARDNAME=\"${BOARD_NAME}\" -ffunction-sections -Wno-unused-but-set-variable -Wall -g)
        set(LFLAGS_BASE "")

        # Get the board options
        include(${BOARD_OPTIONS})
        include(${CONFIG_PATH}/options.cmake)
        include(${BOARD_CONFIG_OPTIONS})

        # Get the architecture options
        include(${ARCH_OPTIONS})

        # Create the target for the Miosix object files
        add_library(miosix-obj OBJECT
                ${MIOSIX_SOURCES}
                ${ARCH_SRC})

        miosix_set_properties(miosix-obj)

        # Create the target for the stage_1_boot library
        add_library(stage_1_boot OBJECT ${BOOT_FILE})
        target_sources(miosix-obj INTERFACE $<TARGET_OBJECTS:stage_1_boot>)
        miosix_set_properties(stage_1_boot)

        # Create the target for the Miosix library
        add_library(miosix-lib STATIC $<TARGET_OBJECTS:miosix-obj>)

        miosix_set_properties(miosix-lib)

        add_custom_command(TARGET miosix-lib PRE_LINK
                COMMAND perl ${MIOSIX_PATH}/_tools/kernel_global_objects.pl $<TARGET_OBJECTS:miosix-obj>
                VERBATIM
                COMMAND_EXPAND_LISTS)
    endif()
endfunction()

function(miosix_add_executable NAME)
    # Declare the target for the binary
    add_executable(${NAME} ${ARGN})

    # Link the Miosix library
    target_link_libraries(${NAME} PUBLIC miosix-lib)

    # Set the output folder
    set_target_properties(${NAME}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Output/${NAME}")

    # Set the name for generated ELF file
    set_target_properties(${NAME} PROPERTIES OUTPUT_NAME main.elf)

    # Generate the .hex and .bin files
    set(ELF_FILE ${CMAKE_BINARY_DIR}/Output/${NAME}/main.elf)
    set(HEX_FILE ${CMAKE_BINARY_DIR}/Output/${NAME}/main.hex)
    set(BIN_FILE ${CMAKE_BINARY_DIR}/Output/${NAME}/main.bin)

    add_custom_command(TARGET ${NAME} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O ihex ${ELF_FILE} ${HEX_FILE}
            BYPRODUCTS ${HEX_FILE}
            COMMENT "Building ${HEX_FILE}")

    add_custom_command(TARGET ${NAME} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_FILE} ${BIN_FILE}
            BYPRODUCTS ${BIN_FILE}
            COMMENT "Building ${BIN_FILE}")
endfunction()

# Discover the allowed configurations
add_subdirectory(${ARCH_FOLDER})

add_subdirectory(_tools/testsuite EXCLUDE_FROM_ALL)
