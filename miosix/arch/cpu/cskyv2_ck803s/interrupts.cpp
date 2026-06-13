/***************************************************************************
 *   CK803S (C-SKY V2) / HR_C7000 interrupt layer for modern Miosix.        *
 *   GPL v2+ with the Miosix linking exception.                            *
 *                                                                          *
 *   Implements the interfaces/interrupts.h contract for the HD2:           *
 *     - IRQinitIrqTable()         build+install the VBR table, init the PIC *
 *     - IRQregisterIrqOnCore()    bind a dynamic handler to a PIC source    *
 *     - IRQunregisterIrqOnCore()  unbind + mask                            *
 *     - IRQisIrqRegistered()                                               *
 *   plus the extern "C" C bodies the cskyv2_context.S entry stubs call.    *
 *                                                                          *
 *   Hardware recipe verified on real HD2 silicon (OpenRTX irq_test.c +     *
 *   project_hd2_interrupt_recipe):                                         *
 *     timer/peripheral IRQ -> PIC @ 0x17000000 (level, active-high)        *
 *       -> CK803 autovectors via VBR (cr<1,0>): PC = *(VBR + vec*4)        *
 *       -> vector = HD2_PIC_VECTOR0(32) + PIC source number.               *
 *     IRQ end: peripheral clears its own request (e.g. read TimerN_EOI),   *
 *       then write PIC_COW1 = eoi(bit2).                                    *
 *                                                                          *
 *   The "id" used throughout the Miosix IRQ API is the PIC source number   *
 *   (0..63); its CK803S vector is id + HD2_PIC_VECTOR0.                     *
 ***************************************************************************/

#include "interfaces/interrupts.h"
#include "interfaces/arch_registers.h"
#include "interfaces_private/cpu.h"             //ctxsave (resume-frame forensics)
#include "kernel/scheduler/scheduler.h"
#include "../../../../openrtx_hd2/hd2_crumb.h"  //hard-lock forensics stamps

