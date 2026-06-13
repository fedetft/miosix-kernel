/***************************************************************************
 *   CK803S (C-SKY V2) cpu_impl.h for modern Miosix.                        *
 *   GPL v2+ with the Miosix linking exception (see armv6m cpu_impl.h).     *
 *                                                                          *
 *   Unlike the Cortex-M ports, the CK803S context switch is implemented in *
 *   hand-written assembly entry stubs (cskyv2_context.S), NOT via          *
 *   saveContext()/restoreContext() C macros: CK803S has no HW register     *
 *   auto-stacking, so the full-frame push/pop is cleaner kept whole in one *
 *   .S file. The interfaces_private/cpu.h contract only *requires* those   *
 *   macros for the arch's own naked handler, which here lives in the .S —  *
 *   so they are intentionally not provided. The kernel core never          *
 *   references them.                                                       *
 ***************************************************************************/

#pragma once

#include "interfaces/arch_registers.h"
#include "interfaces/interrupts.h"   // areInterruptsEnabled()

namespace miosix {

/**
 * \internal
 * Pending-scheduler flag, the CK803S substitute for the Cortex-M PendSV-set
 * bit. Set by IRQinvokeScheduler() when it cannot synchronously trap (i.e. it
 * is called with interrupts disabled — from an IRQ handler or under the global
 * lock). Checked and cleared by the C interrupt bodies (csky_isr_tick /
 * csky_isr_dispatch in interrupts.cpp) just before they reschedule, so the
 * actual context switch happens at the enclosing IRQ's CTX_RESTORE.
 * Defined in interrupts.cpp.
 */
extern volatile bool s_schedPending;
extern volatile bool s_inIrq;

} //namespace miosix

//Synchronous cooperative context switch from thread context (cskyv2_context.S):
//saves the current thread, runs the scheduler, restores+rte into the next.
extern "C" void csky_yield_switch();

namespace miosix {

/**
 * \internal
 * Request a context switch. Per the cpu.h contract this must be callable both
 * with and without the global lock.
 *
 * CK803S has neither a PendSV exception nor (verified against the HR_C7000
 * manual) any software-set-pending PIC source like the armv4 VICSoftInt. The
 * only software switch trigger is the synchronous `trap` instruction, which
 * fires regardless of PSR.IE. So:
 *
 *  - IE on  (thread context, no global lock): `trap 0` vectors to
 *    yield_isr_entry, which saves context, runs the scheduler and restores —
 *    a prompt cooperative yield.
 *
 *  - IE off (inside an IRQ, or under the global lock): trapping now would
 *    nest into the critical section, so instead raise s_schedPending. When the
 *    call is from an IRQ handler, the enclosing dispatcher reschedules before
 *    its CTX_RESTORE — exactly the deferred-PendSV behaviour. When the call is
 *    from a thread holding the global lock, the switch is taken at the next OS
 *    tick (csky_isr_tick honours the flag); this bounds latency to one tick.
 *    PHASE-1 LIMITATION (see project_hd2_miosix_port): a thread that unblocks a
 *    higher-priority thread under the lock keeps running until that next tick.
 *    Acceptable for bring-up; the full fix needs the global-lock release path
 *    to honour the flag.
 */
inline void IRQinvokeScheduler() noexcept
{
    //Switch synchronously ONLY from true thread context with interrupts enabled
    //(no global lock): Thread::yield(). When called with IE off — under the
    //global lock OR from an IRQ — DEFER (s_schedPending); switching synchronously
    //under the lock corrupts mid-kernel-operation (hangs at Thread::create).
    //LIMITATION: the deferred switch is currently only taken at the next IRQ
    //(no PendSV-equivalent yet), so Thread::sleep is not fully functional —
    //needs the core-VIC TSPEND/PendSV (vector 22) to fire at lock-release.
    if(areInterruptsEnabled() && !s_inIrq)
    {
        csky_yield_switch();
    } else {
        s_schedPending=true;
        asm volatile("":::"memory");
    }
}

} //namespace miosix
