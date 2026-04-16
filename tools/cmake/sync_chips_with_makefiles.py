#!/usr/bin/env python3
# Copyright (C) 2026 by Daniele Cattaneo
#
# This program is free software; you can redistribute it and/or
# it under the terms of the GNU General Public License as published
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# As a special exception, if other files instantiate templates or use
# macros or inline functions from this file, or you compile this file
# and link it with other works to produce a work based on this file,
# this file does not by itself cause the resulting work to be covered
# by the GNU General Public License. However the source code for this
# file must still be made available in accordance with the GNU
# Public License. This exception does not invalidate any other
# why a work based on this file might be covered by the GNU General
# Public License.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>

# This tool regenerates all miosix/arch/board/.../CMakeLists.txt files
# by converting the Makefile.inc files to equivalent CMake syntax and variable
# names.

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
