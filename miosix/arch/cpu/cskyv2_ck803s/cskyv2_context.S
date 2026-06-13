/***************************************************************************
 *   CK803S (C-SKY V2) context-switch entry stubs for modern Miosix.        *
 *   GPL v2+ with the Miosix linking exception.                            *
 *                                                                          *
 *   Two naked VBR entry points, both following the proven irq_entry.S      *
 *   stm/ldm + rte template and the modern PendSV contract shape            *
 *   "save full context -> bl C body -> restore context -> exception-return":*
 *                                                                          *
 *     yield_isr_entry   -> VBR[16]    (trap #0)   cooperative-yield path   *
 *     generic_irq_entry -> VBR[32..95] every PIC source incl. the OS tick  *
 *                          (the preemptive path; csky_isr_dispatch routes  *
 *                           to the registered handler then reschedules)    *
 *                                                                          *
 *   ===================== CONTEXT-SWITCH FRAME =====================       *
 *   CK803S HW auto-saves ONLY PC->EPC(cr4) and PSR->EPSR(cr2) on entry and *
 *   clears PSR.IE; it pushes NOTHING to the stack. So these stubs manually *
 *   push the FULL register set the kernel must preserve across an          *
 *   arbitrary-point preemption, on the *interrupted thread's* own stack,   *
 *   full-descending. ck803 implements only the 16-register C-SKY V2 base   *
 *   file (r0-r15) — VERIFIED: gcc -mcpu=ck803 never allocates above r15,   *
 *   and the HW-proven irq_test.S timer ISR saves exactly r0-r13+r15. So    *
 *   the complete frame is:                                                 *
 *                                                                          *
 *     offset  content              (from post-push SP, low->high address)  *
 *     ------  -------                                                       *
 *     +0x00   r0  (a0)   volatile, also threadLauncher arg0                *
 *     +0x04   r1  (a1)   volatile, also threadLauncher arg1                *
 *     +0x08   r2  (a2)                                                      *
 *     +0x0c   r3  (a3)                                                      *
 *     +0x10   r4  (l0)   callee-saved                                       *
 *     +0x14   r5  (l1)   callee-saved                                       *
 *     +0x18   r6  (l2)   callee-saved                                       *
 *     +0x1c   r7  (l3)   callee-saved                                       *
 *     +0x20   r8  (l4)   callee-saved                                       *
 *     +0x24   r9  (l5)   callee-saved                                       *
 *     +0x28   r10 (l6)   callee-saved                                       *
 *     +0x2c   r11 (l7)   callee-saved                                       *
 *     +0x30   r12 (t0)   volatile                                          *
 *     +0x34   r13 (t1)   volatile                                          *
 *     +0x38   r15 (lr)   return address of interrupted code                *
 *     +0x3c   EPC        HW-saved resume PC  (cr<4,0>)                      *
 *     +0x40   EPSR       HW-saved resume PSR (cr<2,0>)                      *
 *     +0x44   (pad)      keep frame 8-byte aligned (C-SKY V2 ABI)          *
 *     ------  total = 0x48 = 72 bytes = CTXSAVE_ON_STACK (cpu_const_impl.h) *
 *                                                                          *
 *   sp(r14) is NOT in the frame — it is the frame pointer itself, published*
 *   to ctxsave[0]. ck803 has no GBR/r28. Carrying EPC/EPSR IN the frame    *
 *   (not relying on the live cr4/cr2 surviving the C body) makes resume    *
 *   UNIFORM between an already-running thread and a brand-new one:         *
 *   initKernelThreadCtxsave (cpu.cpp) lays down an identical frame with    *
 *   r0=arg0, r1=arg1, EPC=threadLauncher, EPSR=IE-set, so the first rte    *
 *   jumps straight into the launcher with interrupts enabled and the args  *
 *   already in r0/r1.                                                      *
 *   ================================================================       *
 *                                                                          *
 *   ctxsave is "volatile unsigned int *ctxsave[CPU_NUM_CORES]" (cpu.h).    *
 *   Single core: ctxsave[0] -> the running thread's CTXSAVE_SIZE-word      *
 *   buffer, whose word[0] (STACK_OFFSET_IN_CTXSAVE=0) holds the thread SP. *
 ***************************************************************************/

    .equ FRAME_SIZE, 0x48      /* 72 bytes, == CTXSAVE_ON_STACK */
    .equ OFF_R15,    0x38
    .equ OFF_EPC,    0x3c
    .equ OFF_EPSR,   0x40

    .text
    .align  2

/* -------------------------------------------------------------------- *
 *  CTX_SAVE: push the full frame onto the current (thread) SP, snapshot  *
 *  EPC/EPSR into it, then publish SP into ctxsave[0].                    *
 * -------------------------------------------------------------------- */
    .macro  CTX_SAVE
    subi    sp, sp, FRAME_SIZE
    stm     r0-r13, (sp)            /* r0..r13 -> +0x00..+0x34 */
    st.w    r15, (sp, OFF_R15)      /* LR of interrupted code */
    mfcr    r0, cr<4, 0>            /* EPC  = HW-saved resume PC  */
    st.w    r0, (sp, OFF_EPC)
    mfcr    r0, cr<2, 0>            /* EPSR = HW-saved resume PSR */
    st.w    r0, (sp, OFF_EPSR)
    lrw     r0, ctxsave             /* r0 = &ctxsave[0] (the pointer slot) */
    ld.w    r0, (r0, 0)             /* r0 = ctxsave[0] (-> thread buffer) */
    st.w    sp, (r0, 0)             /* thread buffer[0] = thread SP */
    sync                            /* barrier == ARM dmb */
    .endm

