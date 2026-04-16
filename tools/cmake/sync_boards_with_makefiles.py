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

def parse_linker_script_options(defs: dict, board_name: str):
    ldscripts = defs['LINKER_SCRIPT']
    if isinstance(ldscripts, Definition):
        ldscripts = [ldscripts]
    ldscripts_options = []
    default_ldscript = None
    last_comment = None
    for ldscript in ldscripts:
        if 'already selected' in ldscript.value:
            break
        if not ldscript.disabled:
            if default_ldscript is not None:
                warning(f'multiple default linker scripts for board_name')
            default_ldscript = ldscript.value
        options = []
        if 'process' in ldscript.value:
            options += ['-DWITH_PROCESSES']
        maybe_xram = ldscript.next
        if maybe_xram and maybe_xram.variable == 'XRAM' \
            and maybe_xram.disabled == ldscript.disabled \
            and 'not uncomment' not in maybe_xram.comment:
            options += maybe_xram.value.split(' ')
        comment = None
        is_bad_comment = ldscript.comment == '## Select linker script\n'
        if last_comment:
            is_bad_comment = is_bad_comment or ldscript.comment.find(last_comment) == 0
        if not is_bad_comment:
            comment = ldscript.comment
            last_comment = comment
        if comment:
            comment = comment.replace('## Select linker script\n', '')
            comment = comment.replace('## Select linker script: ', '# ')
            comment = comment.replace('## Select linker script, ', '# ')
            comment = comment.replace('## Linker script type, there are two options\n', '')
        ldscripts_options += [(ldscript.value, ' '.join(options), comment)]
    if default_ldscript is None:
        error(f'no default linker script for {board_name}')
    return ldscripts_options, default_ldscript

def parse_board_variant_options(defs: dict, board_name: str):
    if 'BOARD_VARIANT' not in defs:
        return None, None, None
    variants = defs['BOARD_VARIANT']
    if isinstance(variants, Definition):
        variants = [variants]
    comment = variants[0].comment
    options = [v.value for v in variants]
    defaults = [v.value for v in variants if v.disabled == False]
    if len(defaults) == 0:
        error(f'no default board variant for {board_name}')
    return comment, options, defaults[-1]

