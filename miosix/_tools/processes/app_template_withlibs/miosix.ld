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
    
    .rodata : ALIGN(8)
    {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
    }

    .rel.data : { *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*) }
    .rel.got  : { *(.rel.got) }
    .rel.plt  : { *(.rel.plt) } /* TODO: what is this? */

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    }
    __exidx_end = .;

    _end = .;
    PROVIDE(end = .);

    /* Here starts the second segment, that is copied in RAM and relocated */
    . = 0x10000000;

    .got      : { *(.got.plt) *(.igot.plt) *(.got) *(.igot) }

    /* FIXME: If this is put in the other segment, it makes it writable */
    .dynamic  : { *(.dynamic) }

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
        *(.debug*)
    }
}
