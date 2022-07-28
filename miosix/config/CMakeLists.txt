# This file contains the options required by the build system to build Miosix
# on various target architectures. All options start with OPT_ to be easily
# recognizable.
# All architecture specific build code is grouped at the end of this file.

# Target board, choose one. This also implicitly select the target
# architecture.

#set(OPT_BOARD lpc2138_miosix_board)
#set(OPT_BOARD stm32f103ze_stm3210e-eval)
#set(OPT_BOARD stm32f103ve_mp3v2)
#set(OPT_BOARD stm32f100rb_stm32vldiscovery)
#set(OPT_BOARD stm32f103ve_strive_mini)
#set(OPT_BOARD stm32f103ze_redbull_v2)
#set(OPT_BOARD stm32f407vg_stm32f4discovery)
#set(OPT_BOARD stm32f207ig_stm3220g-eval)
#set(OPT_BOARD sstm32f207zg_ethboard_v2)
#set(OPT_BOARD stm32f207ze_als_camboard)
#set(OPT_BOARD stm32l151_als_mainboard)
#set(OPT_BOARD stm32f407vg_bitsboard)
#set(OPT_BOARD stm32f205rg_sony-newman)
#set(OPT_BOARD stm32f429zi_stm32f4discovery)
#set(OPT_BOARD stm32f103cb_als_mainboard_rev2)
#set(OPT_BOARD stm32f100cb_tempsensor)
#set(OPT_BOARD stm32f429zi_oledboard2)
#set(OPT_BOARD efm32gg332f1024_wandstem)
set(OPT_BOARD stm32f411re_nucleo)
#set(OPT_BOARD stm32f429zi_skyward_anakin)
#set(OPT_BOARD stm32f100rc_solertegiard)
#set(OPT_BOARD stm32f205rc_skyward_stormtrooper)
#set(OPT_BOARD stm32f401vc_stm32f4discovery)
#set(OPT_BOARD stm32f103c8_breakout)
#set(OPT_BOARD stm32f100c8_microboard)
#set(OPT_BOARD stm32f469ni_stm32f469i-disco)
#set(OPT_BOARD stm32f429zi_skyward_homeone)
#set(OPT_BOARD stm32f401re_nucleo)
#set(OPT_BOARD stm32f746zg_nucleo)
#set(OPT_BOARD stm32h753xi_eval)
#set(OPT_BOARD stm32f407vg_thermal_test_chip)
#set(OPT_BOARD stm32f205_generic)

# Optimization flags, choose one.
# -O0 produces large and slow code, but is useful for in circuit debugging.
# -O2 is recomended otherwise, as it provides a good balance between code
# size and speed

#OPT_OPTIMIZATION := -O0
set(OPT_OPTIMIZATION -O2)
#OPT_OPTIMIZATION := -O3
#OPT_OPTIMIZATION := -Os

# C++ Exception/rtti support disable flags.
# To save code size if not using C++ exceptions (nor some STL code which
# implicitly uses it) uncomment this option.
# the -D__NO_EXCEPTIONS is used by Miosix to know if exceptions are used.

#set(OPT_EXCEPT "-fno-exceptions -fno-rtti -D__NO_EXCEPTIONS")

###############################################################################
# Board specific options
###############################################################################

if ("${OPT_BOARD}" STREQUAL "")
    message(FATAL_ERROR "No board selected")
endif()

#------------------------------------------------------------------------------
# lpc2138_miosix_board

# No options

#------------------------------------------------------------------------------
# stm32f103ze_stm3210e-eval

if (${OPT_BOARD} STREQUAL stm32f103ze_stm3210e-eval)
    # Linker script type, there are three options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    #    the most common choice, available for all microcontrollers
    # 2) Code in FLASH stack in internal RAM heap in external RAM (file
    #    *_xram.ld) useful for hardware like STM3210E-EVAL when big heap is
    #    needed. The microcontroller must have an external memory interface.
    # 3) Code + stack + heap in external RAM, (file *_all_in_xram.ld)
    #    useful for debugging code in hardware like STM3210E-EVAL. Code runs
    #    *very* slow compared to FLASH. Works only with a booloader that
    #    forwards interrrupts @ 0x68000000 (see miosix/_tools/bootloaders for
    #    one).
    #    The microcontroller must have an external memory interface.
    # Warning! enable external ram if you use a linker script that requires it
    # (see the XRAM flag below)

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM3_stm32/stm32f103ze_stm3210e-eval/)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_512k+64k_rom.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_512k+64k_xram.ld)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_512k+64k_all_in_xram.ld)

    # Enable/disable initialization of external RAM at boot. Three options:
    # __ENABLE_XRAM : If you want the heap in xram (with an appropriate linker
    # script selected above)
    # __ENABLE_XRAM and __CODE_IN_XRAM : Debug mode with code + stack + heap
    # in xram (with an appropriate linker script selected above)
    # none selected : don't use xram (with an appropriate linker script
    # selected above)

    #set(XRAM -D__ENABLE_XRAM)
    set(XRAM "-D__ENABLE_XRAM -D__CODE_IN_XRAM")

    # Select clock frequency
    # Not defining any of these results in the internal 8MHz clock to be used

    #set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=24000000)
    #set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=36000000)
    #set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=48000000)
    #set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=56000000)
    set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=72000000)
endif()

#------------------------------------------------------------------------------
# stm32f103ve_mp3v2

if (${OPT_BOARD} STREQUAL stm32f103ve_mp3v2)
    # Linker script type, there are two options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    # 2) Code + stack + heap in internal RAM (file *_ram.ld)
    # Note: this board relies on a bootloader for interrupt forwarding in ram

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM3_stm32/stm32f103ve_mp3v2/)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_512k+64k_rom.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_512k+64k_ram.ld)
endif()

#------------------------------------------------------------------------------
# stm32f103ve_strive_mini

# No options

#------------------------------------------------------------------------------
# stm32f103ze_redbull_v2

# No options

#------------------------------------------------------------------------------
# stm32f407vg_stm32f4discovery
#
if (${OPT_BOARD} STREQUAL stm32f407vg_stm32f4discovery)
    # Linker script type, there are two options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    # 2) Code + stack + heap in internal RAM (file *_ram.ld)
    # 3) Same as 1) but space has been reserved for a process pool, allowing ro
    #    configure the kernel with "#define WITH_PROCESSES"

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM4_stm32f4/stm32f407vg_stm32f4discovery/)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_1m+192k_rom.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_1m+192k_ram.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_1m+192k_rom_processes.ld)

    # This causes the interrupt vector table to be relocated in SRAM, must be
    # uncommented when using the ram linker script

    #set(SRAM_BOOT -DVECT_TAB_SRAM)

    # Select clock frequency (HSE_VALUE is the xtal on board, fixed)

    set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000")
    #set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000")
    #set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000")
endif()

#------------------------------------------------------------------------------
# stm32f207ig_stm3220g-eval