def board_cmake_from_definitions(defs: dict, board_name: str):
    lines = [
        '##',
        f'## CMakeLists.txt for board {board_name}',
        '##',
        '',
        '# Directory with header files for this board',
        f'set(MIOSIX_CHIP_INC ${{CMAKE_CURRENT_SOURCE_DIR}}/{defs["CHIP_INC"].value})',
        ''
    ]

    if 'CPU_FLAGS' in defs:
        multilibs = find_multilib_dir(defs['CPU_FLAGS'].value)
        lines += [
            '# Set the path to the multilib directory for this architecture (used by clang)',
            f'set(MIOSIX_MULTILIB_PATH {multilibs})',
            ''
        ]
        if defs['CPU_FLAGS'].comment is not None:
            comment = defs['CPU_FLAGS'].comment.strip().replace('##', '#')
        else:
            comment = '# Select appropriate compiler flags for both ASM/C/C++/linker'
        lines += [
            comment,
            f'set(MIOSIX_CPU_FLAGS {defs["CPU_FLAGS"].value})',
            ''
        ]

    ldscripts_options, default_ldscript = parse_linker_script_options(defs, board_name)
    ldscript_align = max([len(name) for name, _, _ in ldscripts_options]) + 1
    ldscript_align += (4 - ldscript_align % 4) % 4
    lines += [
        '# Linker script options',
        '# Each script in the list may be followed by the compiler options it requires',
        '# These options are automatically added to the C/CXX compiler command line when',
        '# that linker script is selected',
        'set(MIOSIX_LINKER_SCRIPT_LIST',
    ]
    for name, options, comments in ldscripts_options:
        if comments is not None:
            comment_lines = comments.strip().splitlines()
            comment_lines = ['    '+line.replace('##', '#') for line in comment_lines]
            lines += comment_lines
        lines += ['    ' + (name + ' '*(ldscript_align-len(name))  + options).strip()]
    lines += [
        ')',
        f'set(MIOSIX_DEFAULT_LINKER_SCRIPT {default_ldscript})',
        ''
    ]

    variant_comment, variant_opts, variant_default = parse_board_variant_options(defs, board_name)
    if variant_comment:
        lines += [variant_comment.replace('##', '#').strip()]
    if variant_opts:
        lines += ['set(MIOSIX_BOARD_VARIANT_LIST']
        lines += ['    ' + v for v in variant_opts]
        lines += [
            ')',
            f'set(MIOSIX_DEFAULT_BOARD_VARIANT {variant_default})',
            ''
        ]

    src = fix_src_paths(defs["BOARD_SRC"].value)
    lines += [
        '# Select architecture specific files',
        f'set(MIOSIX_BOARD_SRC',
        src,
        ')',
        ''
    ]

    cflags = defs["BOARD_CFLAGS"].value
    cflags = cflags.replace(' -D$(BOARD_VARIANT)','')
    cxxflags = defs["BOARD_CFLAGS"].value
    cxxflags = cxxflags.replace(' -D$(BOARD_VARIANT)', '')
    lines += ['# Add a #define to allow querying board name']
    # check for mandatory __enable_xram
    if 'XRAM' in defs:
        xrams = defs['XRAM']
        if isinstance(xrams, Definition):
            xrams = [xrams]
        xrams = [xram for xram in xrams if xram.disabled == False]
        if len(xrams) > 0 and 'not uncomment' in xrams[-1].comment:
            comment = xrams[-1].comment.strip().replace('##', '#')
            lines += [comment]
            cflags += ' ' + xrams[-1].value.strip()
            cxxflags += ' ' + xrams[-1].value.strip()
    lines += [
        f'set(MIOSIX_BOARD_C_FLAGS {cflags})',
        f'set(MIOSIX_BOARD_CXX_FLAGS {cxxflags})',
        ''
    ]

    lines += [
        '# Specify a custom flash command',
        '# This is the program that is invoked when the program-<target_name> target is',
        '# built. Use <binary> or <hex> as placeolders, they will be replaced by the',
        '# build systems with the binary or hex file path repectively.',
        '# If a command is not specified, the build system will fallback to st-flash'
    ]
    prog_cmd = '#set(PROGRAM_CMDLINE ...)'
    if 'PROG' not in defs:
        warning(f'board {board_name} does not have PROG definitions')
    else:
        prog = defs['PROG']
        if not isinstance(prog, Definition):
            warning(f'board {board_name} has multiple PROG definitions, '
                    'this is unsupported by the CMake synchronization tool')
        else:
            prog_cmd = prog.value.replace('$(if $(ROMFS_DIR), image.bin, main.bin)', '<binary>')
            prog_cmd = prog_cmd.replace('$(PROG_BIN)', '<binary>')
            prog_cmd = prog_cmd.replace('$(KPATH)/$(BOARD_INC)', '${MIOSIX_BOARD_INC}')
            prog_cmd = prog_cmd.replace('$(KPATH)', '${CMAKE_CURRENT_SOURCE_DIR}')
            prog_cmd = prog_cmd.replace('$(BOARD_INC)', '${MIOSIX_BOARD_INC}')
            if 'st-flash' not in prog.value:
                prog_cmd = 'set(PROGRAM_CMDLINE ' + prog_cmd + ')'
            else:
                prog_cmd = '#set(PROGRAM_CMDLINE ' + prog_cmd + ')'
    lines += [
        prog_cmd,
        ''
    ]

    unused_defs = set(defs.keys()) - {
        "CHIP_INC", 'LINKER_SCRIPT', 'XRAM', 'BOARD_VARIANT', 'BOARD_SRC',
        'BOARD_CFLAGS', 'BOARD_CXXFLAGS', 'PROG', 'CPU_FLAGS'
    }
    if len(unused_defs) > 0:
        warning(f'unused definitions {unused_defs} in {board_name}')
    return '\n'.join(lines)

def main():
    arch_board_path = Path('../../miosix/arch/board')
    if not arch_board_path.is_dir():
        error("The current directory must be the script's directory")
    all_board_makefiles = list(arch_board_path.glob("*/Makefile.inc"))
    #all_board_makefiles = list(arch_board_path.glob("rp2040*/Makefile.inc"))
    #all_board_makefiles = list(arch_board_path.glob("stm32f429zi_skyward_anakin/Makefile.inc"))
    print(len(all_board_makefiles))
    for makefile in all_board_makefiles:
        print(makefile)
        defs = definitions_from_makefile(makefile)
        board_name = makefile.parts[-2]
        config_makefile = Path('../../miosix/config/board') / board_name / 'Makefile.inc'
        if config_makefile.is_file():
            defs |= definitions_from_makefile(config_makefile)
        cmake = board_cmake_from_definitions(defs, board_name)
        cmake_path = makefile.parent / 'CMakeLists.txt'
        #if cmake_path.exists():
        #    warning(f'overwriting {cmake_path}')
        cmake_path.write_text(cmake)

if __name__ == '__main__':
    main()
