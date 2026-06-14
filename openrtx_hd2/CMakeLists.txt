# OpenRTX threaded app for the Ailunce HD2, built on the HW-proven vendor
# Miosix (project_hd2_miosix_console_works). Scope: UI render + keyboard nav;
# radio/audio/DMR are stubbed (brought up in later phases).
cmake_minimum_required(VERSION 3.21)

set(MIOSIX_KPATH ${CMAKE_SOURCE_DIR}/../miosix)
set(CMAKE_TOOLCHAIN_FILE ${MIOSIX_KPATH}/cmake/Toolchains/gcc-csky.cmake)
set(CMAKE_BUILD_TYPE "Release")

project(OpenRtxHd2 C CXX ASM)

set(MIOSIX_BOARD hd2)
set(MIOSIX_USER_CONFIG_PATH ${MIOSIX_KPATH}/config)
add_subdirectory(${MIOSIX_KPATH} miosix EXCLUDE_FROM_ALL)

# Sibling OpenRTX checkout (vendor/OpenRTX).
set(ORTX ${CMAKE_SOURCE_DIR}/../../OpenRTX)

set(ORTX_CORE
    ${ORTX}/openrtx/src/core/state.c
    ${ORTX}/openrtx/src/core/threads.c
    ${ORTX}/openrtx/src/core/graphics.c
    ${ORTX}/openrtx/src/core/input.c
    ${ORTX}/openrtx/src/core/utils.c
    ${ORTX}/openrtx/src/core/queue.c
    ${ORTX}/openrtx/src/core/chan.c
    ${ORTX}/openrtx/src/core/gps.c
    ${ORTX}/openrtx/src/core/cps.c
    ${ORTX}/openrtx/src/core/crc.c
    ${ORTX}/openrtx/src/core/datetime.c
    ${ORTX}/openrtx/src/core/openrtx.c
    ${ORTX}/openrtx/src/core/data_conversion.c
    ${ORTX}/openrtx/src/core/nvmem_access.c
    ${ORTX}/openrtx/src/core/audio_path.cpp
    ${ORTX}/openrtx/src/core/audio_stream.c
    ${ORTX}/openrtx/src/core/voicePromptUtils.c)

set(ORTX_UI
    ${ORTX}/openrtx/src/ui/default/ui.c
    ${ORTX}/openrtx/src/ui/default/ui_main.c
    ${ORTX}/openrtx/src/ui/default/ui_menu.c
    ${ORTX}/openrtx/src/ui/default/ui_strings.c)

# Real HD2 drivers (reused as-is from the superloop bring-up).
set(ORTX_HD2
    ${ORTX}/platform/targets/HD2/platform.c
    ${ORTX}/platform/targets/HD2/hwconfig.c
    ${ORTX}/platform/drivers/display/ST7735S_HD2.c
    ${ORTX}/platform/drivers/keyboard/keyboard_HD2.c
    ${ORTX}/platform/drivers/backlight/backlight_HD2.c
    ${ORTX}/platform/drivers/GPS/gps_HD2.c
    ${ORTX}/platform/drivers/GPS/nmea_rbuf.c
    ${ORTX}/platform/mcu/CSKY_V2/drivers/gpio_csky.c
    ${ORTX}/platform/mcu/CSKY_V2/drivers/i2c_csky.c
    ${ORTX}/platform/mcu/CSKY_V2/drivers/adc_hd2.c
    ${ORTX}/platform/mcu/CSKY_V2/drivers/rtc_hd2.c
    ${ORTX}/platform/targets/HD2/hd2_router.c
    ${ORTX}/platform/targets/HD2/hd2_vendor_import.c
    ${ORTX}/platform/targets/HD2/watchdog_HD2.c
    ${ORTX}/platform/targets/HD2/hd2_diag.cpp
    ${ORTX}/platform/targets/HD2/hd2_pcm_stream.cpp
    ${ORTX}/platform/targets/HD2/hd2_gps.cpp
    ${ORTX}/platform/targets/HD2/hd2_adpcm_sample.c
    # Portable rtx OpModes (dispatched once HD2 links rtx.cpp; compiled now to
    # verify + the HD2 radio/tuner drivers provide the strong HAL overrides).
    ${ORTX}/openrtx/src/rtx/OpMode_FM.cpp
    ${ORTX}/openrtx/src/rtx/OpMode_FMBroadcast.cpp)

