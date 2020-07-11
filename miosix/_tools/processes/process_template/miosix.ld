/*
 * Linker script for writing PROCESSES for the Miosix embedded OS
 */

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
        *(.gcc_except_table)
        *(.gcc_except_table.*)
        *(.ARM.extab*)
        *(.gnu.linkonce.armextab.*)
    }

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    }
    __exidx_end = .;
    
    .rodata : ALIGN(8)
    {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
    }

    .rel.data : { *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*) }
    .rel.got  : { *(.rel.got) }

    /* Here starts the second segment, that is copied in RAM and relocated */
    . = 0x10000000;

    .got      : { *(.got.plt) *(.igot.plt) *(.got) *(.igot) }

    /* FIXME: If this is put in the other segment, it makes it writable */
    .dynamic  : { *(.dynamic) }

    /* FIXME: this name is temporary, find the proper one */
    .initfini : ALIGN(8)
    {
        KEEP(*(.init))

        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;

        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;

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
    _end = .;

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
