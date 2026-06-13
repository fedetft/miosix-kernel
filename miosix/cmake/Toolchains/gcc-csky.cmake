# CK803S / HR_C7000 (Ailunce HD2) GCC toolchain file for Miosix.
# Copy of gcc.cmake retargeted from arm-miosix-eabi to the patched
# csky-miosix-elf toolchain (project_hd2_csky_toolchain / project_hd2_miosix_port,
# installed at /opt/csky-miosix-elf on llama-farm). The CPU flags (-mcpu=ck803
# -EL) come from the chip CMakeLists (MIOSIX_CPU_FLAGS), not here.
#
# Select with: cmake -DCMAKE_TOOLCHAIN_FILE=miosix/cmake/Toolchains/gcc-csky.cmake
#
# Original copyright (C) 2024 by Skyward, GPL v2+ with the linking exception.

# Add the miosix/cmake path to find the Miosix.cmake platform file
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/..)

# Tell CMake that we are building for an embedded Miosix system
set(CMAKE_SYSTEM_NAME Miosix)

# Select compiler — the patched C-SKY Miosix toolchain
set(MIOSIX_PREFIX      csky-miosix-elf)

# From compiler prefix form the name of the compiler and other tools
set(CMAKE_ASM_COMPILER ${MIOSIX_PREFIX}-gcc) #Compiling asm with GCC to allow #ifdef
set(CMAKE_C_COMPILER   ${MIOSIX_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${MIOSIX_PREFIX}-g++)
set(CMAKE_AR           ${MIOSIX_PREFIX}-ar)
set(CMAKE_RANLIB       ${MIOSIX_PREFIX}-ranlib)
set(CMAKE_OBJCOPY      ${MIOSIX_PREFIX}-objcopy)
set(CMAKE_OBJDUMP      ${MIOSIX_PREFIX}-objdump)
set(CMAKE_SIZE         ${MIOSIX_PREFIX}-size)
set(MIOSIX_READELF     ${MIOSIX_PREFIX}-readelf)

# Optimization flags for each language and build configuration
set(CMAKE_ASM_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_ASM_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2")
set(CMAKE_ASM_FLAGS_MINSIZEREL "")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")

# NOTE: the upstream gcc.cmake runs tools/compiler_check.pl to gate the compiler
# version. It may not recognise the csky-miosix-elf banner; if it rejects the
# build, the check is bypassable (the toolchain is gcc 15.2.0, well above the
# minimum). Left in for now — revisit at first build.
execute_process(COMMAND
    perl ${CMAKE_CURRENT_LIST_DIR}/../../tools/compiler_check.pl
    ${CMAKE_C_COMPILER}
    OUTPUT_VARIABLE MIOSIX_COMPILER_IS_COMPATIBLE
)
if(NOT MIOSIX_COMPILER_IS_COMPATIBLE EQUAL 0)
    message(FATAL_ERROR
        "You are using a too old or unsupported compiler. "
        "Get the latest one from "
        "https://miosix.org/wiki/index.php?title=Miosix_Toolchain"
    )
endif()
