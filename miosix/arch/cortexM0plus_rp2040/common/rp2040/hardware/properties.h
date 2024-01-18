/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** \name properties.h
 * Header which defines some properties of the hardware available in the RP2040.
 */

#ifndef _HARDWARE_PROPERTIES_H
#define _HARDWARE_PROPERTIES_H

#include "types.h"

#define NUM_CORES _u(2)
#define NUM_DMA_CHANNELS _u(12)
#define NUM_DMA_TIMERS _u(4)
#define NUM_IRQS _u(32)
#define NUM_USER_IRQS _u(6)
#define NUM_PIOS _u(2)
#define NUM_PIO_STATE_MACHINES _u(4)
#define NUM_PWM_SLICES _u(8)
#define NUM_SPIN_LOCKS _u(32)
#define NUM_UARTS _u(2)
#define NUM_I2CS _u(2)
#define NUM_SPIS _u(2)
#define NUM_TIMERS _u(4)
#define NUM_ADC_CHANNELS _u(5)

#define NUM_BANK0_GPIOS _u(30)
#define NUM_QSPI_GPIOS _u(6)

#define PIO_INSTRUCTION_COUNT _u(32)

#endif