if (${OPT_BOARD} STREQUAL stm32f207ig_stm3220g-eval)
    # Linker script type, there are three options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    #    the most common choice, available for all microcontrollers
    # 2) Code in FLASH stack in internal RAM heap in external RAM (file
    #    *_xram.ld) useful for hardware like STM3220G-EVAL when big heap is
    #    needed. The microcontroller must have an external memory interface.
    # 3) Code + stack + heap in external RAM, (file *_all_in_xram.ld)
    #    useful for debugging code in hardware like STM3220G-EVAL. Code runs
    #    *very* slow compared to FLASH. Works only with a booloader that
    #    forwards interrrupts @ 0x64000000 (see miosix/_tools/bootloaders for
    #    one).
    #    The microcontroller must have an external memory interface.
    # 4) Same as 3) but space has been reserved for a process pool, allowing
    #    to configure the kernel with "#define WITH_PROCESSES"
    # Warning! enable external ram if you use a linker script that requires it
    # (see the XRAM flag below)

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM3_stm32f2/stm32f207ig_stm3220g-eval/)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_1m+128k_rom.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_1m+128k_xram.ld)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_1m+128k_all_in_xram.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_1m+128k_all_in_xram_processes.ld)

    # Enable/disable initialization of external RAM at boot. Three options:
    # __ENABLE_XRAM : If you want the heap in xram (with an appropriate linker
    # script selected above)
    # __ENABLE_XRAM and __CODE_IN_XRAM : Debug mode with code + stack + heap
    # in xram (with an appropriate linker script selected above)
    # none selected : don't use xram (with an appropriate linker script
    # selected above)

    #set(XRAM -D__ENABLE_XRAM)
    set(XRAM "-D__ENABLE_XRAM -D__CODE_IN_XRAM")
endif()

#------------------------------------------------------------------------------
# stm32f207zg_ethboard_v2

if (${OPT_BOARD} STREQUAL stm32f207zg_ethboard_v2)
    # Linker script type, there are two options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    #    the most common choice, available for all microcontrollers
    # 2) Code in external RAM, stack + heap in internal RAM
    #    (file *_code_in_xram.ld) useful for debugging. Code runs
    #    *very* slow compared to FLASH. Works only with a booloader that
    #    forwards interrrupts @ 0x60000000 (see miosix/_tools/bootloaders for
    #    one).
    #    You must -D__CODE_IN_XRAM below.

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM3_stm32f2/stm32f207zg_EthBoardV2/)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_1m+128k_rom.ld)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_1m+128k_code_in_xram.ld)

    # XRAM is always enabled on this board, even if the _rom linker script
    # does not make explicit use of it.
    # Uncommenting __CODE_IN_XRAM (with an appropriate linker script selected
    # above) allows to run code from external RAM, useful for debugging

    set(XRAM -D__CODE_IN_XRAM)
endif()

#------------------------------------------------------------------------------
# stm32f205rg_sony-newman

# No options

#------------------------------------------------------------------------------
# stm32f407vg_bitsboard

# No options

#---------------------------------------------------------------------------
# stm32f429zi_stm32f4discovery

if (${OPT_BOARD} STREQUAL stm32f429zi_stm32f4discovery)
    # Linker script type, there are three options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    #    the most common choice, available for all microcontrollers
    # 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
    #    You must uncomment -D__ENABLE_XRAM below in this case.
    # 3) Code in FLASH, stack + heap in external RAM (file *6m_xram.ld)
    #    Same as above, but leaves the upper 2MB of RAM for the LCD.

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM4_stm32f4/stm32f429zi_stm32f4discovery/)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+256k_rom.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+8m_xram.ld)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+6m_xram.ld)

    # Uncommenting __ENABLE_XRAM enables the initialization of the external
    # 8MB SDRAM memory. Do not uncomment this even if you don't use a linker
    # script that requires it, as it is used for the LCD framebuffer.

    set(XRAM -D__ENABLE_XRAM)

    # Select clock frequency. Warning: the default clock frequency for
    # this board is 168MHz and not 180MHz because, due to a limitation in
    # the PLL, it is not possible to generate a precise 48MHz output when
    # running the core at 180MHz. If 180MHz is chosen the USB peripheral will
    # NOT WORK and the SDIO and RNG will run ~6% slower (45MHz insteand of 48)

    #set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_180MHz=180000000")
    set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000")
    #set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000")
endif()

#------------------------------------------------------------------------------
# stm32f103cb_als_mainboard_rev2

# No options

#------------------------------------------------------------------------------
# stm32f100cb_tempsensor

# No options

#------------------------------------------------------------------------------
# stm32f429zi_oledboard2

if (${OPT_BOARD} STREQUAL stm32f429zi_oledboard2)
    # Linker script type, there are three options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    #    the most common choice, available for all microcontrollers
    # 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
    #    You must uncomment -D__ENABLE_XRAM below in this case.
    # 3) Code in FLASH, stack + heap in external RAM (file *6m_xram.ld)
    #    Same as above, but leaves the upper 2MB of RAM for the LCD.

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM4_stm32f4/stm32f429zi_oledboard2/)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+256k_rom.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+8m_xram.ld)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+6m_xram.ld)

    # Uncommenting __ENABLE_XRAM enables the initialization of the external
    # 8MB SDRAM memory. Do not uncomment this even if you don't use a linker
    # script that requires it, as it is used for the LCD framebuffer.

    set(XRAM -D__ENABLE_XRAM)

    # Select clock frequency. Warning: the default clock frequency for
    # this board is 168MHz and not 180MHz because, due to a limitation in
    # the PLL, it is not possible to generate a precise 48MHz output when
    # running the core at 180MHz. If 180MHz is chosen the USB peripheral will
    # NOT WORK and the SDIO and RNG will run ~6% slower (45MHz insteand of 48)

    #set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000")
    set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_180MHz=180000000")
endif()

#------------------------------------------------------------------------------
# efm32gg332f1024_wandstem

# No options

#------------------------------------------------------------------------------
# stm32f411re_nucleo

if (${OPT_BOARD} STREQUAL stm32f411re_nucleo)
    # Select clock frequency

    set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000")
    #set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000")
endif()

#------------------------------------------------------------------------------
# stm32f429zi_skyward_anakin

if (${OPT_BOARD} STREQUAL stm32f429zi_skyward_anakin)
    # Linker script type, there are three options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    #    the most common choice, available for all microcontrollers
    # 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
    #    You must uncomment -D__ENABLE_XRAM below in this case.

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM4_stm32f4/stm32f429zi_skyward_anakin/)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+256k_rom.ld)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+8m_xram.ld)

    # Uncommenting __ENABLE_XRAM enables the initialization of the external
    # 8MB SDRAM memory. Do not uncomment this even if you don't use a linker
    # script that requires it, as it is used for the LCD framebuffer.

    set(XRAM -D__ENABLE_XRAM)

    # Select clock frequency.
    # Warning: due to a limitation in the PLL, it is not possible to generate
    # a precise 48MHz output when running the core at 180MHz. If 180MHz is
    # chosen the SDIO and RNG will run ~6% slower (45MHz insteand of 48)

    set(CLOCK_FREQ "-DHSE_VALUE=25000000 -DSYSCLK_FREQ_180MHz=180000000")
    #set(CLOCK_FREQ "-DHSE_VALUE=25000000 -DSYSCLK_FREQ_168MHz=168000000")
endif()

#------------------------------------------------------------------------------
# stm32f401vc_stm32f4discovery

# No options

#------------------------------------------------------------------------------
# stm32f103c8_breakout

if (${OPT_BOARD} STREQUAL stm32f103c8_breakout)
    # Linker script type, there are two options:
    # 1) Code in FLASH, stack + heap in RAM
    # 2) Code in FLASH, stack + heap in RAM flashing with bootloader

    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_64k+20k_bootloader.ld)

    ## Select clock frequency
    set(CLOCK_FREQ -DSYSCLK_FREQ_24MHz=24000000)
    #set(CLOCK_FREQ -DSYSCLK_FREQ_36MHz=36000000)
    #set(CLOCK_FREQ -DSYSCLK_FREQ_48MHz=48000000)
    #set(CLOCK_FREQ -DSYSCLK_FREQ_56MHz=56000000)
    #set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=72000000)
