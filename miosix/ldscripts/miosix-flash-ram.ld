/***************************************************************************
 *   Copyright (C) 2026 by Terraneo Federico                               *
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

/*
 * This linker script supports the common case where the chip provides two
 * memory regions:
 * - One contiguous block of nonvolatile memory that is read-only during code
 *   execution, most commonly a FLASH memory, called "flash" in the memory map
 * - One contiguous block of volatile read/write memory, most commonly SRAM
 *   that is called "ram" in the memory map
 *
 * In summary, the assignment of sections to memory regions is as follows:
 *   flash  .text
 *   ram    .irq_stack
 *          .data
 *          .bss
 *          .heap
 *          .process_pool
 */

/* Default values for tunable variables */
PROVIDE(_irq_stack_size = 512);
PROVIDE(_process_pool_size = 0);
PROVIDE(_xram_start = 0); /* Used for memory protection and cacheability */
PROVIDE(_xram_size = 0);  /* Used for memory protection and cacheability */

/* Sanity checks */
ASSERT(_irq_stack_size % 8 == 0, "IRQ stack size error");
ASSERT(_process_pool_size % 1024 == 0, "Process pool size error");

ENTRY(_ZN6miosix13Reset_HandlerEv)

SECTIONS
{
    . = 0;
    
    .text :
    {
        /* Interrupt vectors must be placed at the start of flash */
        KEEP(*(.isr_vector))
        
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
        /* these sections for thumb interwork? */
        *(.glue_7)
        *(.glue_7t)
        /* these sections for C++ */
        *(.gcc_except_table)
        *(.gcc_except_table.*)
        *(.ARM.extab*)
        *(.gnu.linkonce.armextab.*)

        . = ALIGN(4);
        /* .rodata: constant data */
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)

        /* C++ Static constructors/destructors (eabi) */
        . = ALIGN(4);
        KEEP(*(.init))
        
        . = ALIGN(4);
        __miosix_init_array_start = .;
        KEEP (*(SORT(.miosix_init_array.*)))
        KEEP (*(.miosix_init_array))
        __miosix_init_array_end = .;

        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;

        . = ALIGN(4);
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;

        . = ALIGN(4);
        KEEP(*(.fini))

        . = ALIGN(4);
        __fini_array_start = .;
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        __fini_array_end = .;

        /* C++ Static constructors/destructors (elf)  */
        . = ALIGN(4);
        _ctor_start = .;
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        _ctor_end = .;

        . = ALIGN(4);
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
    } > flash

    /* .ARM.exidx is sorted, so has to go in its own output section.  */
    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > flash
    __exidx_end = .;

    /*
     * The irq stack is used for interrupt handling by the kernel. ARM Cortex
     * cores call it the "main" stack, but the name is confusing as it's *not*
     * the stack used for main(). This linker script places it at the bottom of
     * the ram, instead of the top. This is done for two reasons:
     * - as an optimization for microcontrollers with little ram memory.
     *   The implementation of malloc from newlib requests memory to the OS in
     *   blocks which can vary from 1KB to 4KB. Using the top of the RAM for the
     *   irq stack, whose size is usually less than one malloc block means
     *   the difference between the block size and the stack size is wasted RAM
     *   that malloc can't use.
     * - Many microcontrollers produce a fault if the CPU accesses memory below
     *   the RAM start address. By placing the interrupt stack at the beginning
     *   of the RAM we take advantage of this behavior to produce a fault on
     *   interrupt stack overflow. Placing the stack at the top would instead
     *   cause the stack to silently collide with the heap.
     */
    .irq_stack (NOLOAD) :
    {
        . = ORIGIN(ram);
        _irq_stack_bottom = .;
        . = . + _irq_stack_size;
        _irq_stack_top = .;
    } > ram

    .data : ALIGN(8)
    {
        _data = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(8);
        _edata = .;
    } > ram AT > flash
    _etext = LOADADDR(.data);

    _bss_start = .;
    .bss (NOLOAD) :
    {
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        . = ALIGN(8);
    } > ram
    _bss_end = .;

    /*
     * _end is both the end of the statically allocated RAM block (end of .bss)
     * and the start of the heap, while _heap_end is the end of the heap.
     * Just like .data and .bss above, this heap is used by the kernel and
     * kernelspace applications.
     */
    .heap (NOLOAD) :
    {
        _end = .;
        . = ORIGIN(ram) + LENGTH(ram) - _process_pool_size;
        _heap_end = .;
    } > ram

    /*
     * The process pool is the memory area reserved for running processes.
     * It uses a separate allocator and MPU regions are used to enforce memory
     * protection, when architectural support for it is present.
     * Linker scripts which do not override _process_pool_size get a zero-sized
     * process pool and are incapable of running processes.
     */
    .process_pool (NOLOAD) :
    {
        _process_pool_start = .;
        . = . + _process_pool_size;
        _process_pool_end = .;
    } > ram
}
