/*
 * Linker script for LPC2000
 * For compiling Miosix as a unikernel.
 * Allocates all RAM for the kernel and kernelspace application, no process pool
 */

/*
 * The ARM7TDMI core has 7 operating modes: USR, SYS, SVC, IRQ, FIQ, ABT, UND.
 * USR and SYS share the stack pointer, but all other modes 6 have a separate
 * hardware stack. The Miosix ARMv4 CPU support in file interrupts.cpp makes the
 * following simplifications:
 * - ABT, UND, SVC, IRQ share the same stack, called the IRQ stack
 * - USR and SYS are given a stack by the kernel, not here
 * - FIQ is unused so by default its stack size is 0 in this linker script
 *
 * NOTE: the generic linker script in Miosix defines a memory region for the
 * IRQ stack whose size is overridable with _irq_stack_size, and also provides
 * the _irq_stack_top symbol. It does however *not* reserve space for a FIQ
 * stack, so here we use the following workaround:
 * We change the meaning of _irq_stack_size to be the sum of the size of the
 * IRQ and FIQ stacks, and piggyback the FIQ stack at the bottom of the IRQ
 * stack. This is understandably unintuitive, but ARMv4 support in Miosix is
 * considered somewhat legacy, and on top of that no Miosix firwmare I'm aware
 * of ever used the FIQ so that's good enough.
 */
_irq_stack_size = 512;  /* NOTE: Sum of the IRQ and FIQ stack sizes! */
_fiq_stack_size = 0;    /* Size of the FIQ stack */
_fiq_stack_top  = ORIGIN(ram) + _fiq_stack_size;

MEMORY
{
    flash(rx)   : ORIGIN = 0,          LENGTH = 500K   /* 512K-12K bootloader */
    /*
     * NOTE: the upper 32 byte should be reserved for IAP but doing so means
     * the top 1KB block won't be usable by malloc. Since no Miosix firmware
     * for LPC2000 is known too use IAP, we'll just not reserve those bytes
     */
    /* ram(wx)     : ORIGIN = 0x40000000, LENGTH = 32K-32 */
    ram(wx)     : ORIGIN = 0x40000000, LENGTH = 32K
}

INCLUDE ldscripts/miosix-flash-ram.ld