endif()

#------------------------------------------------------------------------------
# stm32f100c8_microboard

# No options

#------------------------------------------------------------------------------
# stm32f469ni_stm32f469i-disco

if (${OPT_BOARD} STREQUAL stm32f469ni_stm32f469i-disco)
    # Linker script type, there are three options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    #    the most common choice, available for all microcontrollers
    # 2) Code in FLASH, stack + heap in external RAM (file *16m_xram.ld)
    #    You must uncomment -D__ENABLE_XRAM below in this case.
    # 3) Code in FLASH, stack + heap in external RAM (file *12m_xram.ld)
    #    Same as above, but leaves the upper 4MB of RAM for the LCD.

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM4_stm32f4/stm32f469ni_stm32f469i-disco/)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+384k_rom.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+16m_xram.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+12m_xram.ld)

    # Uncommenting __ENABLE_XRAM enables the initialization of the external
    # 16MB SDRAM memory. Do not uncomment this even if you don't use a linker
    # script that requires it, as it is used for the LCD framebuffer.

    set(XRAM -D__ENABLE_XRAM)

    # Select clock frequency. Warning: the default clock frequency for
    # this board is 168MHz and not 180MHz because, due to a limitation in
    # the PLL, it is not possible to generate a precise 48MHz output when
    # running the core at 180MHz. If 180MHz is chosen the USB peripheral will
    # NOT WORK and the SDIO and RNG will run ~6% slower (45MHz insteand of 48)

    #set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_180MHz=180000000")
    set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000")
    #set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000")
endif()

#------------------------------------------------------------------------------
# stm32f429zi_skyward_homeone

if (${OPT_BOARD} STREQUAL stm32f429zi_skyward_homeone)
    # Linker script type, there are three options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    #    the most common choice, available for all microcontrollers
    # 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
    #    You must uncomment -D__ENABLE_XRAM below in this case.

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM4_stm32f4/stm32f429zi_skyward_homeone/)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+256k_rom.ld)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+8m_xram.ld)

    # Uncommenting __ENABLE_XRAM enables the initialization of the external
    # 8MB SDRAM memory.

    set(XRAM -D__ENABLE_XRAM)

    # Select clock frequency. Warning: the default clock frequency for
    # this board is 168MHz and not 180MHz because, due to a limitation in
    # the PLL, it is not possible to generate a precise 48MHz output when
    # running the core at 180MHz. If 180MHz is chosen the USB peripheral will
    # NOT WORK and the SDIO and RNG will run ~6% slower (45MHz insteand of 48)

    #set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_180MHz=180000000")
    set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000")
    #set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000")
endif()

#------------------------------------------------------------------------------
# stm32f401re_nucleo

# No options

#------------------------------------------------------------------------------
# stm32f746zg_nucleo

# No options

#------------------------------------------------------------------------------
# stm32h753xi_eval

if (${OPT_BOARD} STREQUAL stm32h753xi_eval)
    # Linker script type, there are three options:
    # 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    #    the most common choice, available for all microcontrollers
    # 2) Code in FLASH, stack + heap in external RAM (file *m_xram.ld)
    #    You must uncomment -D__ENABLE_XRAM below in this case.

    set(LINKER_SCRIPT_PATH ${KPATH}/arch/cortexM7_stm32h7/stm32h753xi_eval/)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+512k_rom.ld)
    #set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_2m+32m_xram.ld)
endif()

#------------------------------------------------------------------------------
# stm32f407vg_thermal_test_chip

# No options

#------------------------------------------------------------------------------
# stm32f205_generic

# No options

###############################################################################
#   From the options selected above, now fill all the variables needed to     #
#   build Miosix. You should modify something here only if you are adding     #
#   a new board or porting Miosix to a new architecture                       #
###############################################################################

# First, auto guess architecture name from board name

if (${OPT_BOARD} STREQUAL lpc2138_miosix_board)
    set(ARCH arm7_lpc2000)
elseif (${OPT_BOARD} STREQUAL stm32f103ze_stm3210e-eval)
    set(ARCH cortexM3_stm32)
elseif (${OPT_BOARD} STREQUAL stm32f103ve_mp3v2)
    set(ARCH cortexM3_stm32)
elseif (${OPT_BOARD} STREQUAL stm32f100rb_stm32vldiscovery)
    set(ARCH cortexM3_stm32)
elseif (${OPT_BOARD} STREQUAL stm32f103ve_strive_mini)
    set(ARCH cortexM3_stm32)
elseif (${OPT_BOARD} STREQUAL stm32f103ze_redbull_v2)
    set(ARCH cortexM3_stm32)
elseif (${OPT_BOARD} STREQUAL stm32f407vg_stm32f4discovery)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL stm32f207ig_stm3220g-eval)
    set(ARCH cortexM3_stm32f2)
elseif (${OPT_BOARD} STREQUAL stm32f207zg_ethboard_v2)
    set(ARCH cortexM3_stm32f2)
elseif (${OPT_BOARD} STREQUAL stm32f207ze_als_camboard)
    set(ARCH cortexM3_stm32f2)
elseif (${OPT_BOARD} STREQUAL stm32f205rg_sony-newman)
    set(ARCH cortexM3_stm32f2)
elseif (${OPT_BOARD} STREQUAL stm32l151_als_mainboard)
    set(ARCH cortexM3_stm32l1)
elseif (${OPT_BOARD} STREQUAL stm32f407vg_bitsboard)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL stm32f429zi_stm32f4discovery)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL stm32f103cb_als_mainboard_rev2)
    set(ARCH cortexM3_stm32)
elseif (${OPT_BOARD} STREQUAL stm32f100cb_tempsensor)
    set(ARCH cortexM3_stm32)
elseif (${OPT_BOARD} STREQUAL stm32f429zi_oledboard2)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL efm32gg332f1024_wandstem)
    set(ARCH cortexM3_efm32gg)
elseif (${OPT_BOARD} STREQUAL stm32f411re_nucleo)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL stm32f429zi_skyward_anakin)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL stm32f100rc_solertegiard)
    set(ARCH cortexM3_stm32)
elseif (${OPT_BOARD} STREQUAL stm32f205rc_skyward_stormtrooper)
    set(ARCH cortexM3_stm32f2)
elseif (${OPT_BOARD} STREQUAL stm32f401vc_stm32f4discovery)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL stm32f103c8_breakout)
    set(ARCH cortexM3_stm32)
elseif (${OPT_BOARD} STREQUAL stm32f100c8_microboard)
    set(ARCH cortexM3_stm32)
elseif (${OPT_BOARD} STREQUAL stm32f469ni_stm32f469i-disco)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL stm32f429zi_skyward_homeone)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL stm32f401re_nucleo)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL stm32f746zg_nucleo)
    set(ARCH cortexM7_stm32f7)
elseif (${OPT_BOARD} STREQUAL stm32h753xi_eval)
    set(ARCH cortexM7_stm32h7)
elseif (${OPT_BOARD} STREQUAL stm32f407vg_thermal_test_chip)
    set(ARCH cortexM4_stm32f4)
elseif (${OPT_BOARD} STREQUAL stm32f205_generic)
    set(ARCH cortexM3_stm32f2)