/* -------------------------------------------------------------------- *
 *  CTX_RESTORE: reload SP from ctxsave[0] (the scheduler may have        *
 *  repointed ctxsave at a DIFFERENT thread), write EPC/EPSR back, pop    *
 *  the frame and rte (PC<-EPC, PSR<-EPSR, re-enabling IE).               *
 * -------------------------------------------------------------------- */
    .macro  CTX_RESTORE
    lrw     r0, ctxsave
    ld.w    r0, (r0, 0)             /* r0 = ctxsave[0] (chosen thread buffer) */
    ld.w    sp, (r0, 0)             /* SP = chosen thread's saved SP */
    ld.w    r0, (sp, OFF_EPSR)
    mtcr    r0, cr<2, 0>            /* EPSR <- resume PSR */
    ld.w    r0, (sp, OFF_EPC)
    mtcr    r0, cr<4, 0>            /* EPC  <- resume PC (launcher for new thr) */
    ld.w    r15, (sp, OFF_R15)
    ldm     r0-r13, (sp)            /* restore r0..r13 (r0/r1 = args for new thr) */
    addi    sp, sp, FRAME_SIZE
    rte                             /* PC<-EPC, PSR<-EPSR; re-enables IE */
    .endm

/* ============ first switch (à la uC/OS OSStartHighRdy) ============= *
 * Directly restore the first thread's context and rte into it, WITHOUT a   *
 * trap. The scheduler has already pointed ctxsave[0] at the first thread.   *
 * CTX_RESTORE loads its SP, writes the frame's EPC(launcher)/EPSR(IE+EE) and *
 * rte's into the thread. Never returns. (Avoids depending on the trap       *
 * vector, which differs from our assumed 16.)                               */
    .globl  csky_first_switch
    .type   csky_first_switch, @function
csky_first_switch:
    CTX_RESTORE
    .size   csky_first_switch, .-csky_first_switch

/* =========== cooperative switch from THREAD context =============== *
 * Mirrors RT-Thread rt_hw_context_switch: synchronously save the current     *
 * thread (resume PC = the .Lyield_resume trampoline, PSR = live psr), CLEAR    *
 * IE, run the scheduler + repoint ctxsave with IRQs OFF, then restore the next *
 * thread and rte into it (rte restores the saved EPSR, re-enabling IE). Called *
 * by IRQinvokeScheduler() when interrupts are enabled (thread context). No     *
 * trap/PendSV needed — rte transfers control directly (given the reset-time    *
 * PSR setup in Reset_Handler). The psrclr ie below is REQUIRED, not optional.  */
    .globl  csky_yield_switch
    .type   csky_yield_switch, @function
csky_yield_switch:
    subi    sp, sp, FRAME_SIZE
    stm     r0-r13, (sp)
    st.w    r15, (sp, OFF_R15)         /* real return address of the caller */
    mfcr    r0, psr
    st.w    r0, (sp, OFF_EPSR)         /* resume with the live PSR (IE=1 here, so
                                          the rte below re-enables IRQs on resume) */
    psrclr  ie                         /* CRITICAL: run the scheduler + ctxsave
                                          repoint with IRQs OFF. IRQ-prefixed kernel
                                          fns require the global lock held, which on
                                          this single-core build == IE off (FastGlobal
                                          LockFromIrq is a no-op). Without this, a
                                          WAKE/timer IRQ landing mid-IRQrunScheduler
                                          re-enters the scheduler on the same ready-
                                          lists -> corruption (the fast-keypress hang).
                                          Matches RT-Thread rt_hw_context_switch. */
    lrw     r0, .Lyield_resume
    st.w    r0, (sp, OFF_EPC)          /* resume PC -> trampoline that rts back */
    lrw     r0, ctxsave
    ld.w    r0, (r0, 0)
    st.w    sp, (r0, 0)                /* publish current SP into ctxsave[0] */
    sync
    jbsr    csky_isr_yield             /* Scheduler::IRQrunScheduler() (repoints) */
    lrw     r0, ctxsave
    ld.w    r0, (r0, 0)
    ld.w    sp, (r0, 0)                /* SP = next thread */
    ld.w    r0, (sp, OFF_EPSR)
    mtcr    r0, cr<2, 0>
    ld.w    r0, (sp, OFF_EPC)
    mtcr    r0, cr<4, 0>
    ld.w    r15, (sp, OFF_R15)
    ldm     r0-r13, (sp)
    addi    sp, sp, FRAME_SIZE
    rte
.Lyield_resume:
    rts                                /* resumed: return to IRQinvokeScheduler */
    .size   csky_yield_switch, .-csky_yield_switch

/* ==================== trap #0 = cooperative YIELD ================== */
    .globl  yield_isr_entry
    .type   yield_isr_entry, @function
yield_isr_entry:
    CTX_SAVE
    jbsr    csky_isr_yield          /* extern "C" -> Scheduler::IRQrunScheduler() */
    CTX_RESTORE
    .size   yield_isr_entry, .-yield_isr_entry

/* ============ all PIC sources = peripheral IRQ + OS TICK ========== *
 * Every PIC vector (32..95) lands here. csky_isr_dispatch services the *
 * fired source(s) via the registered handler — including the OS tick,  *
 * which the chip layer registers like any other source — then          *
 * reschedules iff a handler called IRQinvokeScheduler (which sets the   *
 * pending flag while IE is off). This is the deferred-PendSV behaviour: *
 * the switch is taken here, at the enclosing IRQ's CTX_RESTORE.         */
    .globl  generic_irq_entry
    .type   generic_irq_entry, @function
generic_irq_entry:
    CTX_SAVE
    jbsr    csky_isr_dispatch       /* extern "C" -> PIC dispatch + maybe sched */
    CTX_RESTORE
    .size   generic_irq_entry, .-generic_irq_entry