# Hardware-agnostic protocol modules (openrtx/src/protocols), structured per
# OSI layers like M17.  APRS = 1200-baud AFSK + HDLC + AX.25, RX (Decoder) + TX (Encoder).
set(ORTX_PROTOCOLS
    ${ORTX}/openrtx/src/protocols/APRS/Afsk1200.cpp
    ${ORTX}/openrtx/src/protocols/APRS/Hdlc.cpp
    ${ORTX}/openrtx/src/protocols/APRS/Ax25.cpp
    ${ORTX}/openrtx/src/protocols/APRS/Decoder.cpp
    ${ORTX}/openrtx/src/protocols/APRS/Encoder.cpp)

# Persistent settings + last-VFO storage on the W25Q SPI-NOR (GPIOA
# bit-bang).  nvmem_settings_HD2.c implements the settings half of
# interfaces/nvmem.h (replaces the stubs that used to live in
# hd2_stubs.c); spi_hd2.c is the underlying bit-bang SPI bus driver.
# The codeplug half of nvmem stays stubbed (cps_io_stub.c below).
set(ORTX_NVM
    ${ORTX}/platform/drivers/SPI/spi_hd2.c
    ${ORTX}/platform/drivers/NVM/flash_w25q_HD2.c
    ${ORTX}/platform/drivers/NVM/nvmem_settings_HD2.c
    ${ORTX}/platform/drivers/NVM/nvmem_vendor_settings_HD2.c
    ${ORTX}/platform/drivers/NVM/nvmem_vendor_records_HD2.c)

# Phase C: real analog-FM RX *control* path (tune + RSSI + tone squelch), plus
# the audio_HD2 routing matrix (interfaces/audio.h; replaces audio_stub.c) which
# owns the board-level speaker-amp / RX-route GPIO and the codec/socsys audio
# gate.  hd2_router.c (in ORTX_HD2) drives the matrix + named twiddlers over the
# loader protocol.
set(ORTX_RADIO
    ${ORTX}/platform/drivers/baseband/radio_HD2.cpp
    ${ORTX}/platform/drivers/baseband/AT1846S_HD2.cpp
    ${ORTX}/platform/drivers/baseband/RDA5802E_HD2.c
    ${ORTX}/platform/drivers/audio/audio_HD2.c
    ${ORTX}/platform/drivers/audio/outputStream_HD2.cpp)

# Stubs for the not-yet-wired subsystems (UI scope).
set(ORTX_STUBS
    ${ORTX}/platform/drivers/NVM/cps_io_HD2.c)

# Vendored helper libs OpenRTX core pulls in (cf. meson openrtx_src/inc).
set(ORTX_LIBS
    ${ORTX}/lib/minmea/minmea.c
    ${ORTX}/lib/QRCode/qrcode.c)

# Voice prompts: HD2 IMA-ADPCM player (voicePrompts_HD2.c replaces the stock
# codec2-based core/voicePrompts.c -- codec2 is too slow on this soft-float
# core).  Clip data = an ADPCM container embedded via voicePromptDataHD2.S
# (.incbin voiceprompts.vpa, built by scripts/vpc_to_adpcm.py).  No codec2,
# no audio_codec.c, no dsp.cpp.
set(ORTX_VP
    ${ORTX}/platform/targets/HD2/voicePrompts_HD2.c
    ${ORTX}/platform/targets/HD2/voicePromptDataHD2.S)

# voicePromptDataHD2.S does `.incbin "voiceprompts.vpa"`; GAS resolves .incbin
# via -I (the .vpa is copied beside the .S in platform/targets/HD2).
set_source_files_properties(${ORTX}/platform/targets/HD2/voicePromptDataHD2.S
    PROPERTIES COMPILE_OPTIONS "-Wa,-I${ORTX}/platform/targets/HD2")

add_executable(openrtx_hd2
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/hd2_glue.cpp
    ${CMAKE_SOURCE_DIR}/hd2_stubs.c
    ${CMAKE_SOURCE_DIR}/hd2_rtx.c
    ${CMAKE_SOURCE_DIR}/hd2_fm_broadcast.cpp
    ${ORTX_CORE} ${ORTX_UI} ${ORTX_HD2} ${ORTX_NVM} ${ORTX_RADIO} ${ORTX_STUBS}
    ${ORTX_VP} ${ORTX_LIBS} ${ORTX_PROTOCOLS})