else()
    message(FATAL_ERROR "no board specified in miosix/config/CMakeLists.txt")
endif()

# Then, initialize C/C++ flags

set(CFLAGS_BASE "-D_MIOSIX_BOARDNAME=\\\"${OPT_BOARD}\\\" -ffunction-sections -Wno-unused-but-set-variable -Wall -g")
set(CXXFLAGS_BASE "-D_MIOSIX_BOARDNAME=\\\"${OPT_BOARD}\\\" -ffunction-sections -Wno-unused-but-set-variable -Wall -g")

# Now two big switch-like constructs nested. The first lists all possible
# architectures, setting common things for all boards in the architecture.
# Then for each architecture there is a switch for evry board, where all board
# specific options are set.

#------------------------------------------------------------------------------
# ARCHITECTURE: arm7_lpc2000

if (${ARCH} STREQUAL arm7_lpc2000)
    # Base directory with header files for this board
    set(ARCH_INC arch/arm7_lpc2000/common)

    #--------------------------------------------------------------------------
    # BOARD: lpc2138_miosix_board
    #
    if (${OPT_BOARD} STREQUAL lpc2138_miosix_board)
        # Base directory with header files for this board
        set(BOARD_INC arch/arm7_lpc2000/lpc2138_miosix_board)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(SOURCES BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/arch/arm7_lpc2000/lpc2138_miosix_board/miosix.ld)

        # Select architecture specific files
        # These are the files in arch/<arch name>/<board name>

        set(ARCH_SRC
                ${KPATH}/${BOARD_INC}/interfaces-impl/portability.cpp
                ${KPATH}/arch/common/drivers/sd_lpc2000.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/delays.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
        )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_MIOSIX_BOARD")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_MIOSIX_BOARD")

    #--------------------------------------------------------------------------
    # End of boards list
    #
    endif()

    # Select compiler
    set(PREFIX ${TOOLCHAIN_PATH}/bin/arm-miosix-eabi-)

    # From compiler prefix form the name of the compiler and other tools

    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_C_COMPILER ${PREFIX}gcc)
    set(CMAKE_CXX_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER ${PREFIX}g++)
    set(CMAKE_AR ${PREFIX}ar)
    set(CMAKE_ASM_COMPILER ${PREFIX}as)
    set(CMAKE_OBJCOPY ${PREFIX}objcopy)
    set(CMAKE_OBJDUMP ${PREFIX}objdump)
    set(SIZE ${PREFIX}size)

    # Select appropriate compiler flags for both ASM/C/C++/linker
    set(AFLAGS_BASE -mcpu=arm7tdmi -mapcs-32 -mfloat-abi=soft)
    set(CFLAGS_BASE "${CFLAGS_BASE} -D_ARCH_ARM7_LPC2000 -mcpu=arm7tdmi ${OPT_OPTIMIZATION} -c")
    set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_ARCH_ARM7_LPC2000 -mcpu=arm7tdmi ${OPT_OPTIMIZATION} ${OPT_EXCEPT} -c")
    set(LFLAGS_BASE "-mcpu=arm7tdmi -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib")

    # Select architecture specific files.
    # These are the files in arch/<arch name>/common.
    list(APPEND ARCH_SRC
            ${KPATH}/arch/common/core/interrupts_arm7.cpp
            ${KPATH}/arch/common/drivers/serial_lpc2000.cpp
            )

    # Select programmer command line.
    # This is the program that is invoked when the user types 'make program'
    # The command must provide a way to program the board, or print an error
    # message saying that 'make program' is not supported for that board.

    set(PROGRAM_CMDLINE "lpc21isp -verify main.hex /dev/ttyUSB0 115200 14746")

