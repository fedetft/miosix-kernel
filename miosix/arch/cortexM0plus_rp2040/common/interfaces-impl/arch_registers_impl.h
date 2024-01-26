
#pragma once

#include "../rp2040/hardware/rp2040.h"
#include "../rp2040/hardware/properties.h"
#include "../rp2040/hardware/platform.h"
#if defined(BOARD_VARIANT_PICO)
#include "../rp2040/hardware/boards/pico.h"
#elif defined(BOARD_VARIANT_PICO_W)
#include "../rp2040/hardware/boards/pico_w.h"
#else
#error "Unknown board variant"
#endif
