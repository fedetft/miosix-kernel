#pragma once

#include "interfaces/cpu_const.h"

// Not even sure if 64 bit is supported, but makes it easy to change
typedef uint32_t WordType;

typedef struct
__attribute__ ((packed))
{
    volatile WordType FP_CTRL;
    volatile WordType FP_REMAP; // Unused
    volatile WordType FP_COMP[];
} sFPB;

#define FPB ((sFPB*)0xE0002000)

#define MAKE_MASK(shift, width) (((1 << (width)) - 1) << (shift))

//*****************************************************************************
// FP_CTRL register
//*****************************************************************************
#define FP_Ctrl_WRITE_KEY   0b10
#define FP_Ctrl_ENABLE_BIT  0x01
#define FP_Ctrl_SET_ENABLE  (FP_Ctrl_WRITE_KEY | FP_Ctrl_ENABLE_BIT)
#define FP_Ctrl_CLR_ENABLE  (FP_Ctrl_WRITE_KEY | (~FP_Ctrl_ENABLE_BIT))

#define FP_Ctrl_NUM_CODE1_SHIFT     4
#define FP_Ctrl_NUM_CODE1_WIDTH     4
#define FP_Ctrl_NUM_CODE1_MASK      MAKE_MASK(FP_Ctrl_NUM_CODE1_SHIFT, FP_Ctrl_NUM_CODE1_WIDTH)
#define FP_Ctrl_NUM_CODE2_SHIFT     12
#define FP_Ctrl_NUM_CODE2_WIDTH     3
#define FP_Ctrl_NUM_CODE2_MASK      MAKE_MASK(FP_Ctrl_NUM_CODE2_SHIFT, FP_Ctrl_NUM_CODE2_WIDTH)

#define FP_Ctrl_NUM_LIT_SHFT        8
#define FP_Ctrl_NUM_LIT_WIDT        4
#define FP_Ctrl_NUM_LIT_MASK        MAKE_MASK(FP_Ctrl_NUM_LIT_SHFT, FP_Ctrl_NUM_LIT_WIDT)

#define FP_Ctrl_REV_SHIFT           28
#define FP_Ctrl_REV_WIDTH           4
#define FP_Ctrl_REV_MASK            MAKE_MASK(FP_Ctrl_REV_SHIFT, FP_Ctrl_REV_WIDTH)

#define FP_Ctrl_REVISION_1          (0x00 << FP_Ctrl_REV_SHIFT)
#define FP_Ctrl_REVISION_2          (0x01 << FP_Ctrl_REV_SHIFT)

//*****************************************************************************
// FP_COMP[n] register
//*****************************************************************************

// Revision 1 provides access to FP_REPLACE

#define FP_Comp_REPLACE_SHIFT       30
#define FP_Comp_REPLACE_WIDTH       2
#define FP_Comp_REPLACE_MASK        MAKE_MASK(FP_Comp_REPLACE_SHIFT, FP_Comp_REPLACE_WIDTH)

#define FP_Comp_COMP_SHIFT          2
#define FP_Comp_COMP_WIDTH          27
#define FP_Comp_COMP_MASK           MAKE_MASK(FP_Comp_COMP_SHIFT, FP_Comp_COMP_WIDTH)

#define FP_Comp_ENABLE_SHIFT        0
#define FP_Comp_ENABLE_WIDTH        1
#define FP_Comp_ENABLE_MASK         MAKE_MASK(FP_Comp_ENABLE_SHIFT, FP_Comp_ENABLE_WIDTH)

#define FP_Comp_WRITE_MASK          (FP_Comp_REPLACE_MASK | FP_Comp_COMP_MASK | FP_Comp_ENABLE_MASK)

#define FP_Comp_Mode_REMAP          0
#define FP_Comp_Mode_BKPT_AT_00     1
#define FP_Comp_Mode_BKPT_AT_10     2
#define FP_Comp_Mode_BKPT_AT_X0     3

// Revision 2 Without FlashPatch support

#define FP_Comp_BPADDR_SHIFT        1
#define FP_Comp_BPADDR_WIDTH        1
#define FP_Comp_BPADDR_MASK         MAKE_MASK(FP_Comp_BPADDR_SHIFT, FP_Comp_BPADDR_WIDTH)

#define FP_Comp_BE                  0x01

// Revision 2 With FlashPatch support, Flashpatch not used, only providing Enable/disable

#define FP_Comp_FE_SHIFT            31
#define FP_Comp_FE_WIDTH            1
#define FP_Comp_FE_MASK             MAKE_MASK(FP_Comp_FE_SHIFT, FP_Comp_FE_WIDTH)

#define FP_Comp_FE                  FP_Comp_FE_MASK

// Base implementation has no numeral watchpoints
typedef struct
{
    // Just padding
    volatile uint32_t __PADDING[8U];

    struct {
        volatile uint32_t COMP;
        volatile uint32_t MASK;
        volatile uint32_t FUNCTION;
        volatile uint32_t __RESERVED[1U];
    } WP[];
} DWT_Type2;