#------------------------------------------------------------------------------
# ARCHITECTURE: cortexM3_stm32
#
elseif (${ARCH} STREQUAL cortexM3_stm32)
    # Base directory with header files for this board
    set(ARCH_INC arch/cortexM3_stm32/common)

    #--------------------------------------------------------------------------
    # BOARD: stm32f103ze_stm3210e-eval
    #
    if (${OPT_BOARD} STREQUAL stm32f103ze_stm3210e-eval)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32/stm32f103ze_stm3210e-eval)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/sd_stm32f1.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM3210E_EVAL -DSTM32F10X_HD")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM3210E_EVAL -DSTM32F10X_HD")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        if (${LINKER_SCRIPT} STREQUAL ${LINKER_SCRIPT_PATH}stm32_512k+64k_all_in_xram.ld)
            set(PROGRAM_CMDLINE "${KPATH}/_tools/bootloaders/stm32/pc_loader/pc_loader /dev/ttyUSB0 main.bin")
        else()
            set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB0")
        endif()

    #--------------------------------------------------------------------------
    # BOARD: stm32f103ve_mp3v2
    #
    elseif (${OPT_BOARD} STREQUAL stm32f103ve_mp3v2)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32/stm32f103ve_mp3v2)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/sd_stm32f1.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_MP3V2 -DSTM32F10X_HD")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_MP3V2 -DSTM32F10X_HD")

        # Clock frequency
        set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=72000000)

        # Select programmer command line
        # This is the program that is invoked when the user types
        # 'make program'
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        if (${LINKER_SCRIPT} STREQUAL ${LINKER_SCRIPT_PATH}stm32_512k+64k_ram.ld)
            set(PROGRAM_CMDLINE "mp3v2_bootloader --ram main.bin")
        else()
            set(PROGRAM_CMDLINE "mp3v2_bootloader --code main.bin")
        endif()

    #--------------------------------------------------------------------------
    # BOARD: stm32f100rb_stm32vldiscovery
    #
    elseif (${OPT_BOARD} STREQUAL stm32f100rb_stm32vldiscovery)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32/stm32f100rb_stm32vldiscovery)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_128k+8k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                ${KPATH}/arch/common/drivers/servo_stm32.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32VLDISCOVERY -DSTM32F10X_MD_VL")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32VLDISCOVERY -DSTM32F10X_MD_VL")

        # Clock frequency
        set(CLOCK_FREQ -DSYSCLK_FREQ_24MHz=24000000)

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        #set(PROGRAM_CMDLINE "sudo vsprog -cstm32_vl -ms -I main.hex -oe -owf -ovf")
        set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB1")

    #--------------------------------------------------------------------------
    # BOARD: stm32f100rc_solertegiard
    #
    elseif (${OPT_BOARD} STREQUAL stm32f100rc_solertegiard)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32/stm32f100rc_solertegiard)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_256k+24k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                ${KPATH}/arch/common/drivers/servo_stm32.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_SOLERTEGIARD -DSTM32F10X_HD_VL")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_SOLERTEGIARD -DSTM32F10X_HD_VL")

        # Clock frequency
        set(CLOCK_FREQ -DSYSCLK_FREQ_24MHz=24000000)

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        #set(PROGRAM_CMDLINE "sudo vsprog -cstm32_vl -ms -I main.hex -oe -owf -ovf")
        set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB1")

    #--------------------------------------------------------------------------
    # BOARD: stm32f103ve_strive_mini
    #
    elseif (${OPT_BOARD} STREQUAL stm32f103ve_strive_mini)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32/stm32f103ve_strive_mini)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_512k+64k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/sd_stm32f1.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STRIVE_MINI -DSTM32F10X_HD")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STRIVE_MINI -DSTM32F10X_HD")

        # Clock frequency
        set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=72000000)

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.
        set(PROGRAM_CMDLINE "\"c:/Program Files/STMicroelectronics/STM32 ST-LINK Utility/ST-LINK Utility/ST-LINK_CLI.exe\" -c JTAG -Rst -P main.hex 0x08000000 -Run")

    #--------------------------------------------------------------------------
    # BOARD: HY RedBull V2 (or V1)
    #
    elseif (${OPT_BOARD} STREQUAL stm32f103ze_redbull_v2)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32/stm32f103ze_redbull_v2)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_512k+64k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/sd_stm32f1.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_REDBULL_V2 -DSTM32F10X_HD")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_REDBULL_V2 -DSTM32F10X_HD")

        # Clock frequency
        set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=72000000)

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.
        set(PROGRAM_CMDLINE "\"C:/Program Files/SEGGER/JLinkARM_V434d/JFlashARM.exe\" -openprjSTM32F10xxE.jflash -openmain.hex -auto -exit")
        #set(PROGRAM_CMDLINE "\"c:/Program Files/STMicroelectronics/STM32 ST-LINK Utility/ST-LINK Utility/ST-LINK_CLI.exe\" -c JTAG -Rst -P main.hex 0x08000000 -Run")

    #--------------------------------------------------------------------------
    # BOARD: stm32f103cb_als_mainboard_rev2
    #
    elseif (${OPT_BOARD} STREQUAL stm32f103cb_als_mainboard_rev2)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32/stm32f103cb_als_mainboard_rev2)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_128k+20k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp)

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_ALS_MAINBOARD_REV2 -DSTM32F10X_MD")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_ALS_MAINBOARD_REV2 -DSTM32F10X_MD")

        # Clock frequency.
        # Not defining anything results in HSI being used.
        #CLOCK_FREQ :=

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.
        set(PROGRAM_CMDLINE "stm32flash -w main.bin -v /dev/ttyUSB1")

    #--------------------------------------------------------------------------
    # BOARD: stm32f100cb_tempsensor
    #
    elseif (${OPT_BOARD} STREQUAL stm32f100cb_tempsensor)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32/stm32f100cb_tempsensor)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_127k+8k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp)

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_TEMPSENSOR -DSTM32F10X_MD_VL")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_TEMPSENSOR -DSTM32F10X_MD_VL")

        # Clock frequency.
        # Not defining anything results in HSI being used.
        set(CLOCK_FREQ "-DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.
        set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB1")

    #--------------------------------------------------------------------------
    # BOARD: stm32f103c8_breakout
    #
    elseif (${OPT_BOARD} STREQUAL stm32f103c8_breakout)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32/stm32f103c8_breakout)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_64k+20k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                ${KPATH}/arch/common/drivers/servo_stm32.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F103C8_BREAKOUT -DSTM32F10X_MD")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F103C8_BREAKOUT -DSTM32F10X_MD")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.
        #set(PROGRAM_CMDLINE "sudo vsprog -cstm32_vl -ms -I main.hex -oe -owf -ovf")
        set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB1")

    #--------------------------------------------------------------------------
    # BOARD: stm32f100c8_microboard
    #
    elseif (${OPT_BOARD} STREQUAL stm32f100c8_microboard)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32/stm32f100c8_microboard)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_63k+8k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>
        set(ARCH_SRC
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                ${KPATH}/${BOARD_INC}/drivers/rtc.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_MICROBOARD -DSTM32F10X_MD_VL")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_MICROBOARD -DSTM32F10X_MD_VL")

        # Clock frequency.
        # Not defining anything results in HSI being used.
        set(CLOCK_FREQ "-DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.
        set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB0")

    #--------------------------------------------------------------------------
    # End of boards list
    #
    endif()

    # Select compiler
    set(PREFIX ${TOOLCHAIN_PATH}/bin/arm-miosix-eabi-)

    # From compiler prefix form the name of the compiler and other tools
    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_C_COMPILER ${PREFIX}gcc)
    set(CMAKE_CXX_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER ${PREFIX}g++)
    set(CMAKE_AR ${PREFIX}ar)
    set(CMAKE_ASM_COMPILER ${PREFIX}as)
    set(CMAKE_OBJCOPY ${PREFIX}objcopy)
    set(CMAKE_OBJDUMP ${PREFIX}objdump)
    set(SIZE ${PREFIX}size)

    # Select appropriate compiler flags for both ASM/C/C++/linker
    set(AFLAGS_BASE -mcpu=cortex-m3 -mthumb)
    set(CFLAGS_BASE "${CFLAGS_BASE} -D_ARCH_CORTEXM3_STM32 ${CLOCK_FREQ} ${XRAM} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c")
    set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_ARCH_CORTEXM3_STM32 ${CLOCK_FREQ} ${XRAM} ${OPT_EXCEPT} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c")
    set(LFLAGS_BASE "-mcpu=cortex-m3 -mthumb -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib")

    # Select architecture specific files.
    # These are the files in arch/<arch name>/common
    list(APPEND ARCH_SRC
            ${KPATH}/arch/common/core/interrupts_cortexMx.cpp
            ${KPATH}/arch/common/drivers/serial_stm32.cpp
            ${KPATH}/arch/common/drivers/dcc.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/portability.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/delays.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/gpio_impl.cpp
            ${KPATH}/arch/common/CMSIS/Device/ST/STM32F10x/Source/Templates/system_stm32f10x.c
            )

