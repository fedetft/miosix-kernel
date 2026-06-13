/***************************************************************************
 *   CK803S (C-SKY V2) interrupt enable/disable for modern Miosix.          *
 *   GPL v2+ with the Miosix linking exception (see armv6m interrupts_impl.h)*
 ***************************************************************************/

#pragma once

#include "interfaces/arch_registers.h"

namespace miosix {

/**
 * \addtogroup Interfaces
 * \{
 */

/// CK803S PIC (0x17000000) has 64 maskable sources, gated per-source via the
/// PIC mask register rather than per-IRQ priority levels like ARM NVIC. Miosix's
/// IRQ-priority concept maps weakly here; expose nominal values. (If the kernel
/// references these for a priority-based scheme, revisit when wiring the PIC.)
constexpr int defaultIrqPriority=0;
constexpr int minimumIrqPriority=0;

inline void fastDisableIrq() noexcept
{
    // Clear PSR.IE (maskable interrupt enable). Memory barrier so the compiler
    // doesn't reorder across the critical-section boundary.
    asm volatile("psrclr ie":::"memory");
}

inline void fastEnableIrq() noexcept
{
    asm volatile("psrset ie":::"memory");
}

inline bool areInterruptsEnabled() noexcept
{
    unsigned int psr;
    asm volatile("mfcr %0, psr":"=r"(psr));
    // PSR.IE = bit 6 on CK803S (standard C-SKY V2 layout; verify on silicon).
    return (psr & (1u<<6)) != 0u;
}

/**
 * \}
 */

} //namespace miosix
