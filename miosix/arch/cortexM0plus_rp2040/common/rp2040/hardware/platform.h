/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** \file platform.h
 * Macros and definitions for the RP2 family device / architecture which provide
 * abstractions over low level platform specifics.
 */

#ifndef _HARDWARE_PLATFORM_H
#define _HARDWARE_PLATFORM_H

#include <CMSIS/Include/cmsis_compiler.h>
#include "types.h"
#include "regs/addressmap.h"
#include "regs/sio.h"
#include "regs/sysinfo.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Untyped conversion alias pointer generation macros
#define hw_set_alias_untyped(addr) ((void *)(REG_ALIAS_SET_BITS | ((uintptr_t)addr)))
#define hw_clear_alias_untyped(addr) ((void *)(REG_ALIAS_CLR_BITS | ((uintptr_t)addr)))
#define hw_xor_alias_untyped(addr) ((void *)(REG_ALIAS_XOR_BITS | ((uintptr_t)addr)))

// Typed conversion alias pointer generation macros
#define hw_set_alias(p) ((typeof(p))hw_set_alias_untyped(p))
#define hw_clear_alias(p) ((typeof(p))hw_clear_alias_untyped(p))
#define hw_xor_alias(p) ((typeof(p))hw_xor_alias_untyped(p))

/*! \brief Atomically set the specified bits to 1 in a HW register
 *  \ingroup hardware_base
 *
 * \param addr Address of writable register
 * \param mask Bit-mask specifying bits to set
 */
__STATIC_FORCEINLINE void hw_set_bits(io_rw_32 *addr, uint32_t mask) {
    *(io_rw_32 *) hw_set_alias_untyped((volatile void *) addr) = mask;
}

/*! \brief Atomically clear the specified bits to 0 in a HW register
 *  \ingroup hardware_base
 *
 * \param addr Address of writable register
 * \param mask Bit-mask specifying bits to clear
 */
__STATIC_FORCEINLINE void hw_clear_bits(io_rw_32 *addr, uint32_t mask) {
    *(io_rw_32 *) hw_clear_alias_untyped((volatile void *) addr) = mask;
}

/*! \brief Atomically flip the specified bits in a HW register
 *  \ingroup hardware_base
 *
 * \param addr Address of writable register
 * \param mask Bit-mask specifying bits to invert
 */
__STATIC_FORCEINLINE void hw_xor_bits(io_rw_32 *addr, uint32_t mask) {
    *(io_rw_32 *) hw_xor_alias_untyped((volatile void *) addr) = mask;
}

/*! \brief Set new values for a sub-set of the bits in a HW register
 *  \ingroup hardware_base
 *
 * Sets destination bits to values specified in \p values, if and only if corresponding bit in \p write_mask is set
 *
 * Note: this method allows safe concurrent modification of *different* bits of
 * a register, but multiple concurrent access to the same bits is still unsafe.
 *
 * \param addr Address of writable register
 * \param values Bits values
 * \param write_mask Mask of bits to change
 */
__STATIC_FORCEINLINE void hw_write_masked(io_rw_32 *addr, uint32_t values, uint32_t write_mask) {
    hw_xor_bits(addr, (*addr ^ values) & write_mask);
}

/*! \brief Reset the specified HW blocks
 *  \ingroup hardware_resets
 *
 * \param bits Bit pattern indicating blocks to reset. See \ref reset_bitmask
 */
__STATIC_FORCEINLINE void reset_block(uint32_t bits) {
    hw_set_bits(&resets_hw->reset, bits);
}

/*! \brief Bring specified HW blocks out of reset and wait for completion
 *  \ingroup hardware_resets
 *
 * \param bits Bit pattern indicating blocks to unreset. See \ref reset_bitmask
 */
__STATIC_FORCEINLINE void unreset_block_wait(uint32_t bits) {
    hw_clear_bits(&resets_hw->reset, bits);
    while (~resets_hw->reset_done & bits) {}
}

/*! \brief Returns the RP2040 chip revision number
 *  \ingroup pico_platform
 * @return the RP2040 chip revision number (1 for B0/B1, 2 for B2)
 */
static inline uint8_t rp2040_chip_version(void) {
    // First register of sysinfo is chip id
    uint32_t chip_id = *((io_ro_32*)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET));
    // Version 1 == B0/B1
    unsigned int version = (chip_id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
    return (uint8_t)version;
}

/*! \brief Returns the RP2040 rom version number
 *  \ingroup pico_platform
 * @return the RP2040 rom version number (1 for RP2040-B0, 2 for RP2040-B1, 3 for RP2040-B2)
 */
__STATIC_FORCEINLINE uint8_t rp2040_rom_version(void) {
    return *(uint8_t*)0x13;
}

/*! \brief Get the current core number
 *  \ingroup pico_platform
 *
 * \return The core number the call was made from
 */
__STATIC_FORCEINLINE unsigned int get_core_num(void) {
    return (*(uint32_t *) (SIO_BASE + SIO_CPUID_OFFSET));
}

#ifdef __cplusplus
}
#endif

#endif