#------------------------------------------------------------------------------
# ARCHITECTURE: cortexM4_stm32f4
#
elseif (${ARCH} STREQUAL cortexM4_stm32f4)
    # Base directory with else header files for this board
    set(ARCH_INC arch/cortexM4_stm32f4/common)

    #--------------------------------------------------------------------------
    # BOARD: stm32f407vg_stm32f4discovery
    #
    if (${OPT_BOARD} STREQUAL stm32f407vg_stm32f4discovery)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f407vg_stm32f4discovery)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
                ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
                ${KPATH}/arch/common/drivers/servo_stm32.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F4DISCOVERY")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F4DISCOVERY")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")

    #--------------------------------------------------------------------------
    # BOARD: stm32f4bitsboard
    #
    elseif (${OPT_BOARD} STREQUAL stm32f407vg_bitsboard)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f407vg_bitsboard)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_1m+192k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_BITSBOARD")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_BITSBOARD")

        # Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "stm32flash -w ./main.bin -v /dev/ttyUSB1")

    #--------------------------------------------------------------------------
    # BOARD: stm32f429zi_stm32f4discovery
    #
    elseif (${OPT_BOARD} STREQUAL stm32f429zi_stm32f4discovery)
        ## Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f429zi_stm32f4discovery)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
                ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F429ZI_STM32F4DISCOVERY")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F429ZI_STM32F4DISCOVERY")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")

    #--------------------------------------------------------------------------
    # BOARD: stm32f429zi_oledboard2
    #
    elseif (${OPT_BOARD} STREQUAL stm32f429zi_oledboard2)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f429zi_oledboard2)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F429ZI_OLEDBOARD2")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F429ZI_OLEDBOARD2")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "stm32flash -w main.bin -v /dev/ttyUSB1")

    #--------------------------------------------------------------------------
    # BOARD: stm32f411re_nucleo
    #
    elseif (${OPT_BOARD} STREQUAL stm32f411re_nucleo)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f411re_nucleo)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_512k+128k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp)

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F411RE_NUCLEO")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F411RE_NUCLEO")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")

    #--------------------------------------------------------------------------
    # stm32f429zi_skyward_anakin
    #
    elseif (${OPT_BOARD} STREQUAL stm32f429zi_skyward_anakin)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f429zi_skyward_anakin)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
                ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
                ${KPATH}/arch/common/drivers/stm32_sgm.cpp
                ${KPATH}/arch/common/drivers/stm32_wd.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F429ZI_SKYWARD_ANAKIN")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F429ZI_SKYWARD_ANAKIN")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "stm32flash -w main.bin -v /dev/ttyUSB0")

    #--------------------------------------------------------------------------
    # stm32f401vc_stm32f4discovery
    #
    elseif (${OPT_BOARD} STREQUAL stm32f401vc_stm32f4discovery)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f401vc_stm32f4discovery)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_256k+64k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F401VC_STM32F4DISCOVERY")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F401VC_STM32F4DISCOVERY")

        # Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")

    #--------------------------------------------------------------------------
    # BOARD: stm32f469ni_stm32f469i-disco
    #
    elseif (${OPT_BOARD} STREQUAL stm32f469ni_stm32f469i-disco)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f469ni_stm32f469i-disco)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
                ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F469NI_STM32F469I_DISCO")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F469NI_STM32F469I_DISCO")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")

    #--------------------------------------------------------------------------
    # BOARD: stm32f429zi_skyward_homeone
    #
    elseif (${OPT_BOARD} STREQUAL stm32f429zi_skyward_homeone)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f429zi_skyward_homeone)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
                ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
                ${KPATH}/arch/common/drivers/stm32_sgm.cpp
                ${KPATH}/arch/common/drivers/stm32_wd.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        ## Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F429ZI_SKYWARD_HOMEONE")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F429ZI_SKYWARD_HOMEONE")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")

    #--------------------------------------------------------------------------
    # BOARD: stm32f401re_nucleo
    #
    elseif (${OPT_BOARD} STREQUAL stm32f401re_nucleo)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f401re_nucleo)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_512k+96k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp)

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F401RE_NUCLEO")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F401RE_NUCLEO")

        # Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")

    #-------------------------------------------------------------------------
    # BOARD: stm32f407vg_thermal_test_chip
    #
    elseif (${OPT_BOARD} STREQUAL stm32f407vg_thermal_test_chip)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM4_stm32f4/stm32f407vg_thermal_test_chip)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_1m+192k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp)

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_THERMALTESTCHIP")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_THERMALTESTCHIP")

        # Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")

    #--------------------------------------------------------------------------
    # End of boards list
    #
    endif()

    # Select compiler
    set(PREFIX ${TOOLCHAIN_PATH}/bin/arm-miosix-eabi-)

    # From compiler prefix form the name of the compiler and other tools
    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_C_COMPILER ${PREFIX}gcc)
    set(CMAKE_CXX_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER ${PREFIX}g++)
    set(CMAKE_AR ${PREFIX}ar)
    set(CMAKE_ASM_COMPILER ${PREFIX}as)
    set(CMAKE_OBJCOPY ${PREFIX}objcopy)
    set(CMAKE_OBJDUMP ${PREFIX}objdump)
    set(SIZE ${PREFIX}size)

    # Select appropriate compiler flags for both ASM/C/C++/linker
    set(AFLAGS_BASE -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)
    set(CFLAGS_BASE "${CFLAGS_BASE} -D_ARCH_CORTEXM4_STM32F4 ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${OPT_OPTIMIZATION} -c")
    set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_ARCH_CORTEXM4_STM32F4 ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -c")
    set(LFLAGS_BASE "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib")

    # Select architecture specific files.
    # These are the files in arch/<arch name>/common.
    list(APPEND ARCH_SRC
            ${KPATH}/arch/common/core/interrupts_cortexMx.cpp
            ${KPATH}/arch/common/core/mpu_cortexMx.cpp
            ${KPATH}/arch/common/drivers/serial_stm32.cpp
            ${KPATH}/arch/common/drivers/dcc.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/portability.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/delays.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/gpio_impl.cpp
            ${KPATH}/arch/common/drivers/sd_stm32f2_f4.cpp
            ${KPATH}/arch/common/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
            )

#------------------------------------------------------------------------------
# ARCHITECTURE: cortexM3_stm32f2
#
elseif (${ARCH} STREQUAL cortexM3_stm32f2)
    # Base directory with else header files for this board
    set(ARCH_INC arch/cortexM3_stm32f2/common)

    #--------------------------------------------------------------------------
    # BOARD: stm32f207ig_stm3220g-eval
    #
    if (${OPT_BOARD} EQUAL stm32f207ig_stm3220g-eval)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32f2/stm32f207ig_stm3220g-eval)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/sd_stm32f2_f4.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/delays.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM3220G_EVAL")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM3220G_EVAL")

        # Clock frequency
        set(CLOCK_FREQ "-DHSE_VALUE=25000000 -DSYSCLK_FREQ_120MHz=120000000")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        if (${LINKER_SCRIPT} STREQUAL ${LINKER_SCRIPT_PATH}stm32_1m+128k_all_in_xram.ld)
            set(PROGRAM_CMDLINE "${KPATH}/_tools/bootloaders/stm32/pc_loader/pc_loader /dev/ttyUSB0 main.bin")
        else()
            set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")
        endif()

    #--------------------------------------------------------------------------
    # BOARD: stm32f207zg_ethboard_v2
    #
    elseif (${OPT_BOARD} STREQUAL stm32f207zg_ethboard_v2)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32f2/stm32f207zg_EthBoardV2)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/sd_stm32f2_f4.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/delays.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_ETHBOARDV2")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_ETHBOARDV2")

        # Clock frequency
        set(CLOCK_FREQ "-DHSE_VALUE=25000000 -DSYSCLK_FREQ_120MHz=120000000")

        # XRAM is always enabled in this board
        set(XRAM "${XRAM} -D__ENABLE_XRAM")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        if (${LINKER_SCRIPT} STREQUAL ${LINKER_SCRIPT_PATH}stm32_1m+128k_code_in_xram.ld)
            set(PROGRAM_CMDLINE "${KPATH}/_tools/bootloaders/stm32/pc_loader/pc_loader /dev/ttyUSB1 main.bin")
        else()
            set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB1")
        endif()

    #--------------------------------------------------------------------------
    # BOARD: stm32f207ze_als_camboard
    #
    elseif (${OPT_BOARD} STREQUAL stm32f207ze_als_camboard)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32f2/stm32f207ze_als_camboard)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_1m+128k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/${BOARD_INC}/interfaces-impl/delays.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_ALS_CAMBOARD")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_ALS_CAMBOARD")

        # Clock frequency
        set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_120MHz=120000000")

        # XRAM is always enabled in this board
        set(XRAM "${XRAM} -D__ENABLE_XRAM")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB1")

    #--------------------------------------------------------------------------
    # BOARD: stm32f205rg_sony-newman
    #
    elseif (${OPT_BOARD} STREQUAL stm32f205rg_sony-newman)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32f2/stm32f205rg_sony-newman)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_1M+128k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/delays.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_SONY_NEWMAN")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_SONY_NEWMAN")

        # Clock frequency
        set(CLOCK_FREQ -DHSE_VALUE=26000000)

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.
        # The magic.bin is somewhat used by the bootloader to detect a good fw
        set(PROGRAM_CMDLINE "perl -e 'print \"\\xe7\\x91\\x11\\xc0\"' > magic.bin; \
                            dfu-util -d 0fce:f0fa -a 0 -i 0 -s 0x08040000 -D main.bin;     \
                            dfu-util -d 0fce:f0fa -a 0 -i 0 -s 0x080ffffc -D magic.bin;    \
                            rm magic.bin")

    #--------------------------------------------------------------------------
    # BOARD: stm32f205rc_skyward_stormtrooper
    #
    elseif (${OPT_BOARD} STREQUAL stm32f205rc_skyward_stormtrooper)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32f2/stm32f205rc_skyward_stormtrooper)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_512k+128k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/delays.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F205RC_SKYWARD_STORMTROOPER")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F205RC_SKYWARD_STORMTROOPER")

        # Clock frequency
        set(CLOCK_FREQ "-DHSE_VALUE=25000000 -DSYSCLK_FREQ_120MHz=120000000")

    #--------------------------------------------------------------------------
    # BOARD: stm32f205_generic
    #
    elseif (${OPT_BOARD} STREQUAL stm32f205_generic)
        ## Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32f2/stm32f205_generic)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_1m+128k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/sd_stm32f2_f4.cpp
                ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
                ${KPATH}/arch/common/drivers/servo_stm32.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/delays.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F205_GENERIC")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F205_GENERIC")

        # Clock frequency
        set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_120MHz=120000000")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB0")

    #--------------------------------------------------------------------------
    # End of boards list
    #
    endif()

    # Select compiler
    set(PREFIX ${TOOLCHAIN_PATH}/bin/arm-miosix-eabi-)

    # From compiler prefix form the name of the compiler and other tools
    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_C_COMPILER ${PREFIX}gcc)
    set(CMAKE_CXX_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER ${PREFIX}g++)
    set(CMAKE_AR ${PREFIX}ar)
    set(CMAKE_ASM_COMPILER ${PREFIX}as)
    set(CMAKE_OBJCOPY ${PREFIX}objcopy)
    set(CMAKE_OBJDUMP ${PREFIX}objdump)
    set(SIZE ${PREFIX}size)

    # Select appropriate compiler flags for both ASM/C/C++/linker
    set(AFLAGS_BASE -mcpu=cortex-m3 -mthumb)
    set(CFLAGS_BASE "${CFLAGS_BASE} -D_ARCH_CORTEXM3_STM32F2 ${CLOCK_FREQ} ${XRAM} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c")
    set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_ARCH_CORTEXM3_STM32F2 ${CLOCK_FREQ} ${XRAM} ${OPT_EXCEPT} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c")
    set(LFLAGS_BASE "-mcpu=cortex-m3 -mthumb -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib")

    # Select architecture specific files.
    # These are the files in arch/<arch name>/common.
    list(APPEND ARCH_SRC
            ${KPATH}/arch/common/core/interrupts_cortexMx.cpp
            ${KPATH}/arch/common/core/mpu_cortexMx.cpp
            ${KPATH}/arch/common/drivers/serial_stm32.cpp
            ${KPATH}/arch/common/drivers/dcc.cpp
            ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/portability.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/gpio_impl.cpp
            ${KPATH}/arch/common/CMSIS/Device/ST/STM32F2xx/Source/Templates/system_stm32f2xx.c
            )

