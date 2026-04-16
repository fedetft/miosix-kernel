#!/usr/bin/env python3

from pathlib import Path
from miosix_makefile_parser import *

def chip_cmake_from_definitions(defs: dict, chip_name: str):
    cflags = defs["CHIP_CFLAGS"].value.replace(' $(XRAM)', '')
    cxxflags = defs["CHIP_CXXFLAGS"].value.replace(' $(XRAM)', '')
    if 'CPU_FLAGS' in defs:
        cpuflags = defs["CPU_FLAGS"].value
        multilib = find_multilib_dir(cpuflags)
    else:
        cpuflags = None
    src = fix_src_paths(defs["CHIP_SRC"])
    lines = [
        '##',
        f'## CMakeLists.txt for chip {chip_name}',
        '##',
        '',
        '# CPU microarchitecture',
        f'set(MIOSIX_CPU_INC ${{CMAKE_CURRENT_SOURCE_DIR}}/{defs["CPU_INC"].value})',
        ''
    ]
    if cpuflags is not None:
        lines += [
            '# Set the path to the multilib directory for this architecture (used by clang)',
            f'set(MIOSIX_MULTILIB_PATH {multilib})',
            ''
        ]
    lines += [
        '# Select appropriate compiler flags for both ASM/C/C++/linker'
    ]
    if cpuflags is not None:
        lines += [f'set(MIOSIX_CPU_FLAGS {cpuflags})']
    lines += [
        f'set(MIOSIX_CHIP_C_FLAGS {cflags})',
        f'set(MIOSIX_CHIP_CXX_FLAGS {cxxflags})',
        '',
        '# Select architecture specific files',
        f'set(MIOSIX_CHIP_SRC',
        src,
        ')',
        ''
    ]
    unused_defs = set(defs.keys()) - {
        "CHIP_CFLAGS", "CHIP_CXXFLAGS", 'CPU_FLAGS', "CHIP_SRC",
        "CHIP_SRC", "CPU_INC"
    }
    if len(unused_defs) > 0:
        warning(f'unused definitions {unused_defs} in {chip_name}')
    return '\n'.join(lines)

def main():
    arch_chip_path = Path('../../miosix/arch/chip')
    if not arch_chip_path.is_dir():
        error("The current directory must be the script's directory")
    all_chips_makefiles = list(arch_chip_path.glob("*/Makefile.inc"))
    for makefile in all_chips_makefiles:
        chip_name = makefile.parts[-2]
        defs = definitions_from_makefile(makefile)
        cmake = chip_cmake_from_definitions(defs, chip_name)
        cmake_path = makefile.parent / 'CMakeLists.txt'
        #if cmake_path.exists():
        #    warning(f'overwriting {cmake_path}')
        cmake_path.write_text(cmake)

if __name__ == '__main__':
    main()