typedef unsigned int (*U3int)[3];

#define _DWT    ((DWT_Type2*)     DWT_BASE)     /*!< DWT configuration struct */

//*****************************************************************************
// FP_COMP[n] register
//*****************************************************************************

// TODO: Are these functions dependent on architecture or can I include them in debugger/breakpoints.cpp?
namespace miosix {

/**
 * @brief Enable FlashPatchUnit and Breakpoint unit (core specific)
 */
inline void flashPatchEnable() {
    FPB->FP_CTRL = FP_Ctrl_SET_ENABLE;
}

/**
 * @brief Disable FlashPatchUnit and Breakpoint unit (core specific)
 */
inline void flashPatchDisable() {
    // NOTE: Not &= as "key" is RAZ
    FPB->FP_CTRL = FP_Ctrl_WRITE_KEY;
}

inline void debugMonitorEnable() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_MON_EN_Msk;
}

/**
 * @brief Enable DebugMonitor & trace (core specific)
 */
inline void debugTraceEnable() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
}

/**
 * @brief Disable DebugMonitor & trace (core specific)
 */
inline void debugTraceDisable() {
    // Apparently not enough, need to also disable all DWT features
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
}

/**
 * @brief Clear pending state (core specific)
 */
inline void debugMonitorPendClear() {
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_MON_PEND_Msk;
}

/**
 * @brief Clear pending state (core specific)
 */
inline void debugMonitorPendSet() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_MON_PEND_Msk;
}

/**
 * @brief Reads debug pending state (core specific)
 *
 * @return 0 if not pending != 0 otherwise
 */
inline unsigned int debugMonitorPendGet() {
    return (CoreDebug->DEMCR & CoreDebug_DEMCR_MON_PEND_Msk) >> CoreDebug_DEMCR_MON_PEND_Pos;
}

/**
 * @brief Enable DebugMonitor stepping (core specific)
 */
inline void debugMonitorSteppingEnable() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_MON_STEP_Msk;
}

/**
 * @brief Disable DebugMonitor stepping (core specific)
 */
inline void debugMonitorSteppingDisable() {
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_MON_STEP_Msk;
}

////////////////////////////////////////////////////////////////////////////////
/// TODO: These values are constants for a speific board, consider switching to #def

/**
 * @brief Returns therevision version of implemented fpb
 */
inline unsigned int fpbGetRevisionVersion() {
    return FPB->FP_CTRL & FP_Ctrl_REV_MASK;
}

/**
 * @brief Returns breakpoint writemask for the architecture (maybe can be defined)
 */
inline unsigned int fpbGetWriteMask() {
    if (fpbGetRevisionVersion() == FP_Ctrl_REVISION_1)
        return FP_Comp_WRITE_MASK;
    else
        return 0xffffffff;
}

inline bool validBPUAddress(unsigned int address) {
    // Do not accept odd addresses (instructions are 2-bytes aligned)
    if(address & 0x1) return false;

    // Rev.2 supports whole address space
    if (fpbGetRevisionVersion() == FP_Ctrl_REVISION_2) return true;

    // Rev.1 supports up to RAM address (0x20000000, excluded)
    return address == (address & (FP_Comp_COMP_MASK | 0x2));
}

/**
 * @brief Get the number of available physical address comparators
 *
 * @return
 */
inline int fpbGetAvailableBreakpoints() {
    return (((FPB->FP_CTRL & FP_Ctrl_NUM_CODE1_MASK) >> FP_Ctrl_NUM_CODE1_SHIFT) 
         | (((FPB->FP_CTRL & FP_Ctrl_NUM_CODE2_MASK) >> FP_Ctrl_NUM_CODE2_SHIFT) << FP_Ctrl_NUM_CODE1_WIDTH));
}

/**
 * @brief Get the number of available physical literal comparators
 *
 * @return 
 */
inline int fpbGetAvailableWatchpoints() {
    return (DWT->CTRL & DWT_CTRL_NUMCOMP_Msk) >> DWT_CTRL_NUMCOMP_Pos;
}

// Smaller masks may be supported, in which case probe the maximum supported mask
inline unsigned int fpbGetSupportedWatchpointMask() {
    DWT->MASK0 = DWT_MASK_MASK_Msk;
    return DWT->MASK0 & DWT_MASK_MASK_Msk;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * @typedef 
 * @brief Define watchpoint as the code they use in the cpu registers
 *
 */
typedef enum : unsigned int {
    NONE    = 0b0000,
    READ    = 0b0101,
    WRITE   = 0b0110,
    ACCESS  = 0b0111,
} WatchpointType;

/**
 * @brief Disable specified watchpoint for local cpu
 *
 * @param id
 */
inline void clearLocalWatchpoint(int id) {
    _DWT->WP[id].FUNCTION = NONE;
}

}
