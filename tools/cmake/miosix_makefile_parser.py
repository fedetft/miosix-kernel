#!/usr/bin/env python3

from pathlib import Path
import sys
import re
import subprocess

class Definition:
    def __init__(self, variable, type, value, comment, disabled, next=None):
        self.variable = variable
        self.type = type
        self.value = value
        self.comment = comment
        self.disabled = disabled
        self.next = next

    def __repr__(self):
        return repr({
            'variable': self.variable,
            'type': self.type,
            'value': self.value,
            'comment': self.comment,
            'disabled': self.disabled})

def error(str):
    print("error:", str, file=sys.stderr)
    sys.exit(1)

def warning(str):
    print("warning:", str, file=sys.stderr)

def definitions_from_makefile(path: Path):
    lines = path.read_text().splitlines()
    # Eliminate everything up to the first empty line
    # to eliminate header comments
    i = 0
    while i < len(lines) and lines[i].strip() != '':
        i += 1
    lines = lines[i:]
    definitions = {}
    last_definition = None
    # Find all lines with a plausible assignment in it
    # which might be commented
    for i, line in enumerate(lines):
        match = re.match('(# ?)?([A-Z_]+) +([:?]?=)(.*)', lines[i])
        if not match:
            continue
        disabled = True if match[1] else False
        variable = match[2]
        type = match[3]
        value = match[4].strip()
        # join multi-line values
        j = i+1
        while j < len(lines) and value != '' and value[-1] == '\\':
            value = value[:-1].strip() + ' ' + lines[j].strip()
            j += 1
        # read preceding comments
        comments = ''
        j = i-1
        while j >= 0 and lines[j] != '' and lines[j][0] == '#':
            comments = lines[j] + '\n' + comments
            j -= 1
        definition = Definition(variable, type, value, comments, disabled)
        if variable in definitions:
            old = definitions[variable]
            if isinstance(old, Definition):
                old = [old]
            definitions[variable] = old + [definition]
        else:
            definitions[variable] = definition
        if last_definition is not None:
            last_definition.next = definition
        last_definition = definition
    return definitions

def find_multilib_dir(options):
    if 'arm7tdmi' in options:
        # TODO: why does this not work for arm7tdmi?
        # probably a bug in -print-search-dirs specifically because when compiling it works fine
        return 'arm/v4t/nofp'
    out = subprocess.check_output(['arm-miosix-eabi-gcc'] + options.split(' ') + ['-print-search-dirs']).decode('utf-8')
    lines = out.splitlines()
    libraries = lines[2]
    paths = libraries.split(':')
    dir = paths[2]
    idx = dir.rfind('15.2.0/')
    return dir[idx+7:-1]

def fix_src_paths(paths):
    paths = paths.strip().split(' ')
    def fix_src_path(path):
        path = path.replace('$(CHIP_INC)', '${MIOSIX_CHIP_INC}')
        path = path.replace('$(BOARD_INC)', '${MIOSIX_BOARD_INC}')
        path = path.replace('$(CPU_INC)', '${MIOSIX_CPU_INC}')
        if path[0] != '$':
            path = '${CMAKE_CURRENT_SOURCE_DIR}/' + path
        return '    ' + path.strip()
    return '\n'.join([fix_src_path(path) for path in paths])
