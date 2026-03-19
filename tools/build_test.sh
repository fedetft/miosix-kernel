#! /bin/bash

MIOSIX_KPATH=$(dirname $(dirname $(realpath $BASH_SOURCE)))

build_program() {
    rm -rf $1/build
    cmake -S $1 -B $1/build $2 --toolchain $MIOSIX_KPATH/cmake/Toolchains/gcc.cmake > /dev/null
    cmake --build $1/build -j 16 > /dev/null
}

# Try to build all available programs in _examples and _tools
echo "Building all available programs"
for file in $(find $MIOSIX_KPATH/_tools $MIOSIX_KPATH/_examples -name CMakeLists.txt); do
    # Check if the CMakeLists.txt is for a miosix program
    if grep -q "miosix_link_target" $file; then
        file=$(dirname $file)
        echo "  Building program in $file"
        build_program $file
    fi
done

# Try to compile the blinking_led test for every supported board
declare -a boards=(
    "stm32f407vg_stm32f4discovery stm32_1m+192k_rom.ld"
    "stm32f407vg_stm32f4discovery stm32_1m+192k_rom_processes.ld"
    "stm32f429zi_stm32f4discovery stm32_2m+256k_rom.ld"
    "stm32f429zi_stm32f4discovery stm32_2m+256k_ro_processes.ld"
    "stm32f429zi_stm32f4discovery stm32_2m+6m_xram.ld"
    "stm32f429zi_stm32f4discovery stm32_2m+8m_xram.ld"
    "stm32f767zi_nucleo stm32_2m+384k_ram.ld"
    "stm32f767zi_nucleo stm32_2m+384k_ram_processes.ld"
)
echo "Building testsuite for all available boards"
dir=$MIOSIX_KPATH/_tools/testsuite
for board in "${boards[@]}"; do
    read -a board <<< "$board"
    echo "  board=${board[0]} linker_script=${board[1]}"
    build_program $dir "-DMIOSIX_OPT_BOARD=${board[0]} -DMIOSIX_LINKER_SCRIPT=${board[1]}"
done
