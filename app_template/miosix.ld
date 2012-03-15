/*
 * Linker script for writing PROGRAMS for the Miosix embedded OS
 * TFT:Terraneo Federico Technlogies
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
    }

    .rel.data : { *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*) }
    .rel.got  : { *(.rel.got) }

    /* Here starts the second segment, that is copied in RAM and relocated */
    . = 0x10000000;

    .got      : { *(.got.plt) *(.igot.plt) *(.got) *(.igot) }

    /* FIXME: If this is put in the other segment, it makes it writable */
    .dynamic  : { *(.dynamic) }

    /* FIXME: The compiler insists in addressing rodata relative to r9 */
    .rodata : ALIGN(8)
    {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
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
