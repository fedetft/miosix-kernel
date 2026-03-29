/*
 * C++ enabled linker script for LPC2000 (500K FLASH, 32K RAM)
 * Optimized for use with the Miosix kernel
 */

/*
 * This linker script puts:
 * - read only data and code (.text, .rodata, .eh_*) in flash
 * - stacks, heap and sections .data and .bss in RAM
 */

_fiq_stack_size = 0x00000000;  /* stack for "FIQ" 0 bytes    */
_irq_stack_size = 0x00000200;  /* stack for "IRQ" 512 bytes  */
ASSERT(_fiq_stack_size % 4 == 0, "FIQ stack size error");
ASSERT(_irq_stack_size % 4 == 0, "IRQ stack size error");
_fiq_stack_top  = 0x40007fe0;  /* end of available RAM (top 32bytes reserved) */
_irq_stack_top  = _fiq_stack_top - _fiq_stack_size;
_heap_end       = _irq_stack_top - _irq_stack_size;

/* identify the Entry Point  */
ENTRY(_ZN6miosix13Reset_HandlerEv)

/* specify the memory areas  */
MEMORY
{
    flash(rx)   : ORIGIN = 0,          LENGTH = 500K  /* 512K-12K bootloader */
    ram(wx)     : ORIGIN = 0x40000000, LENGTH = 32736 /* free RAM area       */
}

/* now define the output sections  */
SECTIONS
{
    . = 0;

    /* .text section: code goes to flash */
    .text :
    {
        /* Startup code must go at address 0 */
        KEEP(*(.isr_vector))

        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
        /* these sections for thumb interwork? */
        *(.glue_7)
        *(.glue_7t)
        /* these sections for C++? */
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
        . = ALIGN(0x4);
        _ctor_start = .;
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
       _ctor_end = .;

        . = ALIGN(0x4);
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

    /* .data section: global variables go to ram, but also store a copy to
       flash to initialize them */
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

    /* .bss section: uninitialized global variables go to ram */
    _bss_start = .;
    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        . = ALIGN(8);
    } > ram
    _bss_end = .;

    _end = .;
    PROVIDE(end = .);
}
