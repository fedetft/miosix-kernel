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

# Utility function for printing a list
function(miosix_print_list MESSAGE_TYPE HEADER THE_LIST)
    cmake_parse_arguments(PARSE_ARGV 0 "PRINTLIST" "" "SELECTION;DEFAULT" "")
    set(STR_ITEMS "")
    foreach(LIST_ITEM ${THE_LIST})
        string(APPEND STR_ITEMS "\n   - ${LIST_ITEM}")
        if(LIST_ITEM STREQUAL PRINTLIST_SELECTION)
            string(APPEND STR_ITEMS " (selected)")
        endif()
        if(LIST_ITEM STREQUAL PRINTLIST_DEFAULT)
            string(APPEND STR_ITEMS " (default)")
        endif()
    endforeach()
    message(${MESSAGE_TYPE} "${HEADER}${STR_ITEMS}")
endfunction()

# Utility function for notifying the user that they have not set a variable
# correctly, if that variable needed to be set from a list
function(miosix_expected_item_in_list MESSAGE THE_LIST)
    set(STR_ITEMS "")
    foreach(LIST_ITEM ${THE_LIST})
        string(APPEND STR_ITEMS "\n   - ${LIST_ITEM}")
    endforeach()
    message(FATAL_ERROR
        " * ${MESSAGE}\n"
        " * Remember to delete cmake cache for changes in CMakeLists.txt to have effect (or use --fresh to ignore it)\n"
        " * Available options:${STR_ITEMS}")
endfunction()
