#!/usr/bin/env bash
#
#   Copyright (C) 2023-2024 by Daniele Cattaneo
#                                                                       
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or   
#   (at your option) any later version.                                 
#                                                                       
#   This program is distributed in the hope that it will be useful,     
#   but WITHOUT ANY WARRANTY; without even the implied warranty of      
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
#   GNU General Public License for more details.                        
#                                                                       
#   As a special exception, if other files instantiate templates or use 
#   macros or inline functions from this file, or you compile this file 
#   and link it with other works to produce a work based on this file,  
#   this file does not by itself cause the resulting work to be covered 
#   by the GNU General Public License. However the source code for this 
#   file must still be made available in accordance with the GNU General
#   Public License. This exception does not invalidate any other reasons
#   why a work based on this file might be covered by the GNU General   
#   Public License.                                                     
#                                                                       
#   You should have received a copy of the GNU General Public License   
#   along with this program; if not, see <http://www.gnu.org/licenses/> 
#

#set -x

SCRIPT_ROOT=$(cd -- $(dirname -- "$0") && pwd)
REPO_ROOT="$SCRIPT_ROOT"/../..
PROJECT_ROOT="$REPO_ROOT"/templates/simple
cd "$PROJECT_ROOT"
dir="$SCRIPT_ROOT"

usage()
{
  printf 'usage: %s [clean]\n' $0
}

clean()
{
  rm -rf "$dir"/*.build
}

if [[ $# -eq 1 && $1 == 'clean' ]]; then
  clean
  exit 0
fi
if [[ $# -ne 0 ]]; then
  usage $0
  exit 1
fi

if command -v nproc > /dev/null; then
  PARALLEL="-j$(nproc)"
elif [[ $(uname -s) == 'Darwin' ]]; then
  PARALLEL="-j$(sysctl -n hw.logicalcpu)"
else
  PARALLEL="-j1";
fi

boards=$(cd "$REPO_ROOT/miosix/arch/board"; ls)

clean
for board in $boards; do
  if [[ -e "$REPO_ROOT/miosix/arch/board/${board}/CMakeLists.txt" ]]; then
    rm -rf "$dir/$board.build"
    mkdir -p "$dir/$board.build"
    cmake -B "$dir/$board.build" -S . \
        -GUnix\ Makefiles \
        -DMIOSIX_OPT_BOARD=$board -DCMAKE_VERBOSE_MAKEFILE=TRUE \
            &> "$dir/$board.build/main.log"
    cmake --build "$dir/$board.build" $PARALLEL &>> "$dir/$board.build/main.log"
    if [[ $? == 0 ]]; then
      arm-miosix-eabi-objdump -d "$dir/$board.build/main.elf" > "$dir/$board.build/main.asm"
      arm-miosix-eabi-size "$dir/$board.build/main.elf" > "$dir/$board.build/main_size.txt"
      echo "OK     $board"
    else
      echo "failed $board"
    fi
  else
    echo "ign    $board"
  fi
done
echo '*****'
echo 'done!'
echo '*****'