#------------------------------------------------------------------------------
# ARCHITECTURE: cortexM3_stm32l1
#
elseif (${ARCH} STREQUAL cortexM3_stm32l1)
    # Base directory with else header files for this board
    set(ARCH_INC arch/cortexM3_stm32l1/common)

    #--------------------------------------------------------------------------
    # BOARD: stm32l151c8_als_mainboard
    #
    if (${OPT_BOARD} EQUAL stm32l151_als_mainboard)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_stm32l1/stm32l151c8_als_mainboard)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_64k+10k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp)

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_ALS_MAINBOARD")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_ALS_MAINBOARD")

        # Clock frequency
        set(CLOCK_FREQ -DSYSCLK_FREQ_16MHz=16000000)

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "stm32flash -w main.bin -v /dev/ttyUSB1")

    #--------------------------------------------------------------------------
    # End of boards list
    #
    endif()

    # Select compiler
    set(PREFIX ${TOOLCHAIN_PATH}/bin/arm-miosix-eabi-)

    # From compiler prefix form the name of the compiler and other tools
    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_C_COMPILER ${PREFIX}gcc)
    set(CMAKE_CXX_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER ${PREFIX}g++)
    set(CMAKE_AR ${PREFIX}ar)
    set(CMAKE_ASM_COMPILER ${PREFIX}as)
    set(CMAKE_OBJCOPY ${PREFIX}objcopy)
    set(CMAKE_OBJDUMP ${PREFIX}objdump)
    set(SIZE ${PREFIX}size)

    # Select appropriate compiler flags for both ASM/C/C++/linker
    set(AFLAGS_BASE -mcpu=cortex-m3 -mthumb)
    set(CFLAGS_BASE "${CFLAGS_BASE} -D_ARCH_CORTEXM3_STM32L1 ${CLOCK_FREQ} ${XRAM} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c")
    set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_ARCH_CORTEXM3_STM32L1 ${CLOCK_FREQ} ${XRAM} ${OPT_EXCEPT} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c")
    set(LFLAGS_BASE " -mcpu=cortex-m3 -mthumb -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib")

    # Select architecture specific files.
    # These are the files in arch/<arch name>/common.
    list(APPEND ARCH_SRC
            ${KPATH}/arch/common/core/interrupts_cortexMx.cpp
            ${KPATH}/arch/common/drivers/serial_stm32.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/portability.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/gpio_impl.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/delays.cpp
            ${KPATH}/arch/common/CMSIS/Device/ST/STM32L1xx/Source/Templates/system_stm32l1xx.c
            )

#------------------------------------------------------------------------------
# ARCHITECTURE: cortexM3_efm32gg
#
elseif (${ARCH} STREQUAL cortexM3_efm32gg)
    # Base directory with else header files for this board
    set(ARCH_INC arch/cortexM3_efm32gg/common)

    #--------------------------------------------------------------------------
    # BOARD: efm32gg332f1024_wandstem
    #
    if (${OPT_BOARD} STREQUAL efm32gg332f1024_wandstem)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM3_efm32gg/efm32gg332f1024_wandstem)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/efm32_1M+128k_rom_usbbootloader.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/spi.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/power_manager.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/gpioirq.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/hardware_timer.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/transceiver.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -DEFM32GG332F1024 -D_BOARD_WANDSTEM")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} ${CFLAGS_BASE}")

        # Clock frequency
        set(CLOCK_FREQ "-DEFM32_HFXO_FREQ=48000000 -DEFM32_LFXO_FREQ=32768")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "echo 'make program not supported.'")

    #--------------------------------------------------------------------------
    # End of boards list
    #
    endif()

    # Select compiler
    set(PREFIX ${TOOLCHAIN_PATH}/bin/arm-miosix-eabi-)

    # From compiler prefix form the name of the compiler and other tools
    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_C_COMPILER ${PREFIX}gcc)
    set(CMAKE_CXX_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER ${PREFIX}g++)
    set(CMAKE_AR ${PREFIX}ar)
    set(CMAKE_ASM_COMPILER ${PREFIX}as)
    set(CMAKE_OBJCOPY ${PREFIX}objcopy)
    set(CMAKE_OBJDUMP ${PREFIX}objdump)
    set(SIZE ${PREFIX}size)

    # Select appropriate compiler flags for both ASM/C/C++/linker
    set(AFLAGS_BASE -mcpu=cortex-m3 -mthumb)
    set(CFLAGS_BASE "${CFLAGS_BASE} -D_ARCH_CORTEXM3_EFM32GG ${CLOCK_FREQ} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c")
    set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_ARCH_CORTEXM3_EFM32GG ${CLOCK_FREQ} ${OPT_EXCEPT} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c")
    set(LFLAGS_BASE "-mcpu=cortex-m3 -mthumb -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib")

    # Select architecture specific files.
    # These are the files in arch/<arch name>/common.
    list(APPEND ARCH_SRC
            ${KPATH}/arch/common/core/interrupts_cortexMx.cpp
            ${KPATH}/arch/common/drivers/serial_efm32.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/portability.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/gpio_impl.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/delays.cpp
            ${KPATH}/arch/common/CMSIS/Device/SiliconLabs/EFM32GG/Source/system_efm32gg.c
            )

