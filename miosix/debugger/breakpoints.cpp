#include "breakpoints.h"

#include "debug_registers.h"
#include "debugger_interface.h"

namespace miosix {

unsigned int BreakpointUnit::dirty      = 0xffffffff;
int BreakpointUnit::breakpointsNum      = fpbGetAvailableBreakpoints();
int BreakpointUnit::watchpointsNum      = fpbGetAvailableWatchpoints();
Breakpoint* BreakpointUnit::breakpoints = new Breakpoint[fpbGetAvailableBreakpoints()];
Watchpoint* BreakpointUnit::watchpoints = new Watchpoint[fpbGetAvailableWatchpoints()];

const unsigned int BreakpointUnit::mask         = fpbGetSupportedWatchpointMask(),
                   BreakpointUnit::revision     = fpbGetRevisionVersion(),
                   BreakpointUnit::writeMask    = fpbGetWriteMask();

SoftBreakpoint::SoftBreakpoint(unsigned int address, unsigned int kind) {
    this->address = address;
    this->kind = kind;
}

void SoftBreakpoint::applyPatch() {
    if (kind == 2) {
        const auto addr = reinterpret_cast<unsigned short*>(address);
        istr = *addr;
        *addr = BREAKPOINT_INSTRUCTION_2;
    } else {
        const auto addr = reinterpret_cast<unsigned int*>(address);
        istr = *addr;
        *addr = BREAKPOINT_INSTRUCTION_4;
    }
}

void SoftBreakpoint::removePatch() {
    if (kind == 2) {
        const auto addr = reinterpret_cast<unsigned short*>(address);
        *addr = istr;
    } else {
        const auto addr = reinterpret_cast<unsigned int*>(address);
        *addr = istr;
    }
    clear();
}

bool SoftBreakpoint::eq (const SoftBreakpoint& other) const {
    return address == other.address && kind == other.kind;
}

}
