##
## CMakeLists.txt for chip hr_c7000 (CK803S core)
##

# CPU microarchitecture
set(MIOSIX_CPU_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/cskyv2_ck803s)

# Multilib directory (used by clang; GCC selects via -mcpu). C-SKY ck803, LE,
# no FPU. TODO: confirm the patched csky-miosix-elf multilib layout at build.
set(MIOSIX_MULTILIB_PATH csky/ck803/nofp)

# Compiler flags for ASM/C/C++/linker. ck803 = 16-GPR C-SKY V2 base, no FPU,
# little-endian (-EL); built for the patched csky-miosix-elf toolchain.
# NOTE: this csky-miosix-elf GCC was configured WITHOUT init_array support
# (-fuse-init-array is unrecognized), so C++ global ctors land in the legacy
# .ctors section. kernel_global_objects.pl is adapted to rename kernel-object
# .ctors -> .miosix_init_array so kernel global ctors still run before the
# kernel starts (the EABI .init_array path ARM uses by default).
set(MIOSIX_CPU_FLAGS -mcpu=ck803 -EL)
set(MIOSIX_CHIP_C_FLAGS -D_CHIP_HR_C7000)
set(MIOSIX_CHIP_CXX_FLAGS -D_CHIP_HR_C7000)

# All architecture-specific sources (cpu + chip), like the lpc2000 chip lists
# its cpu (armv4) files. Paths are relative to the miosix source root.
set(MIOSIX_CHIP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/cskyv2_ck803s/cskyv2_context.S
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/cskyv2_ck803s/interfaces-impl/cpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/cskyv2_ck803s/interrupts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/chip/hr_c7000/hr_c7000_os_timer.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/poweroff.cpp
)