#------------------------------------------------------------------------------
# ARCHITECTURE: cortexM7_stm32f7
#
elseif (${ARCH} STREQUAL cortexM7_stm32f7)
    # Base directory with else header files for this board
    set(ARCH_INC arch/cortexM7_stm32f7/common)

    #--------------------------------------------------------------------------
    # BOARD: stm32f746zg_nucleo
    #
    if (${OPT_BOARD} STREQUAL stm32f746zg_nucleo)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM7_stm32f7/stm32f746zg_nucleo)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        set(LINKER_SCRIPT ${KPATH}/${BOARD_INC}/stm32_1024k+256k_rom.ld)

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32F746ZG_NUCLEO")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32F746ZG_NUCLEO")

        # Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        set(CLOCK_FREQ "-DHSE_VALUE=8000000 -DSYSCLK_FREQ_216MHz=216000000")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "echo 'make program not supported.'")

    #--------------------------------------------------------------------------
    # End of boards list
    #
    endif()

    # Select compiler
    set(PREFIX ${TOOLCHAIN_PATH}/bin/arm-miosix-eabi-)

    # From compiler prefix form the name of the compiler and other tools
    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_C_COMPILER ${PREFIX}gcc)
    set(CMAKE_CXX_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER ${PREFIX}g++)
    set(CMAKE_AR ${PREFIX}ar)
    set(CMAKE_ASM_COMPILER ${PREFIX}as)
    set(CMAKE_OBJCOPY ${PREFIX}objcopy)
    set(CMAKE_OBJDUMP ${PREFIX}objdump)
    set(SIZE ${PREFIX}size)

    # Select appropriate compiler flags for both ASM/C/C++/linker.
    # TODO: when upgrading compiler, add native cortex M7 support.
    # For now we'll fallback to M4 which is compatible, but does not support
    # double precision HW floating point.
    set(AFLAGS_BASE -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)
    set(CFLAGS_BASE "${CFLAGS_BASE} -D_ARCH_CORTEXM7_STM32F7 ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${OPT_OPTIMIZATION} -c")
    set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_ARCH_CORTEXM7_STM32F7 ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -c")
    set(LFLAGS_BASE "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib")

    # Select architecture specific files.
    # These are the files in arch/<arch name>/common.
    list(APPEND ARCH_SRC
            ${KPATH}/arch/common/core/interrupts_cortexMx.cpp
            ${KPATH}/arch/common/core/mpu_cortexMx.cpp
            ${KPATH}/arch/common/core/cache_cortexMx.cpp
            ${KPATH}/arch/common/drivers/serial_stm32.cpp
            ${KPATH}/arch/common/drivers/sd_stm32f2_f4.cpp
            ${KPATH}/arch/common/drivers/dcc.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/portability.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/delays.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/gpio_impl.cpp
            ${KPATH}/arch/common/CMSIS/Device/ST/STM32F7xx/Source/Templates/system_stm32f7xx.c
            )

#------------------------------------------------------------------------------
# ARCHITECTURE: cortexM7_stm32h7
#
elseif (${ARCH} STREQUAL cortexM7_stm32h7)
    # Base directory with else header files for this board
    set(ARCH_INC arch/cortexM7_stm32h7/common)

    #--------------------------------------------------------------------------
    # BOARD: stm32h753xi_eval
    #
    if (${OPT_BOARD} STREQUAL stm32h753xi_eval)
        # Base directory with header files for this board
        set(BOARD_INC arch/cortexM7_stm32h7/stm32h753xi_eval)

        # Select linker script and boot file.
        # Their path must be relative to the miosix directory.
        set(BOOT_FILE ${KPATH}/${BOARD_INC}/core/stage_1_boot.cpp)
        #LINKER_SCRIPT := already selected in board options

        # Select architecture specific files.
        # These are the files in arch/<arch name>/<board name>.
        set(ARCH_SRC
                ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
                ${KPATH}/${BOARD_INC}/interfaces-impl/bsp.cpp
                )

        # Add a #define to allow querying board name
        set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_STM32H753XI_EVAL")
        set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_STM32H753XI_EVAL")

        # Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        set(CLOCK_FREQ "-DHSE_VALUE=25000000 -DSYSCLK_FREQ_400MHz=400000000")

        # Select programmer command line.
        # This is the program that is invoked when the user types
        # 'make program'.
        # The command must provide a way to program the board, or print an
        # error message saying that 'make program' is not supported for that
        # board.

        set(PROGRAM_CMDLINE "echo 'make program not supported.'")

    #--------------------------------------------------------------------------
    # End of board list
    #
    endif()

    # Select compiler
    set(PREFIX ${TOOLCHAIN_PATH}/bin/arm-miosix-eabi-)

    # From compiler prefix form the name of the compiler and other tools
    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_C_COMPILER ${PREFIX}gcc)
    set(CMAKE_CXX_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER ${PREFIX}g++)
    set(CMAKE_AR ${PREFIX}ar)
    set(CMAKE_ASM_COMPILER ${PREFIX}as)
    set(CMAKE_OBJCOPY ${PREFIX}objcopy)
    set(CMAKE_OBJDUMP ${PREFIX}objdump)
    set(SIZE ${PREFIX}size)

    # Select appropriate compiler flags for both ASM/C/C++/linker.
    # TODO: when upgrading compiler, add native cortex M7 support.
    # For now we'll fallback to M4 which is compatible, but does not support
    # double precision HW floating point.
    set(AFLAGS_BASE -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)
    set(CFLAGS_BASE "${CFLAGS_BASE} -D_ARCH_CORTEXM7_STM32H7 ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${OPT_OPTIMIZATION} -c")
    set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_ARCH_CORTEXM7_STM32H7 ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -c")
    set(LFLAGS_BASE "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib")

    # Select architecture specific files.
    # These are the files in arch/<arch name>/common.
    list(APPEND ARCH_SRC
            ${KPATH}/arch/common/core/interrupts_cortexMx.cpp
            ${KPATH}/arch/common/core/mpu_cortexMx.cpp
            ${KPATH}/arch/common/core/cache_cortexMx.cpp
            ${KPATH}/arch/common/drivers/serial_stm32.cpp
            ${KPATH}/arch/common/drivers/dcc.cpp
            ${KPATH}/${ARCH_INC}/drivers/pll.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/portability.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/delays.cpp
            ${KPATH}/${ARCH_INC}/interfaces-impl/gpio_impl.cpp
            ${KPATH}/arch/common/CMSIS/Device/ST/STM32H7xx/Source/Templates/system_stm32h7xx.c
            )

    #--------------------------------------------------------------------------
    # end of architecture list
    #
endif()
