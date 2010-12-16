/* *****************************************************
Miosix boot system V2.0
Stage 1 boot process
This code will set up the stacks, initialize global
variables and jump to stage_2_boot.
C and C++ are supported.
Also includes the interrupt table.
***************************************************** */

/* Mode bits and Interrupt (I & F) flags in PSRs (program status registers) */
.set  MODE_USR, 0x10  /* User Mode                      */
.set  MODE_FIQ, 0x11  /* FIQ Fast Interrupt Mode        */
.set  MODE_IRQ, 0x12  /* IRQ Interrupt Mode             */
.set  MODE_SVC, 0x13  /* Supervisor Call Interrupt Mode */
.set  MODE_ABT, 0x17  /* Abort (memory fault) Mode      */
.set  MODE_UND, 0x1B  /* Undefined Instructions Mode    */
.set  MODE_SYS, 0x1F  /* System Mode                    */
.set  I_BIT, 0x80     /* if I bit set, IRQ is disabled  */
.set  F_BIT, 0x40     /* if F bit set, FIQ is disabled  */

.text
.arm

.global  _startup
.func    _startup

_startup:

/* Interrupt vectors */

_vectors:
    ldr     pc, Reset_Addr
    ldr     pc, Undef_Addr
    ldr     pc, SWI_Addr
    ldr     pc, PAbt_Addr
    ldr     pc, DAbt_Addr
    nop                      /* Reserved for ISP checksum  */
    ldr     pc, [pc,#-0xFF0] /* Jump to VIC                */
    ldr     pc, FIQ_Addr

Reset_Addr: .word   Reset_Handler  /* defined below                          */
Undef_Addr: .word   UNDEF_Routine  /* defined in interrupts.cpp              */
SWI_Addr:   .word   kernel_SWI_Routine /* defined in kernel/portability.cpp  */
PAbt_Addr:  .word   PABT_Routine   /* defined in interrupts.cpp              */
DAbt_Addr:  .word   DABT_Routine   /* defined in interrupts.cpp              */
FIQ_Addr:   .word   FIQ_Routine    /* defined in interrupts.cpp              */
            .word   0              /* rounds the interrupt vector to 64Bytes */

/* Reset Handler */

Reset_Handler:  
    /* Setup a stack for each mode */
    msr     CPSR_c, #MODE_UND|I_BIT|F_BIT  /* Undef      (IRQ Off, FIQ Off) */
    ldr     sp, =_und_stack_top

    msr     CPSR_c, #MODE_ABT|I_BIT|F_BIT  /* Abort      (IRQ Off, FIQ Off) */
    ldr     sp, =_abt_stack_top

    msr     CPSR_c, #MODE_FIQ|I_BIT|F_BIT  /* FIQ        (IRQ Off, FIQ Off) */
    ldr     sp, =_fiq_stack_top

    msr	    CPSR_c, #MODE_IRQ|I_BIT        /* IRQ        (IRQ Off, FIQ On ) */
    ldr     sp, =_irq_stack_top

    msr	    CPSR_c, #MODE_SVC|I_BIT        /* Supervisor (IRQ Off, FIQ On ) */
    ldr     sp, =_svc_stack_top

    msr	    CPSR_c, #MODE_SYS|I_BIT|F_BIT  /* System     (IRQ Off, FIQ Off) */
    ldr	    sp, =_sys_stack_top

    /* copy .data section from FLASH to RAM */
    ldr     r1, =_etext
    ldr     r2, =_data
    ldr	    r3, =_edata
1:  cmp	    r2, r3
    ldrlo   r0, [r1], #4
    strlo   r0, [r2], #4
    blo	    1b
    /* clear .bss section */
    mov	    r0, #0
    ldr	    r1, =_bss_start
    ldr	    r2, =_bss_end
2:  cmp	    r1, r2
    strlo    r0, [r1], #4
    blo	    2b
    /* call static constructors. Supports both eabi and elf binary formats */
    ldr	    r0, =__preinit_array_start
    ldr	    r1, =__preinit_array_end
    bl	    _call_constructors
    ldr	    r0, =_ctor_start
    ldr	    r1, =_ctor_end
    bl	    _call_constructors
    ldr	    r0, =__init_array_start
    ldr	    r1, =__init_array_end
    bl	    _call_constructors
    /* enter stage_2_boot. the lr (return address) is set to 0x00000000, so
    if main returns, it will jump to the reset vector, rebooting the system.
    Using bx instead of b to support thumb mode */
    ldr     r12, =_init
    ldr	    lr, =0
    bx      r12

.endfunc

/*
Some info about static constructors: by reading newlib and gcc sources, and by
searching on the web it looks like static constructors are handled in a
different way in every binary format. The common part is this: the compiler
creates an array of function pointers, in a particular section. The startup
code, before calling main(), calls every function in the array, initializing
all static and global objets. But the name of the section is diferent: eabi
compilers (like codesourcery gcc) place the array in .preinit_array,
.init_array (for constructors) and .fini_array (for destructors) while elf
compilers (like arm-elf-g++) place the array in .ctor (for constructors) and
.dtor (for destructors). Constructors in .preinit_array and .init_array
sections are called by the function __libc_init_array (defined in
newlib-1.16.0/newlib/libc/misc/init.c). This function also calls _init, which
is supposed to call constructors in .ctor section.
By callng all constructors in .preint_array, .ctor and .init_array all
consructors will be called regardless of the binary format (elf or eabi).
This file has been used for over a year, so it is believed to be accurate.
*/
.func    _call_constructors
_call_constructors:
    stmfd   sp!, {lr}
4:  cmp	    r0,r1
    beq	    5f              /* 5f = forward reference to label  */
    ldr	    r2, [r0], #4
    stmfd   sp!, {r0-r1}
    mov     lr, pc
    bx      r2
    ldmfd   sp!, {r0-r1}
    b       4b              /* 4b = backward reference to label */
5:  ldmfd   sp!, {pc}
.endfunc
	
.end
