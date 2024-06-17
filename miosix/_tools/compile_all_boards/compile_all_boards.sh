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
REPO_ROOT="$SCRIPT_ROOT"/../../..
cd "$REPO_ROOT"
dir="$SCRIPT_ROOT"

usage()
{
  printf 'usage: %s [clean]\n' $0
}

clean()
{
  rm -f "$dir"/*.bin "$dir"/*.elf "$dir"/*.asm "$dir"/*_size.txt "$dir"/*.log
}

if [[ $# -eq 1 && $1 == 'clean' ]]; then
  clean
  exit 0
fi
if [[ $# -ne 0 ]]; then
  usage $0
  exit 1
fi

boards=$(grep -E 'OPT_BOARD := ' "miosix/config/Makefile.inc" | sed -E 's/#?OPT_BOARD := //g')

clean
for board in $boards; do
  make clean OPT_BOARD=$board &> /dev/null
  make -j8 VERBOSE=1 OPT_BOARD=$board &> "$dir/$board.log"
  if [[ $? == 0 ]]; then
    mv main.bin "$dir/$board.bin"
    mv main.elf "$dir/$board.elf"
    arm-miosix-eabi-objdump -d "$dir/$board.elf" > "$dir/$board.asm"
    arm-miosix-eabi-size "$dir/$board.elf" > "$dir/${board}_size.txt"
    echo "OK     $board"
  else
    echo "failed $board"
  fi
  make clean OPT_BOARD=$board &> /dev/null
done
echo '*****'
echo 'done!'
echo '*****'
