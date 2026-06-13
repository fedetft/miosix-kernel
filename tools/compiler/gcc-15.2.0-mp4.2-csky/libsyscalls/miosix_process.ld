/***************************************************************************
 *   Copyright (C) 2012-2020 by Terraneo Federico                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
    /* Here starts the first elf segment, that stays in flash */
    . = 0 + SIZEOF_HEADERS;

    .text : ALIGN(8)
    {
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
    }
    
    .rodata : ALIGN(8)
    {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
    }

    .ARM.extab : ALIGN(8)
    {
        *(.gcc_except_table)
        *(.gcc_except_table.*)
        *(.ARM.extab*)
        *(.gnu.linkonce.armextab.*)
    }
    __exidx_start = .;
    /* NOTE: just calling it .ARM.exidx breaks the program header */
    .ARM.exidx.mx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    }
    __exidx_end = .;

    .rel.data : { *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*) }
    .rel.got  : { *(.rel.got) }

    /* Here starts the second segment, that is copied in RAM and relocated */
    . = 0x40000000; /* DATA_BASE */
    _data = .; /* used by _Unwind_GetDataRelBase to support C++ exceptions */

    .got      : { *(.got.plt) *(.igot.plt) *(.got) *(.igot) }

    .dynamic  : { *(.dynamic) }

    /* NOTE: just calling it .init_array adds two useless entries to dynamic */
    .init_array.mx : ALIGN(8)
    {
        KEEP(*(.init))

        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;

        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
    }

    .fini_array.mx : ALIGN(8)
    {
        KEEP(*(.fini))

        __fini_array_start = .;
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        __fini_array_end = .;
    }

    .data : ALIGN(8)
    {
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
    }

    .bss : ALIGN(8)
    {
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
    }
    _end = .; /* used by _sbrk_r */

    /* These are removed since are unused and increase binary size */
    /DISCARD/ :
    {
        *(.interp)
        *(.dynsym)
        *(.dynstr)
        *(.hash)
        *(.comment)
        *(.ARM.attributes)
    }
}
