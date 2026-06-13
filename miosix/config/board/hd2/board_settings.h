/***************************************************************************
 *   Board settings for the Ailunce HD2 (HR_C7000 / CK803S).                *
 *   GPL v2+ with the Miosix linking exception.                            *
 ***************************************************************************/

#pragma once

/**
 * \internal
 * Versioning for board_settings.h for out of git tree projects
 */
#define BOARD_SETTINGS_VERSION 300

namespace miosix {

/**
 * \addtogroup Settings
 * \{
 */

/// Clock frequencies after IRQmemoryAndClockInit() (project_hd2_clock_init).
/// NOTE: the authoritative OS-timer frequency is HD2_TIMER_HZ (42 MHz, measured,
/// in arch_registers_impl.h) — the os_timer and delays use that directly. These
/// constants are informational / for any driver that references them.
constexpr unsigned int peripheralFrequency=42000000; ///< DW_apb_timers input clk
constexpr unsigned int cpuFrequency=42000000;        ///< CK803S core (post-PLL)

/// Serial port baudrate (the IAP/loader UART0 runs 57600 8N1 — used once a
/// console driver is wired; project_hd2_diagboot_works).
const unsigned int SERIAL_PORT_SPEED=57600;

/// Size of stack for main(). 192 KiB of internal SRAM leaves ample room; the C
/// library is stack-heavy (iprintf ~1.5 KB), so keep a comfortable 4 KB.
const unsigned int MAIN_STACK_SIZE=4*1024;

/// \def OS_TIMER_MODEL_UNIFIED
/// The HD2 uses the unified single-core timer model: the custom os_timer
/// (hr_c7000_os_timer.cpp) drives both wakeup and preemption through
/// IRQosTimerSetInterrupt(), so IRQosTimerSetPreemption() is not needed.
#define OS_TIMER_MODEL_UNIFIED

/**
 * \}
 */

} //namespace miosix
