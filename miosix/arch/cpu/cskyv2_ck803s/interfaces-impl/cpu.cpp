/***************************************************************************
 *   CK803S (C-SKY V2) cpu.cpp for modern Miosix.                           *
 *   GPL v2+ with the Miosix linking exception (see armv6m cpu.cpp).        *
 *                                                                          *
 *   New-thread context seeding + the architecture-specific kernel start.   *
 *   The frame laid down here MUST match cskyv2_context.S's CTX_SAVE/RESTORE *
 *   (72 bytes: r0-r13, r15, EPC, EPSR, pad) and CTXSAVE_ON_STACK.          *
 ***************************************************************************/

#include "interfaces_private/cpu.h"
#include "interfaces/arch_registers.h"
#include "interfaces/interrupts.h"   // fastEnableIrq()
#include "kernel/scheduler/scheduler.h"

//Direct first-thread restore (cskyv2_context.S): loads ctxsave[0]'s frame and
//rte's into it. Never returns.
extern "C" void csky_first_switch();

namespace miosix {

void initKernelThreadCtxsave(unsigned int *ctxsave, void (*pc)(void *(*)(void*),void*),
                             unsigned int *sp, unsigned int *spLimit,
                             void *(*arg0)(void*), void *arg1) noexcept
{
    (void)spLimit;   //hardware stack-overflow detection is an MPU job (Phase 2+)

    //Stack is full descending: the frame occupies the CTXSAVE_ON_STACK bytes
    //just below the given top-of-stack. Word indices == byte offset / 4, and
    //MUST match cskyv2_context.S exactly.
    unsigned int *frame=sp-(CTXSAVE_ON_STACK/4);   //18 words below the top

    frame[0]=reinterpret_cast<unsigned int>(arg0);              // r0  (+0x00) launcher arg0
    frame[1]=reinterpret_cast<unsigned int>(arg1);              // r1  (+0x04) launcher arg1
    frame[2]=0;                                                 // r2  (+0x08)
    frame[3]=0;                                                 // r3  (+0x0c)
    frame[4]=0;                                                 // r4  (+0x10)
    frame[5]=0;                                                 // r5  (+0x14)
    frame[6]=0;                                                 // r6  (+0x18)
    frame[7]=0;                                                 // r7  (+0x1c)
    frame[8]=0;                                                 // r8  (+0x20)
    frame[9]=0;                                                 // r9  (+0x24)
    frame[10]=0;                                                // r10 (+0x28)
    frame[11]=0;                                                // r11 (+0x2c)
    frame[12]=0;                                                // r12 (+0x30)
    frame[13]=0;                                                // r13 (+0x34)
    frame[14]=0xffffffff;                                       // r15 (+0x38) lr sentinel
    frame[15]=reinterpret_cast<unsigned int>(pc);              // EPC (+0x3c) -> threadLauncher
    //EPSR (+0x40): the exact thread PSR RT-Thread's ck803 port uses (verified on
    //this core): 0x80000140 = S(bit31, SUPERVISOR) | EE(bit8) | IE(bit6). The
    //SUPERVISOR bit is essential — without it the thread runs in user mode and
    //its first privileged/MMIO access faults. (Was csky_get_psr()|IE|EE, which
    //could drop bit31 and leave the thread in user mode.)
    frame[16]=(1u<<31)|(1u<<HD2_PSR_EE_BIT)|(1u<<HD2_PSR_IE_BIT); //0x80000140
    frame[17]=0;                                                // pad (+0x44) 8-byte align

    ctxsave[STACK_OFFSET_IN_CTXSAVE]=reinterpret_cast<unsigned int>(frame); //ctxsave[0]=SP
}

void IRQportableStartKernel() noexcept
{
    //Throwaway ctxsave so the scheduler has a valid current-thread pointer.
    static unsigned int s_ctxsave[CTXSAVE_SIZE];
    ctxsave[getCurrentCoreId()]=s_ctxsave;

    //First switch the uC/OS OSStartHighRdy way: directly restore the first
    //thread, NOT via a trap. trap 0 does not vector to our handler on this core
    //(vector != our assumed 16; VBR is correctly installed). So:
    //  1. run the scheduler to point ctxsave[0] at the highest-priority thread,
    //  2. csky_first_switch() (asm) loads that thread's SP + frame (EPC=launcher,
    //     EPSR=IE+EE) and rte's into it — interrupts get enabled by that rte.
    Scheduler::IRQrunScheduler();   //select first thread (repoints ctxsave[0])
    csky_first_switch();            //restore its frame + rte into it (now works,
                                    //given the reset-time PSR setup)
    //Never reaches here
}

/**
 * \internal
 * Idle-thread CPU sleep (interfaces_private/sleep.h). PHASE 1: a no-op, so the
 * idle thread busy-spins — functionally correct (the os-timer IRQ still
 * preempts it). TODO: use the CK803S `wait` instruction (WFI equivalent) for
 * real power saving once interrupt-wake behaviour is confirmed on hardware.
 */
void sleepCpu() {}

} //namespace miosix