target_include_directories(openrtx_hd2 PRIVATE
    ${CMAKE_SOURCE_DIR}                   # fork: board-level headers (hd2_crumb.h)
    ${ORTX}/openrtx/include
    ${ORTX}/platform
    ${ORTX}/platform/targets/HD2
    ${ORTX}/platform/mcu/CSKY_V2
    ${ORTX}/lib/minmea/include
    ${ORTX}/lib/QRCode/include
    ${ORTX}/lib/qdec/include)

target_compile_definitions(openrtx_hd2 PRIVATE
    PLATFORM_HD2 _MIOSIX FONT_UBUNTU_REGULAR timegm=mktime
    "GIT_VERSION=\"openrtx-hd2-miosix\"")

# Optional debug: echo raw GPS NMEA from UART2 to the console UART0 (visible in
# the loader/diag bridge log).  Enable with:  cmake -DHD2_GPS_DEBUG=ON .
option(HD2_GPS_DEBUG "Echo raw GPS NMEA to the console UART" OFF)
if(HD2_GPS_DEBUG)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_GPS_DEBUG=1)
endif()

# Radio-bus (AT1846S/RDA5802E) I2C transport.  Default = the HW DesignWare
# I2C1 controller (2026-06-13: full Test-A checklist HW-verified on it -- RX,
# TX, PTT, codeplug, FM broadcast incl. the RDA5802E 64-byte init burst).
# The historical "wedge" was platform_init stealing the pad mux
# (docs/TASK_hw_i2c_migration.md); the post-boot hard lock was the missing
# no-PendSV drain in PKrestartKernelAndWaitImpl (same doc).  The bit-bang
# transport is kept as a fallback: cmake -DHD2_I2C_BITBANG=ON .
option(HD2_I2C_BITBANG "Use software bit-bang I2C for the radio bus (fallback; default = HW I2C1)" OFF)
if(HD2_I2C_BITBANG)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_I2C_BITBANG=1)
endif()

# LCD transport: default = the HW i8080 controller @0x12000000 (live-verified
# 2026-06-13, red-stripe test; ~6 AHB cycles/byte vs hundreds for the GPIO
# latch).  Keypad scans transiently re-mux the shared PTC pins (keyboard_HD2.c).
# Fallback: cmake -DHD2_LCD_BITBANG=ON .
option(HD2_LCD_BITBANG "Use the GPIO-latch bit-bang for the LCD (fallback; default = HW i8080)" OFF)
if(HD2_LCD_BITBANG)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_LCD_BITBANG=1)
endif()

# W25Q flash transport: default = HW SPI0 @0x140a0000 (live-verified
# 2026-06-13, JEDEC ef4020 over the controller; full-duplex mode -- the
# custom variant's EEPROM-read TMOD is broken).  CS stays GPIO-owned.
# Fallback: cmake -DHD2_SPI_BITBANG=ON .
option(HD2_SPI_BITBANG "Use software bit-bang SPI for the W25Q flash (fallback; default = HW SPI0)" OFF)
if(HD2_SPI_BITBANG)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_SPI_BITBANG=1)
endif()

# Isolation aid: stub the vendor-codeplug readers (cps_read* return -1 -> core
# uses the default codeplug, no W25Q channel enumeration at boot).  Used to
# isolate whether boot stalls / lockups come from the codeplug reader's W25Q
# traffic vs the radio-bus transport.  cmake -DHD2_CODEPLUG_STUB=ON .
option(HD2_CODEPLUG_STUB "Stub the vendor codeplug readers (no W25Q enumeration)" OFF)
if(HD2_CODEPLUG_STUB)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_CODEPLUG_STUB=1)
endif()

miosix_link_target(openrtx_hd2)

# Always refresh the build-time version stamp. hd2_diag.cpp bakes the version
# string from __DATE__/__TIME__, which the compiler only re-evaluates when that
# TU is recompiled — so when only other files change, the 'P' probe reports a
# stale build time and can't be trusted as a "did my flash take" signal. Touch
# the source before every build so its object is always rebuilt with a fresh
# timestamp. Cheap (one small TU) and keeps the reported version honest.
add_custom_target(hd2_refresh_version ALL
    COMMAND ${CMAKE_COMMAND} -E touch ${ORTX}/platform/targets/HD2/hd2_diag.cpp
    COMMENT "Refreshing version timestamp (touch hd2_diag.cpp)")
add_dependencies(openrtx_hd2 hd2_refresh_version)