namespace miosix {

//
// CK803S substitute for the Cortex-M PendSV-set bit (see cpu_impl.h).
//
volatile bool s_schedPending=false;

//True while inside the PIC dispatcher (IRQ context). IRQinvokeScheduler() uses
//this to decide: in IRQ context -> defer (s_schedPending); in thread context
//(even under the global lock / IE off) -> switch synchronously.
volatile bool s_inIrq=false;

//
// VBR table: HD2_VBR_NVEC word entries, each a handler ADDRESS. CK803 loads the
// word at VBR + vec*4 and jumps to it. Must be 1 KiB aligned (VBR alignment).
//
static unsigned int g_vbrTable[HD2_VBR_NVEC] __attribute__((aligned(1024)));

//
// Dynamic per-source handler table. One slot per PIC source (0..63).
//
static constexpr unsigned int NUM_PIC_SOURCES=64;
struct IrqSlot { void (*fn)(void*); void *arg; };
static IrqSlot g_irqSlots[NUM_PIC_SOURCES];

//
// Naked entry stubs (cskyv2_context.S).
//
extern "C" void yield_isr_entry();      // VBR[16] = trap 0  (cooperative yield)
extern "C" void generic_irq_entry();    // VBR[32..95] = PIC sources

//
// Boot entry. The generic linker script sets ENTRY(miosix::Reset_Handler) and
// KEEPs .isr_vector at the very start of flash; the Dahua IAP jumps to the
// image base (0x0300d000) and EXECUTES there (project_hd2_diagboot_works), so
// Reset_Handler must be the first code. It sets the boot stack, runs early
// clock init (IRQmemoryAndClockInit, board layer), then hands off to the kernel
// boot (IRQkernelBootEntryPoint does .data/.bss/ctors/IRQinitIrqTable/IRQbspInit
// /IRQosTimerInit/IRQstartKernel). Mirrors armv4's Reset_Handler. Naked: pure
// asm, no compiler-generated prologue, and it switches SP mid-function. The
// mangled callee names match the kernel's C++ symbols.
//
void Reset_Handler() __attribute__((naked,section(".isr_vector")));
void Reset_Handler()
{
    asm volatile(
        //Mirror RT-Thread's ck803 reset_handler entry: put the core in the known
        //mode it expects (PSR=0x80000000 = S/supervisor, IE/EE off) and clear the
        //random-prefetch enable (cr<31,0>[3]=RPE). Without this PSR setup, rte
        //from normal context does not transfer control on this core (HW-found).
        "lrw  r0, 0x80000000                        \n\t"
        "mtcr r0, psr                               \n\t"
        "mfcr r0, cr<31, 0>                         \n\t"
        "bclri r0, 3                                \n\t"
        "mtcr r0, cr<31, 0>                         \n\t"
        "lrw  r0, _irq_stack_top                    \n\t" //small stack in internal RAM
        "mov  sp, r0                                \n\t"
        "jbsr _ZN6miosix21IRQmemoryAndClockInitEv   \n\t" //early clock/memory init
        "lrw  r0, _heap_end                         \n\t" //big boot stack (top of heap)
        "mov  sp, r0                                \n\t"
        "jbsr _ZN6miosix23IRQkernelBootEntryPointEv \n\t" //never returns
    );
}

//
// Catch-all for any unexpected exception/IRQ: spin so a stray vector is
// observable (radio wedges) rather than running wild. NOT naked — it must
// never return anyway.
//
static void unexpectedIrq()
{
    for(;;) ;
}

//-----------------------------------------------------------------------------
// C bodies called by the .S entry stubs (between CTX_SAVE and CTX_RESTORE).
//-----------------------------------------------------------------------------

/// Run the scheduler (repoint ctxsave[]) — called by csky_yield_switch (after
/// it has saved the current thread) and by the IRQ dispatcher.
extern "C" void csky_isr_yield()
{
    Scheduler::IRQrunScheduler();
}

/// PIC path: service every fired+enabled source, PIC-EOI, then reschedule iff a
/// handler requested it (IRQinvokeScheduler set s_schedPending while IE was off).
extern "C" void csky_isr_dispatch()
{
    s_inIrq=true;   //handlers calling IRQinvokeScheduler defer (don't switch here)

    //Phase 1 wires only sources 0..31 (timer tick); scan the high half too so
    //adding a >=32 source later "just works".
    unsigned int pend0=HD2_PIC_INT_ST  & ~HD2_PIC_MASK;
    unsigned int pend1=HD2_PIC_INT_ST1 & ~HD2_PIC_MASK1;
    for(unsigned int src=0; src<32; src++)
        if(pend0 & (1u<<src)) { IrqSlot &s=g_irqSlots[src]; if(s.fn) s.fn(s.arg); }
    for(unsigned int src=0; src<32; src++)
        if(pend1 & (1u<<src)) { IrqSlot &s=g_irqSlots[32+src]; if(s.fn) s.fn(s.arg); }

    //Clear the PIC's latched interrupt-request RECORD for every source we just
    //serviced. PIC_INT_ST is a write-1-to-clear record register (manual
    //§4.7.4.2: "Each interrupt source can only record one interrupt request.
    //Writing 1 to the corresponding bit clears the status."). COW1=eoi only pops
    //the single highest-priority in-service entry (the ISR) — it does NOT clear
    //these records. Without this clear, as soon as TWO sources are pending in the
    //same dispatch (which begins when the TIME_CH overflow at ~102s adds a 2nd
    //frequently-firing source alongside WAKE_CH) the un-popped record stays
    //latched forever and is re-dispatched on every subsequent IRQ — a spurious
    //IRQ storm that re-runs the overflow handler ~thousands/s, racing getTime()
    //and collapsing all sleeps into a busy-loop. Write AFTER the handlers run so
    //a level source they de-asserted stays clear; one still asserting re-latches.
    if(pend0) HD2_PIC_INT_ST =pend0;
    if(pend1) HD2_PIC_INT_ST1=pend1;
    HD2_PIC_COW1=HD2_PIC_COW1_EOI;          //PIC interrupt-end (pop in-service)
    asm volatile("":::"memory");

    s_inIrq=false;  //about to leave IRQ ctx (generic_irq_entry's CTX_RESTORE switches)
    if(s_schedPending) { s_schedPending=false; Scheduler::IRQrunScheduler(); }

    HD2_CRUMB_STAMP(disp);  //hard-lock forensics: dispatcher ran to completion
                            //(disp_tick older than wake_tick post-mortem = the
                            //final wake's dispatch never finished)

    //Record the resume frame CTX_RESTORE is about to consume (the rte target):
    //the chosen thread's saved SP (ctxsave[0][0]) and its frame's EPC/EPSR
    //words (cskyv2_context.S layout: EPC @+0x3c, EPSR @+0x40).  Post-mortem,
    //the final dispatch's record is the EXACT context that never ran again.
    {
        volatile unsigned int *tsp=ctxsave[0];
        unsigned int sp=tsp[0];
        HD2_CRUMB->res_sp  =sp;
        HD2_CRUMB->res_epc =*reinterpret_cast<volatile unsigned int*>(sp+0x3cu);
        HD2_CRUMB->res_epsr=*reinterpret_cast<volatile unsigned int*>(sp+0x40u);
    }
}

//-----------------------------------------------------------------------------
// interfaces/interrupts.h contract
//-----------------------------------------------------------------------------

void IRQinitIrqTable() noexcept
{
    //Hard-lock forensics (hd2_crumb.h): capture the PIC pending/mask state of
    //the PREVIOUS life into its crumb block BEFORE wiping the PIC below --
    //IRQbspInit then snapshots the whole block.  Post-mortem this shows
    //whether a timer IRQ (src1=TIME ovf, src2=WAKE) was pending-but-
    //undelivered when the watchdog fired.
    {
        volatile hd2_crumb_t *crumb=(volatile hd2_crumb_t*)HD2_CRUMB_BASE;
        crumb->pic_snap=( HD2_PIC_INT_ST       & 0xffu)
                      | ((HD2_PIC_MASK   & 0xffu) << 8)
                      | ((HD2_PIC_INT_ST1 & 0xffu) << 16)
                      | ((HD2_PIC_MASK1  & 0xffu) << 24);
    }

    //All vectors -> unexpectedIrq, except trap 0 (yield) and the PIC range.
    for(unsigned int v=0; v<HD2_VBR_NVEC; v++)
        g_vbrTable[v]=reinterpret_cast<unsigned int>(&unexpectedIrq);
    g_vbrTable[HD2_YIELD_VEC]=reinterpret_cast<unsigned int>(&yield_isr_entry);
    for(unsigned int src=0; src<NUM_PIC_SOURCES; src++)
        g_vbrTable[HD2_PIC_VECTOR0+src]=reinterpret_cast<unsigned int>(&generic_irq_entry);

    for(unsigned int i=0; i<NUM_PIC_SOURCES; i++) g_irqSlots[i]=IrqSlot{nullptr,nullptr};

    //Install VBR (cr<1,0>).
    csky_set_vbr(g_vbrTable);

    //PIC: level-triggered, active-high, all pending cleared, ALL masked. The
    //timer/peripheral sources are unmasked by IRQregisterIrqOnCore(). IE stays
    //OFF here — IRQportableStartKernel() enables it for the first switch.
    HD2_PIC_MODE   =0u;
    HD2_PIC_PO     =0xFFFFFFFFu;
    HD2_PIC_MODE1  =0u;
    HD2_PIC_PO1    =0xFFFFFFFFu;
    HD2_PIC_INT_ST =0xFFFFFFFFu;
    HD2_PIC_INT_ST1=0xFFFFFFFFu;
    HD2_PIC_MASK   =0xFFFFFFFFu;
    HD2_PIC_MASK1  =0xFFFFFFFFu;

    //BOOT TRACE (Phase 1): GREEN off + RED on = IRQinitIrqTable completed
    //(VBR installed + PIC configured). Preserves other GPIOB bits (PWR_HOLD).
    {
        volatile unsigned int *bdr=reinterpret_cast<volatile unsigned int*>(0x14100000u);
        *bdr=(*bdr & ~(1u<<0)) | (1u<<1);
    }
}

void IRQregisterIrqOnCore(GlobalIrqLock&, unsigned char /*coreId*/, unsigned int id,
                          void (*handler)(void*), void *arg) noexcept
{
    if(id>=NUM_PIC_SOURCES) return;
    g_irqSlots[id]=IrqSlot{handler,arg};
    //Unmask the PIC source (bit=0 enables).
    if(id<32) HD2_PIC_MASK  &= ~(1u<<id);
    else      HD2_PIC_MASK1 &= ~(1u<<(id-32));
}

void IRQunregisterIrqOnCore(GlobalIrqLock&, unsigned char /*coreId*/, unsigned int id,
                            void (*/*handler*/)(void*), void */*arg*/) noexcept
{
    if(id>=NUM_PIC_SOURCES) return;
    //Mask the PIC source first, then drop the handler.
    if(id<32) HD2_PIC_MASK  |= (1u<<id);
    else      HD2_PIC_MASK1 |= (1u<<(id-32));
    g_irqSlots[id]=IrqSlot{nullptr,nullptr};
}

bool IRQisIrqRegistered(unsigned int id) noexcept
{
    if(id>=NUM_PIC_SOURCES) return false;
    return g_irqSlots[id].fn!=nullptr;
}

// Lock-free IRQ registration for EARLY BOOT only (single-threaded, IE off):
// bind a handler + unmask its PIC source, without constructing a GlobalIrqLock.
// Used by IRQosTimerInit, which runs before the kernel is started where the
// GlobalIrqLock machinery may not yet be usable.
void hd2RegisterIrqNoLock(unsigned int id, void (*handler)(void*), void *arg) noexcept
{
    if(id>=NUM_PIC_SOURCES) return;
    g_irqSlots[id]=IrqSlot{handler,arg};
    if(id<32) HD2_PIC_MASK  &= ~(1u<<id);
    else      HD2_PIC_MASK1 &= ~(1u<<(id-32));
}

// Non-SMP single-core convenience overloads: interfaces/interrupts.h declares
// (but does not inline) these when WITH_SMP is off — forward to the OnCore
// variant with coreId 0. (Default arg lives in the header declaration.)
void IRQregisterIrq(GlobalIrqLock& lock, unsigned int id,
                    void (*handler)(void*), void *arg) noexcept
{
    IRQregisterIrqOnCore(lock,0,id,handler,arg);
}

void IRQunregisterIrq(GlobalIrqLock& lock, unsigned int id,
                      void (*handler)(void*), void *arg) noexcept
{
    IRQunregisterIrqOnCore(lock,0,id,handler,arg);
}

} //namespace miosix
