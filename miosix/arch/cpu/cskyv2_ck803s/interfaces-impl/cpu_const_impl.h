/***************************************************************************
 *   CK803S (C-SKY V2) CPU constants for modern Miosix.                     *
 *   GPL v2+ with the Miosix linking exception (see armv6m cpu_const_impl.h)*
 ***************************************************************************/

#pragma once

namespace miosix {

/**
 * \addtogroup Interfaces
 * \{
 */

/// \internal Size in words of the per-thread ctxsave[] array.
/// CK803S has NO hardware register auto-stacking on exception entry (the core
/// saves only PC->EPC and PSR->EPSR), so — unlike armv6m which splits context
/// between ctxsave (sp+r4-r11) and a HW-pushed stack frame — our port pushes
/// the FULL register frame onto the interrupted thread's own stack and stores
/// only the thread SP in ctxsave[0]. ctxsave[1] reserved (keeps the array
/// 8-byte sized). This is the design validated in tmp/miosix_phase1_draft.
const unsigned char CTXSAVE_SIZE=2;

/// \internal Bytes the context save pushes onto the thread's own stack.
/// = the full CK803S interrupt frame built by the cskyv2_vectors.S CTX_SAVE
/// macro, 8-byte aligned. MUST equal FRAME_SIZE in the entry asm and the word
/// layout in initKernelThreadCtxsave (cpu.cpp). Divisible by 4.
///
/// CK803 implements only the 16-register C-SKY V2 base file (r0-r15) — VERIFIED:
/// `csky-miosix-elf-gcc -mcpu=ck803 -O2`, even forced to spill 24 volatiles,
/// never allocates above r15 (push l0/lr, temps t0/t1, sp=r14, lr=r15). The
/// hardware-proven OpenRTX irq_test.S timer ISR likewise saves only r0-r13+r15.
/// So the FULL preserved set across an arbitrary-point preemption is:
///   r0-r13 (stm, 14 words) + r15/lr (1) + EPC (1) + EPSR (1) = 17 words = 68B,
/// padded to 72 (8-byte ABI stack alignment). sp(r14) is NOT in the frame — it
/// is the frame pointer itself, published to ctxsave[0]. ck803 has no GBR/r28.
const unsigned int CTXSAVE_ON_STACK=72;    // 18 words: r0-r13, r15, EPC, EPSR, pad

/// \internal Stack alignment required by the C-SKY V2 ABI.
const unsigned int CTXSAVE_STACK_ALIGNMENT=8;

/// \internal Offset in words to retrieve the thread stack pointer in ctxsave.
const unsigned int STACK_OFFSET_IN_CTXSAVE=0;

/**
 * \}
 */

} //namespace miosix
