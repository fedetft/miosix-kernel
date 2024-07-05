##
## Makefile for Miosix embedded OS
## 
## This file contains the options required by the build system to build
## Miosix on various target architectures. All options start with OPT_
## to be easily recognizable.
## All architecture specific build code is grouped at the end of this file.
##

##
## Target board, choose one. This also implicitly select the target
## architecture
##
#OPT_BOARD := lpc2138_miosix_board
#OPT_BOARD := stm32f103ze_stm3210e-eval
#OPT_BOARD := stm32f103ve_mp3v2
#OPT_BOARD := stm32f100rb_stm32vldiscovery
#OPT_BOARD := stm32f103ve_strive_mini
#OPT_BOARD := stm32f103ze_redbull_v2
#OPT_BOARD := stm32f407vg_stm32f4discovery
#OPT_BOARD := stm32f207ig_stm3220g-eval
#OPT_BOARD := stm32f207zg_ethboard_v2
#OPT_BOARD := stm32f207zg_nucleo
#OPT_BOARD := stm32f207ze_als_camboard
#OPT_BOARD := stm32l151_als_mainboard
#OPT_BOARD := stm32f407vg_bitsboard
#OPT_BOARD := stm32f205rg_sony-newman
#OPT_BOARD := stm32f429zi_stm32f4discovery
#OPT_BOARD := stm32f103cb_als_mainboard_rev2
#OPT_BOARD := stm32f100cb_tempsensor
#OPT_BOARD := stm32f429zi_oledboard2
#OPT_BOARD := efm32gg332f1024_wandstem
#OPT_BOARD := stm32f411re_nucleo
#OPT_BOARD := stm32f429zi_skyward_anakin
#OPT_BOARD := stm32f100rc_solertegiard
#OPT_BOARD := stm32f205rc_skyward_stormtrooper
#OPT_BOARD := stm32f401vc_stm32f4discovery
#OPT_BOARD := stm32f103c8_breakout
#OPT_BOARD := stm32f100c8_microboard
#OPT_BOARD := stm32f469ni_stm32f469i-disco
#OPT_BOARD := stm32f429zi_skyward_homeone
#OPT_BOARD := stm32f401re_nucleo
#OPT_BOARD := stm32f746zg_nucleo
#OPT_BOARD := stm32h753xi_eval
#OPT_BOARD := stm32h723zg_nucleo
#OPT_BOARD := stm32f407vg_thermal_test_chip
#OPT_BOARD := stm32f205_generic
#OPT_BOARD := stm32f103cx_generic
#OPT_BOARD := stm32f072rb_stm32f0discovery
#OPT_BOARD := stm32f100cx_generic
#OPT_BOARD := stm32f303vc_stm32f3discovery
#OPT_BOARD := stm32f100c8_vaisala_rs41
#OPT_BOARD := stm32l476rg_nucleo
#OPT_BOARD := atsam4lc2aa_generic
#OPT_BOARD := stm32f411ce_blackpill
#OPT_BOARD := stm32l4r9zi_sensortile
#OPT_BOARD := stm32f767zi_nucleo
#OPT_BOARD := stm32f769ni_discovery
#OPT_BOARD := stm32f415vg_st25dvdiscovery
#OPT_BOARD := efm32g222f128_generic
#OPT_BOARD := stm32l053r8_nucleo
#OPT_BOARD := stm32f030r8_stm32f0discovery
#OPT_BOARD := stm32f765ii_marco_ram_board
#OPT_BOARD := rp2040_raspberry_pi_pico
#OPT_BOARD := stm32h755zi_nucleo

##
## Optimization flags, choose one.
## -O0 produces large and slow code, but is useful for in circuit debugging.
## -O2 is recomended otherwise, as it provides a good balance between code
## size and speed
##
#OPT_OPTIMIZATION := -O0
OPT_OPTIMIZATION := -O2
#OPT_OPTIMIZATION := -O3
#OPT_OPTIMIZATION := -Os

##
## C++ Exception/rtti support disable flags.
## To save code size if not using C++ exceptions (nor some STL code which
## implicitly uses it) uncomment this option.
## the -D__NO_EXCEPTIONS is used by Miosix to know if exceptions are used.
##
#OPT_EXCEPT := -fno-exceptions -fno-rtti -D__NO_EXCEPTIONS

## C++ Exception/rtti support disable flags for userspace processes
#PROC_OPT_EXCEPT := -fno-exceptions -fno-rtti -D__NO_EXCEPTIONS

#############################################################################
## Board specific options
#############################################################################

##---------------------------------------------------------------------------
## lpc2138_miosix_board
##

# No options

##---------------------------------------------------------------------------
## stm32f103ze_stm3210e-eval
##
ifeq ($(OPT_BOARD),stm32f103ze_stm3210e-eval)

    ## Linker script type, there are three options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in FLASH stack in internal RAM heap in external RAM (file
    ##    *_xram.ld) useful for hardware like STM3210E-EVAL when big heap is
    ##    needed. The microcontroller must have an external memory interface.
    ## 3) Code + stack + heap in external RAM, (file *_all_in_xram.ld)
    ##    useful for debugging code in hardware like STM3210E-EVAL. Code runs
    ##    *very* slow compared to FLASH. Works only with a booloader that
    ##    forwards interrrupts @ 0x68000000 (see miosix/_tools/bootloaders for
    ##    one).
    ##    The microcontroller must have an external memory interface.
    ## Warning! enable external ram if you use a linker script that requires it
    ## (see the XRAM flag below)
    LINKER_SCRIPT_PATH := arch/cortexM3_stm32f1/stm32f103ze_stm3210e-eval/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+64k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+64k_xram.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+64k_xram_processes.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+64k_all_in_xram.ld

    ## Enable/disable initialization of external RAM at boot. Three options:
    ## __ENABLE_XRAM : If you want the heap in xram (with an appropriate linker
    ## script selected above)
    ## __ENABLE_XRAM and __CODE_IN_XRAM : Debug mode with code + stack + heap
    ## in xram (with an appropriate linker script selected above)
    ## none selected : don't use xram (with an appropriate linker script
    ## selected above)
    #XRAM := -D__ENABLE_XRAM
    XRAM := -D__ENABLE_XRAM -D__CODE_IN_XRAM

    ## Select clock frequency
    ## Not defining any of these results in the internal 8MHz clock to be used
    #CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_36MHz=36000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_48MHz=48000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_56MHz=56000000
    CLOCK_FREQ := -DSYSCLK_FREQ_72MHz=72000000

endif

##---------------------------------------------------------------------------
## stm32f103ve_mp3v2
##
ifeq ($(OPT_BOARD),stm32f103ve_mp3v2)

    ## Linker script type, there are two options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ## 2) Code + stack + heap in internal RAM (file *_ram.ld)
    ## Note: this board relies on a bootloader for interrupt forwarding in ram
    LINKER_SCRIPT_PATH := arch/cortexM3_stm32f1/stm32f103ve_mp3v2/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+64k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+64k_ram.ld

endif

##---------------------------------------------------------------------------
## stm32f103ve_strive_mini
##

# No options

##---------------------------------------------------------------------------
## stm32f103ze_redbull_v2
##

# No options

##---------------------------------------------------------------------------
## stm32f407vg_stm32f4discovery
##
ifeq ($(OPT_BOARD),stm32f407vg_stm32f4discovery)

    ## Linker script type, there are two options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ## 2) Code + stack + heap in internal RAM (file *_ram.ld)
    ## 3) Same as 1) but space has been reserved for a process pool, allowing
    ##    to configure the kernel with "#define WITH_PROCESSES"
    LINKER_SCRIPT_PATH := arch/cortexM4_stm32f4/stm32f407vg_stm32f4discovery/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+192k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+192k_ram.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+192k_rom_processes.ld

    ## This causes the interrupt vector table to be relocated in SRAM, must be
    ## uncommented when using the ram linker script
    #SRAM_BOOT := -DVECT_TAB_SRAM

    ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
    CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000
    #CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000
    #CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000

endif

##---------------------------------------------------------------------------
## stm32f207ig_stm3220g-eval
##
ifeq ($(OPT_BOARD),stm32f207ig_stm3220g-eval)

    ## Linker script type, there are three options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in FLASH stack in internal RAM heap in external RAM (file
    ##    *_xram.ld) useful for hardware like STM3220G-EVAL when big heap is
    ##    needed. The microcontroller must have an external memory interface.
    ## 3) Code + stack + heap in external RAM, (file *_all_in_xram.ld)
    ##    useful for debugging code in hardware like STM3220G-EVAL. Code runs
    ##    *very* slow compared to FLASH. Works only with a booloader that
    ##    forwards interrrupts @ 0x64000000 (see miosix/_tools/bootloaders for
    ##    one).
    ##    The microcontroller must have an external memory interface.
    ## 4) Same as 3) but space has been reserved for a process pool, allowing
    ##    to configure the kernel with "#define WITH_PROCESSES"
    ## 5) Code in FLASH, kernel stack+heap in internal RAM, ELF pool in Flash
    ##    and process pool in external RAM.
    ## 6) Code in FLASH, kernel heap, data and bss in external RAM. Kernel heap
    ##    (used for boot and IRQs) in internal RAM.
    ## Warning! enable external ram if you use a linker script that requires it
    ## (see the XRAM flag below)
    LINKER_SCRIPT_PATH := arch/cortexM3_stm32f2/stm32f207ig_stm3220g-eval/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_all_in_xram.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram_processes.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram-data-bss-heap.ld

    ## Enable/disable initialization of external RAM at boot. Three options:
    ## __ENABLE_XRAM : If you want the heap in xram (with an appropriate linker
    ## script selected above)
    ## __ENABLE_XRAM and __CODE_IN_XRAM : Debug mode with code + stack + heap
    ## in xram (with an appropriate linker script selected above)
    ## none selected : don't use xram (with an appropriate linker script
    ## selected above)
    #XRAM := -D__ENABLE_XRAM
    XRAM := -D__ENABLE_XRAM -D__CODE_IN_XRAM

endif

##---------------------------------------------------------------------------
## stm32f207zg_ethboard_v2
##
ifeq ($(OPT_BOARD),stm32f207zg_ethboard_v2)

    ## Linker script type, there are two options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in external RAM, stack + heap in internal RAM
    ##    (file *_code_in_xram.ld) useful for debugging. Code runs
    ##    *very* slow compared to FLASH. Works only with a booloader that
    ##    forwards interrrupts @ 0x60000000 (see miosix/_tools/bootloaders for
    ##    one).
    ##    You must -D__CODE_IN_XRAM below.
    LINKER_SCRIPT_PATH := arch/cortexM3_stm32f2/stm32f207zg_EthBoardV2/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_rom.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_code_in_xram.ld

    ## XRAM is always enabled on this board, even if the _rom linker script
    ## does not make explicit use of it.
    ## Uncommenting __CODE_IN_XRAM (with an appropriate linker script selected
    ## above) allows to run code from external RAM, useful for debugging
    XRAM := -D__CODE_IN_XRAM

endif

##---------------------------------------------------------------------------
## stm32f207zg_nucleo
##

# No options

##---------------------------------------------------------------------------
## stm32f205rg_sony-newman
##

# No options

##---------------------------------------------------------------------------
## stm32f407vg_bitsboard
##

# No options

##---------------------------------------------------------------------------
## stm32f429zi_stm32f4discovery
##
ifeq ($(OPT_BOARD),stm32f429zi_stm32f4discovery)

    ## Linker script type, there are three options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
    ##    You must uncomment -D__ENABLE_XRAM below in this case.
    ## 3) Code in FLASH, stack + heap in external RAM (file *6m_xram.ld)
    ##    Same as above, but leaves the upper 2MB of RAM for the LCD.
    LINKER_SCRIPT_PATH := arch/cortexM4_stm32f4/stm32f429zi_stm32f4discovery/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+256k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+8m_xram.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+6m_xram.ld

    ## Uncommenting __ENABLE_XRAM enables the initialization of the external
    ## 8MB SDRAM memory. Do not uncomment this even if you don't use a linker
    ## script that requires it, as it is used for the LCD framebuffer.
    XRAM := -D__ENABLE_XRAM
    
    ## Select clock frequency. Warning: the default clock frequency for
    ## this board is 168MHz and not 180MHz because, due to a limitation in
    ## the PLL, it is not possible to generate a precise 48MHz output when
    ## running the core at 180MHz. If 180MHz is chosen the USB peripheral will
    ## NOT WORK and the SDIO and RNG will run ~6% slower (45MHz insteand of 48)
    #CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_180MHz=180000000
    CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000
    #CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000

endif

##---------------------------------------------------------------------------
## stm32f103cb_als_mainboard_rev2
##

# No options

##---------------------------------------------------------------------------
## stm32f100cb_tempsensor
##

# No options

##---------------------------------------------------------------------------
## stm32f429zi_oledboard2
##
ifeq ($(OPT_BOARD),stm32f429zi_oledboard2)

    ## Linker script type, there are three options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
    ##    You must uncomment -D__ENABLE_XRAM below in this case.
    ## 3) Code in FLASH, stack + heap in external RAM (file *6m_xram.ld)
    ##    Same as above, but leaves the upper 2MB of RAM for the LCD.
    LINKER_SCRIPT_PATH := arch/cortexM4_stm32f4/stm32f429zi_oledboard2/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+256k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+8m_xram.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+6m_xram.ld

    ## Uncommenting __ENABLE_XRAM enables the initialization of the external
    ## 8MB SDRAM memory. Do not uncomment this even if you don't use a linker
    ## script that requires it, as it is used for the LCD framebuffer.
    XRAM := -D__ENABLE_XRAM
    
    ## Select clock frequency. Warning: the default clock frequency for
    ## this board is 168MHz and not 180MHz because, due to a limitation in
    ## the PLL, it is not possible to generate a precise 48MHz output when
    ## running the core at 180MHz. If 180MHz is chosen the USB peripheral will
    ## NOT WORK and the SDIO and RNG will run ~6% slower (45MHz insteand of 48)
    CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000
    #CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_180MHz=180000000

endif

##---------------------------------------------------------------------------
## efm32gg332f1024_wandstem
##
ifeq ($(OPT_BOARD),efm32gg332f1024_wandstem)

    ## Linker script type, there are two options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ## 2) Same as 1) but space has been reserved for a process pool, allowing
    ##    to configure the kernel with "#define WITH_PROCESSES"
    LINKER_SCRIPT_PATH := arch/cortexM3_efm32gg/efm32gg332f1024_wandstem/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)efm32_1M+128k_rom_usbbootloader.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)efm32_1M+128k_rom_usbbootloader_processes.ld

endif

##---------------------------------------------------------------------------
## stm32f411re_nucleo
##
ifeq ($(OPT_BOARD),stm32f411re_nucleo)

    # Select clock frequency
    CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000
    #CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000

endif

##---------------------------------------------------------------------------
## stm32f429zi_skyward_anakin
##
ifeq ($(OPT_BOARD),stm32f429zi_skyward_anakin)

    ## Linker script type, there are three options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
    ##    You must uncomment -D__ENABLE_XRAM below in this case.
    LINKER_SCRIPT_PATH := arch/cortexM4_stm32f4/stm32f429zi_skyward_anakin/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+256k_rom.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+8m_xram.ld

    ## Uncommenting __ENABLE_XRAM enables the initialization of the external
    ## 8MB SDRAM memory. Do not uncomment this even if you don't use a linker
    ## script that requires it, as it is used for the LCD framebuffer.
    XRAM := -D__ENABLE_XRAM
    
    ## Select clock frequency.
    ## Warning: due to a limitation in the PLL, it is not possible to generate
    ## a precise 48MHz output when running the core at 180MHz. If 180MHz is
    ## chosen the SDIO and RNG will run ~6% slower (45MHz insteand of 48)
    CLOCK_FREQ := -DHSE_VALUE=25000000 -DSYSCLK_FREQ_180MHz=180000000
    #CLOCK_FREQ := -DHSE_VALUE=25000000 -DSYSCLK_FREQ_168MHz=168000000

endif

##---------------------------------------------------------------------------
## stm32f401vc_stm32f4discovery
##

# No options

##---------------------------------------------------------------------------
## stm32f103c8_breakout
##
ifeq ($(OPT_BOARD),stm32f103c8_breakout)
    
    ## Select clock frequency
    CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_36MHz=36000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_48MHz=48000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_56MHz=56000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_72MHz=72000000

endif

##---------------------------------------------------------------------------
## stm32f100c8_microboard
##

# No options

##---------------------------------------------------------------------------
## stm32f469ni_stm32f469i-disco
##
ifeq ($(OPT_BOARD),stm32f469ni_stm32f469i-disco)

    ## Linker script type, there are three options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in FLASH, stack + heap in external RAM (file *16m_xram.ld)
    ##    You must uncomment -D__ENABLE_XRAM below in this case.
    ## 3) Code in FLASH, stack + heap in external RAM (file *12m_xram.ld)
    ##    Same as above, but leaves the upper 4MB of RAM for the LCD.
    LINKER_SCRIPT_PATH := arch/cortexM4_stm32f4/stm32f469ni_stm32f469i-disco/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+384k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+16m_xram.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+12m_xram.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+16m_xram_processes.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+16m_xram_processes_and_kernel.ld

    ## Uncommenting __ENABLE_XRAM enables the initialization of the external
    ## 16MB SDRAM memory. Do not uncomment this even if you don't use a linker
    ## script that requires it, as it is used for the LCD framebuffer.
    XRAM := -D__ENABLE_XRAM
    
    ## Select clock frequency. Warning: the default clock frequency for
    ## this board is 168MHz and not 180MHz because, due to a limitation in
    ## the PLL, it is not possible to generate a precise 48MHz output when
    ## running the core at 180MHz. If 180MHz is chosen the USB peripheral will
    ## NOT WORK and the SDIO and RNG will run ~6% slower (45MHz insteand of 48)
    #CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_180MHz=180000000
    CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000
    #CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000

endif

##---------------------------------------------------------------------------
## stm32f429zi_skyward_homeone
##
ifeq ($(OPT_BOARD),stm32f429zi_skyward_homeone)

    ## Linker script type, there are three options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
    ##    You must uncomment -D__ENABLE_XRAM below in this case.
    LINKER_SCRIPT_PATH := arch/cortexM4_stm32f4/stm32f429zi_skyward_homeone/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+256k_rom.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+8m_xram.ld

    ## Uncommenting __ENABLE_XRAM enables the initialization of the external
    ## 8MB SDRAM memory.
    XRAM := -D__ENABLE_XRAM
    
    ## Select clock frequency. Warning: the default clock frequency for
    ## this board is 168MHz and not 180MHz because, due to a limitation in
    ## the PLL, it is not possible to generate a precise 48MHz output when
    ## running the core at 180MHz. If 180MHz is chosen the USB peripheral will
    ## NOT WORK and the SDIO and RNG will run ~6% slower (45MHz insteand of 48)
    #CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_180MHz=180000000
    CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000
    #CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000

endif

##---------------------------------------------------------------------------
## stm32f401re_nucleo
##

# No options

##---------------------------------------------------------------------------
## stm32f746zg_nucleo
##

ifeq ($(OPT_BOARD),stm32f746zg_nucleo)

    ## Linker script type, there are two options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ## 2) Same as 1) but space has been reserved for a process pool, allowing
    ##    to configure the kernel with "#define WITH_PROCESSES"
    LINKER_SCRIPT_PATH := arch/cortexM7_stm32f7/stm32f746zg_nucleo/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+256k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+320k_rom_processes.ld

endif

##---------------------------------------------------------------------------
## stm32h753xi_eval
##

ifeq ($(OPT_BOARD),stm32h753xi_eval)

    ## Linker script type, there are three options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in FLASH, stack + heap in external RAM (file *m_xram.ld)
    ##    You must uncomment -D__ENABLE_XRAM below in this case.
    LINKER_SCRIPT_PATH := arch/cortexM7_stm32h7/stm32h753xi_eval/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+512k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+32m_xram.ld

endif

##---------------------------------------------------------------------------
## stm32h723zg_nucleo
##

ifeq ($(OPT_BOARD),stm32h723zg_nucleo)

    ## Linker script type, there are three options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in FLASH, stack + heap in external RAM (file *m_xram.ld)
    ##    You must uncomment -D__ENABLE_XRAM below in this case.
    LINKER_SCRIPT_PATH := arch/cortexM7_stm32h7/stm32h723zg_nucleo/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_rom.ld
endif

##---------------------------------------------------------------------------
## stm32f407vg_thermal_test_chip
##

# No options

##---------------------------------------------------------------------------
## stm32f205_generic
##

# No options

##---------------------------------------------------------------------------
## stm32f103cx_generic
##

ifeq ($(OPT_BOARD),stm32f103cx_generic)
    
    # stm32f103c8 has 64K, stm32f103cb has 128K
    LINKER_SCRIPT_PATH := arch/cortexM3_stm32f1/stm32f103cx_generic/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_64k+20k_rom.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_128k+20k_rom.ld
    
    ## Select clock frequency
    CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_36MHz=36000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_48MHz=48000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_56MHz=56000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_72MHz=72000000

endif
##---------------------------------------------------------------------------
## stm32f072rb_stm32f0discovery
##

# Actually no options

##---------------------------------------------------------------------------
## stm32f100cx_generic
##

ifeq ($(OPT_BOARD),stm32f100cx_generic)

    # stm32f100c8 has 64K, stm32f100cb has 128K
    LINKER_SCRIPT_PATH := arch/cortexM3_stm32f1/stm32f100cx_generic/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_64k+8k_rom.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_128k+8k_rom.ld

    ## Select clock frequency
    CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000

endif

##---------------------------------------------------------------------------
## stm32f303vc_stm32f3discovery
##

ifeq ($(OPT_BOARD),stm32f303vc_stm32f3discovery)

    ## Select clock frequency
    ## Not defining any of these results in the internal 8MHz clock to be used
    #CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_36MHz=36000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_48MHz=48000000 -DRUN_WITH_HSI
    CLOCK_FREQ := -DSYSCLK_FREQ_56MHz=56000000 -DRUN_WITH_HSI

endif

##---------------------------------------------------------------------------
## stm32f100c8_vaisala_rs41
##

ifeq ($(OPT_BOARD),stm32f100c8_vaisala_rs41)

    ## Select clock frequency
    ## Not defining anything results in HSI being used
    #CLOCK_FREQ := -DSYSCLK_FREQ_8MHz=8000000
    CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000 -DHSE_VALUE=24000000

endif

##---------------------------------------------------------------------------
## stm32l476rg_nucleo
##

ifeq ($(OPT_BOARD),stm32l476rg_nucleo)

    ## Select clock frequency (16MHz HSI)
    #CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_36MHz=36000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_48MHz=48000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_56MHz=56000000 -DRUN_WITH_HSI
    CLOCK_FREQ := -DSYSCLK_FREQ_80MHz=80000000 -DRUN_WITH_HSI
    ## Select clock frequency (4MHz MSI)
    #CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_36MHz=36000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_48MHz=48000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_56MHz=56000000
    ## Other clock options
    #CLOCK_FREQ := -DRUN_WITH_MSI

endif

##---------------------------------------------------------------------------
## atsam4lc2aa_generic
##

ifeq ($(OPT_BOARD),atsam4lc2aa_generic)

    ## Linker script type, there are two options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ## 2) Same as 1) but space has been reserved for a process pool, allowing
    ##    to configure the kernel with "#define WITH_PROCESSES"
    LINKER_SCRIPT_PATH := arch/cortexM4_atsam4l/atsam4lc2aa_generic/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)atsam_112k+32k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)atsam_112k+32k_rom_processes.ld

endif

##---------------------------------------------------------------------------
## stm32f411ce_blackpill
##

ifeq ($(OPT_BOARD),stm32f411ce_blackpill)

    ## Linker script type, there are two options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ## 2) Same as 1) but space has been reserved for a process pool, allowing
    ##    to configure the kernel with "#define WITH_PROCESSES"
    LINKER_SCRIPT_PATH := arch/cortexM4_stm32f4/stm32f411ce_blackpill/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+128k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+128k_rom_processes.ld

endif

##---------------------------------------------------------------------------
## stm32l4r9zi_sensortile
##

ifeq ($(OPT_BOARD),stm32l4r9zi_sensortile)

    ## Linker script type, there are two options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ## 2) Same as 1) but space has been reserved for a process pool, allowing
    ##    to configure the kernel with "#define WITH_PROCESSES"
    LINKER_SCRIPT_PATH := arch/cortexM4_stm32l4/stm32l4r9zi_sensortile/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+640k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+640k_rom_processes.ld

    ## Select clock frequency (8MHz HSE)
    #CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000 -DHSE_VALUE=16000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_36MHz=36000000 -DHSE_VALUE=16000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_48MHz=48000000 -DHSE_VALUE=16000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_56MHz=56000000 -DHSE_VALUE=16000000
    CLOCK_FREQ := -DSYSCLK_FREQ_80MHz=80000000 -DHSE_VALUE=16000000
    ## Select clock frequency (16MHz HSI)
    #CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_36MHz=36000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_48MHz=48000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_56MHz=56000000 -DRUN_WITH_HSI
    #CLOCK_FREQ := -DSYSCLK_FREQ_80MHz=80000000 -DRUN_WITH_HSI
    ## Select clock frequency (4MHz MSI)
    #CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_36MHz=36000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_48MHz=48000000
    #CLOCK_FREQ := -DSYSCLK_FREQ_56MHz=56000000
    ## Other clock options
    #CLOCK_FREQ := -DHSE_VALUE=16000000
    #CLOCK_FREQ := -DRUN_WITH_MSI

endif

##---------------------------------------------------------------------------
## stm32f415vg_st25dvdiscovery
##

# No options

##---------------------------------------------------------------------------
## efm32g222f128_generic
##

# No options

##---------------------------------------------------------------------------
## stm32l053r8_nucleo
##

# No options

##---------------------------------------------------------------------------
## stm32f030r8_stm32f0discovery
##

# No options

##---------------------------------------------------------------------------
## stm32f765ii_marco_ram_board
##

ifeq ($(OPT_BOARD),stm32f765ii_marco_ram_board)

    ## Linker script type
    ## 1) stm32_2M+384k_ram.ld
    ##      - .text                      internal Flash
    ##      - IRQ stack                  internal SRAM
    ##      - .data, .bss                internal SRAM
    ##      - Heap (and thread stacks)   internal SRAM
    ##    The most common choice, available for all microcontrollers
    ## 2) stm32_2M+384k_xram-heap_256M.ld
    ##      - .text                      internal Flash
    ##      - IRQ stack                  internal SRAM
    ##      - .data, .bss                internal SRAM
    ##      - Heap (and thread stacks)   external SDRAM (bank 1)
    ##    For application with small static buffers. Only uses SDRAM bank 1.
    ## 3) stm32_2M+384k_xram_256M.ld
    ##      - .text                      internal Flash
    ##      - IRQ stack                  internal SRAM
    ##      - .data, .bss                external SDRAM (bank 1)
    ##      - Heap (and thread stacks)   external SDRAM (bank 1)
    ##    For application with large static buffers. Only uses SDRAM bank 1.
    ## 4) stm32_2m+384k_ram_processes.ld
    ##      - .text                      internal Flash
    ##      - IRQ stack                  internal SRAM (low 128K)
    ##      - .data, .bss                internal SRAM (low 128K)
    ##      - Heap (and thread stacks)   internal SRAM (low 128K)
    ##      - Process Pool               internal SRAM (high 256K)
    ## 5) stm32_2m+384k_xram_256M_processes.ld
    ##      - .text                      internal Flash
    ##      - IRQ stack                  internal SRAM
    ##      - .data, .bss                internal SRAM
    ##      - Heap (and thread stacks)   internal SRAM
    ##      - Process Pool               external SDRAM (bank 1)
    LINKER_SCRIPT_PATH := arch/cortexM7_stm32f7/stm32f765ii_marco_ram_board
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2M+384k_ram.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2M+384k_xram-heap_256M.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2M+384k_xram_256M.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2m+384k_ram_processes.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2m+384k_xram_256M_processes.ld

endif

##---------------------------------------------------------------------------
## rp2040_raspberry_pi_pico
##

ifeq ($(OPT_BOARD),rp2040_raspberry_pi_pico)

    # Board variant for selecting either the "plain" Pico or Pico W
    #BOARD_VARIANT := BOARD_VARIANT_PICO
    BOARD_VARIANT := BOARD_VARIANT_PICO_W

    # Clock frequency option. The Raspberry Pi Pico boards have a 12MHz
    # external oscillator.
    CLOCK_FREQ := -DXOSC_FREQ=12000000 -DCLK_SYS_FREQ=133000000
    #CLOCK_FREQ := -DXOSC_FREQ=12000000 -DCLK_SYS_FREQ=125000000

endif

##---------------------------------------------------------------------------
## stm32h755zi_nucleo
##

ifeq ($(OPT_BOARD),stm32h755zi_nucleo)

    ## Linker script type, there are two options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ## 2) Same as 1) but space has been reserved for a process pool, allowing
    ##    to configure the kernel with "#define WITH_PROCESSES"
    LINKER_SCRIPT_PATH := arch/cortexM7_stm32h7/stm32h755zi_nucleo/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1M+512k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1M+512k_rom_processes.ld

endif


############################################################################
## From the options selected above, now fill all the variables needed to  ##
## build Miosix. You should modify something here only if you are adding  ##
## a new board or porting Miosix to a new architecture                    ##
############################################################################

ifneq ($(MAKEFILE_VERSION),1.15)
    $(info You are using an incompatible makefile. Make sure it matches \
      the one distributed with the current version of the kernel)
    $(error Error)
endif

##
## First, auto guess architecture name from board name
##
ifeq ($(OPT_BOARD),lpc2138_miosix_board)
    ARCH := arm7_lpc2000
else ifeq ($(OPT_BOARD),stm32f103ze_stm3210e-eval)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f103ve_mp3v2)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f100rb_stm32vldiscovery)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f103ve_strive_mini)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f103ze_redbull_v2)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f407vg_stm32f4discovery)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f207ig_stm3220g-eval)
    ARCH := cortexM3_stm32f2
else ifeq ($(OPT_BOARD),stm32f207zg_ethboard_v2)
    ARCH := cortexM3_stm32f2
else ifeq ($(OPT_BOARD),stm32f207zg_nucleo)
    ARCH := cortexM3_stm32f2
else ifeq ($(OPT_BOARD),stm32f207ze_als_camboard)
    ARCH := cortexM3_stm32f2
else ifeq ($(OPT_BOARD),stm32f205rg_sony-newman)
    ARCH := cortexM3_stm32f2
else ifeq ($(OPT_BOARD),stm32l151_als_mainboard)
    ARCH := cortexM3_stm32l1
else ifeq ($(OPT_BOARD),stm32f407vg_bitsboard)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f429zi_stm32f4discovery)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f103cb_als_mainboard_rev2)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f100cb_tempsensor)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f429zi_oledboard2)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),efm32gg332f1024_wandstem)
    ARCH := cortexM3_efm32gg
else ifeq ($(OPT_BOARD),stm32f411re_nucleo)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f429zi_skyward_anakin)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f100rc_solertegiard)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f205rc_skyward_stormtrooper)
    ARCH := cortexM3_stm32f2
else ifeq ($(OPT_BOARD),stm32f401vc_stm32f4discovery)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f103c8_breakout)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f100c8_microboard)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f469ni_stm32f469i-disco)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f429zi_skyward_homeone)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f401re_nucleo)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f746zg_nucleo)
    ARCH := cortexM7_stm32f7
else ifeq ($(OPT_BOARD),stm32h753xi_eval)
    ARCH := cortexM7_stm32h7
else ifeq ($(OPT_BOARD),stm32h723zg_nucleo)
    ARCH := cortexM7_stm32h7
else ifeq ($(OPT_BOARD),stm32f407vg_thermal_test_chip)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f205_generic)
    ARCH := cortexM3_stm32f2
else ifeq ($(OPT_BOARD),stm32f103cx_generic)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f072rb_stm32f0discovery)
    ARCH := cortexM0_stm32f0
else ifeq ($(OPT_BOARD),stm32f100cx_generic)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32f303vc_stm32f3discovery)
    ARCH := cortexM4_stm32f3
else ifeq ($(OPT_BOARD),stm32f100c8_vaisala_rs41)
    ARCH := cortexM3_stm32f1
else ifeq ($(OPT_BOARD),stm32l476rg_nucleo)
    ARCH := cortexM4_stm32l4
else ifeq ($(OPT_BOARD),atsam4lc2aa_generic)
    ARCH := cortexM4_atsam4l
else ifeq ($(OPT_BOARD),stm32f411ce_blackpill)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32l4r9zi_sensortile)
    ARCH := cortexM4_stm32l4
else ifeq ($(OPT_BOARD),stm32f767zi_nucleo)
    ARCH := cortexM7_stm32f7
else ifeq ($(OPT_BOARD),stm32f769ni_discovery)
    ARCH := cortexM7_stm32f7
else ifeq ($(OPT_BOARD),stm32f415vg_st25dvdiscovery)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),efm32g222f128_generic)
    ARCH := cortexM3_efm32g
else ifeq ($(OPT_BOARD),stm32l053r8_nucleo)
    ARCH := cortexM0plus_stm32l0
else ifeq ($(OPT_BOARD),stm32f030r8_stm32f0discovery)
    ARCH := cortexM0_stm32f0
else ifeq ($(OPT_BOARD),stm32f765ii_marco_ram_board)
    ARCH := cortexM7_stm32f7
else ifeq ($(OPT_BOARD),rp2040_raspberry_pi_pico)
    ARCH := cortexM0plus_rp2040
else ifeq ($(OPT_BOARD),stm32h755zi_nucleo)
    ARCH := cortexM7_stm32h7
else
    $(info Error: no board specified in miosix/config/Makefile.inc)
    $(error Error)
endif


##
## Then, initialize C/C++ flags
##
CFLAGS_BASE   := -D_MIOSIX_BOARDNAME=\"$(OPT_BOARD)\" -D_DEFAULT_SOURCE=1 \
                 -ffunction-sections -Wall -Werror=return-type -g
CXXFLAGS_BASE := -D_MIOSIX_BOARDNAME=\"$(OPT_BOARD)\" -D_DEFAULT_SOURCE=1 \
                 -std=c++14 -ffunction-sections -Wall -Werror=return-type -g

##
## Now two big switch-like constructs nested. The first lists all possible
## architectures, setting common things for all boards in the architecture.
## Then for each architecture there is a switch for evry board, where all
## board specific options are set.
##

##-----------------------------------------------------------------------------
## ARCHITECTURE: arm7_lpc2000
##
ifeq ($(ARCH),arm7_lpc2000)
    ## Base directory with header files for this board
    ARCH_INC := arch/arm7_lpc2000/common

    ##-------------------------------------------------------------------------
    ## BOARD: lpc2138_miosix_board
    ##
    ifeq ($(OPT_BOARD),lpc2138_miosix_board)

        ## Base directory with header files for this board
        BOARD_INC := arch/arm7_lpc2000/lpc2138_miosix_board

        ## Select linker script
        LINKER_SCRIPT := arch/arm7_lpc2000/lpc2138_miosix_board/miosix.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.s             \
        $(BOARD_INC)/interfaces-impl/portability.cpp \
        $(BOARD_INC)/interfaces-impl/os_timer.cpp    \
        arch/common/drivers/sd_lpc2000.cpp           \
        $(BOARD_INC)/interfaces-impl/delays.cpp      \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_MIOSIX_BOARD
        CXXFLAGS_BASE += -D_BOARD_MIOSIX_BOARD
        
    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture does not support processes
    #POSTLD :=

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=arm7tdmi
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_ARM7_LPC2000 $(CPU) $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_ARM7_LPC2000 $(CPU) $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                      $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                  \
    arch/common/core/interrupts_arm7.cpp         \
    arch/common/drivers/serial_lpc2000.cpp

    ## Select programmer command line
    ## This is the program that is invoked when the user types 'make program'
    ## The command must provide a way to program the board, or print an error
    ## message saying that 'make program' is not supported for that board.
    PROG ?= lpc21isp -verify main.hex /dev/ttyUSB0 115200 14746

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM3_stm32f1
##
else ifeq ($(ARCH),cortexM3_stm32f1)
    ## Base directory with header files for this board
    ARCH_INC := arch/cortexM3_stm32f1/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f103ze_stm3210e-eval
    ##
    ifeq ($(OPT_BOARD),stm32f103ze_stm3210e-eval)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f103ze_stm3210e-eval

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/sd_stm32f1.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM3210E_EVAL -DSTM32F103xE
        CXXFLAGS_BASE += -D_BOARD_STM3210E_EVAL -DSTM32F103xE

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        ifeq ($(LINKER_SCRIPT),$(LINKER_SCRIPT_PATH)stm32_512k+64k_all_in_xram.ld)
            PROG ?= $(KPATH)/_tools/bootloaders/stm32/pc_loader/pc_loader \
                    /dev/ttyUSB0 $(if $(ROMFS_DIR), image.bin, main.bin)
        else
            PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
        endif

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f103ve_mp3v2
    ##
    else ifeq ($(OPT_BOARD),stm32f103ve_mp3v2)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f103ve_mp3v2

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/sd_stm32f1.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_MP3V2 -DSTM32F103xE
        CXXFLAGS_BASE += -D_BOARD_MP3V2 -DSTM32F103xE

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_72MHz=72000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        ifeq ($(LINKER_SCRIPT),$(LINKER_SCRIPT_PATH)stm32_512k+64k_ram.ld)
            PROG ?= mp3v2_bootloader --ram main.bin
        else
            PROG ?= mp3v2_bootloader --code $(if $(ROMFS_DIR), image.bin, main.bin)
        endif

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f100rb_stm32vldiscovery
    ##
    else ifeq ($(OPT_BOARD),stm32f100rb_stm32vldiscovery)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f100rb_stm32vldiscovery

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_128k+8k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp         \
        arch/common/drivers/stm32_rtc.cpp            \
        arch/common/drivers/servo_stm32.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32VLDISCOVERY -DSTM32F100xB
        CXXFLAGS_BASE += -D_BOARD_STM32VLDISCOVERY -DSTM32F100xB

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
        
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f100rc_solertegiard
    ##
    else ifeq ($(OPT_BOARD),stm32f100rc_solertegiard)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f100rc_solertegiard

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_256k+24k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp         \
        arch/common/drivers/servo_stm32.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_SOLERTEGIARD -DSTM32F100xE
        CXXFLAGS_BASE += -D_BOARD_SOLERTEGIARD -DSTM32F100xE

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
            

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f103ve_strive_mini
    ##
    else ifeq ($(OPT_BOARD),stm32f103ve_strive_mini)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f103ve_strive_mini

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_512k+64k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/sd_stm32f1.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STRIVE_MINI -DSTM32F103xE
        CXXFLAGS_BASE += -D_BOARD_STRIVE_MINI -DSTM32F103xE

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_72MHz=72000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: HY RedBull V2 (or V1)
    ##
    else ifeq ($(OPT_BOARD),stm32f103ze_redbull_v2)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f103ze_redbull_v2

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_512k+64k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/sd_stm32f1.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_REDBULL_V2 -DSTM32F103xE
        CXXFLAGS_BASE += -D_BOARD_REDBULL_V2 -DSTM32F103xE

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_72MHz=72000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
        
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f103cb_als_mainboard_rev2
    ##
    else ifeq ($(OPT_BOARD),stm32f103cb_als_mainboard_rev2)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f103cb_als_mainboard_rev2

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_128k+20k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_ALS_MAINBOARD_REV2 -DSTM32F103xB
        CXXFLAGS_BASE += -D_BOARD_ALS_MAINBOARD_REV2 -DSTM32F103xB

        ## Clock frequency
        # Not defining anything results in HSI being used
        CLOCK_FREQ :=

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
    
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f100cb_tempsensor
    ##
    else ifeq ($(OPT_BOARD),stm32f100cb_tempsensor)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f100cb_tempsensor

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_127k+8k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_TEMPSENSOR -DSTM32F100xB
        CXXFLAGS_BASE += -D_BOARD_TEMPSENSOR -DSTM32F100xB

        ## Clock frequency
        # Not defining anything results in HSI being used
        CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
        
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f103c8_breakout
    ##
    else ifeq ($(OPT_BOARD),stm32f103c8_breakout)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f103c8_breakout

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_64k+20k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp         \
        arch/common/drivers/servo_stm32.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F103C8_BREAKOUT -DSTM32F103xB
        CXXFLAGS_BASE += -D_BOARD_STM32F103C8_BREAKOUT -DSTM32F103xB
       
        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
        
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f100c8_microboard
    ##
    else ifeq ($(OPT_BOARD),stm32f100c8_microboard)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f100c8_microboard

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_63k+8k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp         \
        arch/common/drivers/stm32_rtc.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_MICROBOARD -DSTM32F100xB
        CXXFLAGS_BASE += -D_BOARD_MICROBOARD -DSTM32F100xB

        ## Clock frequency
        # Not defining anything results in HSI being used
        CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
        
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f103cx_generic
    ##
    else ifeq ($(OPT_BOARD),stm32f103cx_generic)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f103cx_generic

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp         \
        arch/common/drivers/stm32_rtc.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F103CX_GENERIC -DSTM32F103xB
        CXXFLAGS_BASE += -D_BOARD_STM32F103CX_GENERIC -DSTM32F103xB

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f100cx_generic
    ##
    else ifeq ($(OPT_BOARD),stm32f100cx_generic)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f100cx_generic

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp         \
        arch/common/drivers/stm32_rtc.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F100CX_GENERIC -DSTM32F100xB
        CXXFLAGS_BASE += -D_BOARD_STM32F100CX_GENERIC -DSTM32F100xB

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
    
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f100c8_vaisala_rs41
    ##
    else ifeq ($(OPT_BOARD),stm32f100c8_vaisala_rs41)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f1/stm32f100c8_vaisala_rs41

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_64k+8k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_VAISALA_RS41 -DSTM32F100xB
        CXXFLAGS_BASE += -D_BOARD_VAISALA_RS41 -DSTM32F100xB

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture supports processes
    POSTLD := mx-postlinker

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=cortex-m3 -mthumb
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM3_STM32F1 $(CLOCK_FREQ) $(XRAM) $(CPU) \
                     $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM3_STM32F1 $(CLOCK_FREQ) $(XRAM) $(CPU) \
                     $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                  \
    arch/common/core/interrupts_cortexMx.cpp     \
    arch/common/core/mpu_cortexMx.cpp            \
    arch/common/drivers/serial_stm32.cpp         \
    arch/common/drivers/dcc.cpp                  \
    $(ARCH_INC)/interfaces-impl/portability.cpp  \
    $(ARCH_INC)/interfaces-impl/delays.cpp       \
    $(ARCH_INC)/interfaces-impl/deep_sleep.cpp   \
    arch/common/drivers/stm32f1_gpio.cpp         \
    arch/common/core/stm32_16bit_os_timer.cpp    \
    arch/common/core/stm32_rtc_os_timer.cpp      \
    arch/common/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM4_stm32f4
##
else ifeq ($(ARCH),cortexM4_stm32f4)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM4_stm32f4/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f407vg_stm32f4discovery
    ##
    ifeq ($(OPT_BOARD),stm32f407vg_stm32f4discovery)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f407vg_stm32f4discovery

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32f2_f4_i2c.cpp       \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        arch/common/drivers/servo_stm32.cpp          \
        $(BOARD_INC)/drivers/rtc.cpp                 \
        $(BOARD_INC)/interfaces-impl/deep_sleep.cpp  \
        $(BOARD_INC)/interfaces-impl/bsp.cpp    

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F4DISCOVERY 
        CXXFLAGS_BASE += -D_BOARD_STM32F4DISCOVERY

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f4bitsboard
    ##
    else ifeq ($(OPT_BOARD),stm32f407vg_bitsboard)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f407vg_bitsboard

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_1m+192k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_BITSBOARD
        CXXFLAGS_BASE += -D_BOARD_BITSBOARD

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
        
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f429zi_stm32f4discovery
    ##
    else ifeq ($(OPT_BOARD),stm32f429zi_stm32f4discovery)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f429zi_stm32f4discovery

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32f2_f4_i2c.cpp       \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F429ZI_STM32F4DISCOVERY
        CXXFLAGS_BASE += -D_BOARD_STM32F429ZI_STM32F4DISCOVERY

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f429zi_oledboard2
    ##
    else ifeq ($(OPT_BOARD),stm32f429zi_oledboard2)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f429zi_oledboard2

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F429ZI_OLEDBOARD2
        CXXFLAGS_BASE += -D_BOARD_STM32F429ZI_OLEDBOARD2

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f411re_nucleo
    ##
    else ifeq ($(OPT_BOARD),stm32f411re_nucleo)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f411re_nucleo

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_512k+128k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F411RE_NUCLEO
        CXXFLAGS_BASE += -D_BOARD_STM32F411RE_NUCLEO

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## stm32f429zi_skyward_anakin
    ##
    else ifeq ($(OPT_BOARD),stm32f429zi_skyward_anakin)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f429zi_skyward_anakin

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32f2_f4_i2c.cpp       \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        arch/common/drivers/stm32_sgm.cpp            \
        arch/common/drivers/stm32_wd.cpp             \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F429ZI_SKYWARD_ANAKIN
        CXXFLAGS_BASE += -D_BOARD_STM32F429ZI_SKYWARD_ANAKIN

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

    ##-------------------------------------------------------------------------
    ## stm32f401vc_stm32f4discovery
    ##
    else ifeq ($(OPT_BOARD),stm32f401vc_stm32f4discovery)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f401vc_stm32f4discovery

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_256k+64k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32f2_f4_i2c.cpp       \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F401VC_STM32F4DISCOVERY
        CXXFLAGS_BASE += -D_BOARD_STM32F401VC_STM32F4DISCOVERY

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f469ni_stm32f469i-disco
    ##
    else ifeq ($(OPT_BOARD),stm32f469ni_stm32f469i-disco)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f469ni_stm32f469i-disco

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32f2_f4_i2c.cpp       \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F469NI_STM32F469I_DISCO
        CXXFLAGS_BASE += -D_BOARD_STM32F469NI_STM32F469I_DISCO

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f429zi_skyward_homeone
    ##
    else ifeq ($(OPT_BOARD),stm32f429zi_skyward_homeone)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f429zi_skyward_homeone

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32f2_f4_i2c.cpp       \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        arch/common/drivers/stm32_sgm.cpp            \
        arch/common/drivers/stm32_wd.cpp             \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F429ZI_SKYWARD_HOMEONE
        CXXFLAGS_BASE += -D_BOARD_STM32F429ZI_SKYWARD_HOMEONE

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f401re_nucleo
    ##
    else ifeq ($(OPT_BOARD),stm32f401re_nucleo)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f401re_nucleo

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_512k+96k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F401RE_NUCLEO
        CXXFLAGS_BASE += -D_BOARD_STM32F401RE_NUCLEO

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
        
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f407vg_thermal_test_chip
    ##
    else ifeq ($(OPT_BOARD),stm32f407vg_thermal_test_chip)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f407vg_thermal_test_chip

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_1m+192k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_THERMALTESTCHIP
        CXXFLAGS_BASE += -D_BOARD_THERMALTESTCHIP

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f411ce_blackpill
    ##
    else ifeq ($(OPT_BOARD),stm32f411ce_blackpill)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f411ce_blackpill

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F411CE_BLACKPILL
        CXXFLAGS_BASE += -D_BOARD_STM32F411CE_BLACKPILL

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=25000000 -DSYSCLK_FREQ_100MHz=100000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D \
                $(if $(ROMFS_DIR), image.bin, main.bin)

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f415vg_st25dvdiscovery
    ##
    else ifeq ($(OPT_BOARD),stm32f415vg_st25dvdiscovery)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f415vg_st25dvdiscovery

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_1m+192k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F415VG_ST25DVDISCOVERY
        CXXFLAGS_BASE += -D_BOARD_STM32F415VG_ST25DVDISCOVERY

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture supports processes
    POSTLD := mx-postlinker

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM4_STM32F4 $(CLOCK_FREQ) $(XRAM) \
                     $(SRAM_BOOT) $(CPU) $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM4_STM32F4 $(CLOCK_FREQ) $(XRAM) \
                     $(SRAM_BOOT) $(CPU) $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/core/mpu_cortexMx.cpp                        \
    arch/common/drivers/serial_stm32.cpp                     \
    arch/common/drivers/dcc.cpp                              \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/drivers/stm32_gpio.cpp                       \
    arch/common/drivers/sd_stm32f2_f4_f7.cpp                 \
    arch/common/core/stm32_32bit_os_timer.cpp                \
    arch/common/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM3_stm32f2
##
else ifeq ($(ARCH),cortexM3_stm32f2)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM3_stm32f2/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f207ig_stm3220g-eval
    ##
    ifeq ($(OPT_BOARD),stm32f207ig_stm3220g-eval)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f2/stm32f207ig_stm3220g-eval

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/sd_stm32f2_f4_f7.cpp     \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM3220G_EVAL
        CXXFLAGS_BASE += -D_BOARD_STM3220G_EVAL

        ## Clock frequency
        CLOCK_FREQ := -DHSE_VALUE=25000000 -DSYSCLK_FREQ_120MHz=120000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        ifeq ($(LINKER_SCRIPT),$(LINKER_SCRIPT_PATH)stm32_1m+128k_all_in_xram.ld)
            PROG ?= $(KPATH)/_tools/bootloaders/stm32/pc_loader/pc_loader \
                    /dev/ttyUSB0 main.bin
        else
            PROG ?= st-flash --connect-under-reset --reset write \
                    $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
        endif

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f207zg_ethboard_v2
    ##
    else ifeq ($(OPT_BOARD),stm32f207zg_ethboard_v2)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f2/stm32f207zg_EthBoardV2

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/sd_stm32f2_f4_f7.cpp     \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_ETHBOARDV2
        CXXFLAGS_BASE += -D_BOARD_ETHBOARDV2

        ## Clock frequency
        CLOCK_FREQ := -DHSE_VALUE=25000000 -DSYSCLK_FREQ_120MHz=120000000

        ## XRAM is always enabled in this board
        XRAM += -D__ENABLE_XRAM

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        ifeq ($(LINKER_SCRIPT),$(LINKER_SCRIPT_PATH)stm32_1m+128k_code_in_xram.ld)
            PROG ?= $(KPATH)/_tools/bootloaders/stm32/pc_loader/pc_loader \
                    /dev/ttyUSB0 main.bin
        else
            PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
        endif

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f207zg_nucleo
    ##
    else ifeq ($(OPT_BOARD),stm32f207zg_nucleo)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f2/stm32f207zg_nucleo

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_1m+128k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/sd_stm32f2_f4_f7.cpp     \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F207ZG_NUCLEO
        CXXFLAGS_BASE += -D_BOARD_STM32F207ZG_NUCLEO

        ## Clock frequency
        ## External clock is 8Mhz provided from MCO output of ST-LINK. Actual
        ## HSE crystal (X3) is not fitted.
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_120MHz=120000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f207ze_als_camboard
    ##
    else ifeq ($(OPT_BOARD),stm32f207ze_als_camboard)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f2/stm32f207ze_als_camboard

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_1m+128k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_ALS_CAMBOARD
        CXXFLAGS_BASE += -D_BOARD_ALS_CAMBOARD

        ## Clock frequency
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_120MHz=120000000

        ## XRAM is always enabled in this board
        XRAM += -D__ENABLE_XRAM

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f205rg_sony-newman
    ##
    else ifeq ($(OPT_BOARD),stm32f205rg_sony-newman)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f2/stm32f205rg_sony-newman

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_1M+128k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32f2_f4_i2c.cpp       \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_SONY_NEWMAN
        CXXFLAGS_BASE += -D_BOARD_SONY_NEWMAN

        ## Clock frequency
        CLOCK_FREQ := -DHSE_VALUE=26000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        ## The magic.bin is somewhat used by the bootloader to detect a good fw
        PROG ?= perl -e 'print "\xe7\x91\x11\xc0"' > magic.bin; \
                dfu-util -d 0fce:f0fa -a 0 -i 0 -s 0x08040000 -D main.bin;  \
                dfu-util -d 0fce:f0fa -a 0 -i 0 -s 0x080ffffc -D magic.bin; \
                rm magic.bin
            
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f205rc_skyward_stormtrooper
    ##
    else ifeq ($(OPT_BOARD),stm32f205rc_skyward_stormtrooper)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f2/stm32f205rc_skyward_stormtrooper

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_512k+128k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32f2_f4_i2c.cpp       \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F205RC_SKYWARD_STORMTROOPER
        CXXFLAGS_BASE += -D_BOARD_STM32F205RC_SKYWARD_STORMTROOPER

        ## Clock frequency
        CLOCK_FREQ := -DHSE_VALUE=25000000 -DSYSCLK_FREQ_120MHz=120000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f205_generic
    ##
    else ifeq ($(OPT_BOARD),stm32f205_generic)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f2/stm32f205_generic

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_1m+128k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/sd_stm32f2_f4_f7.cpp     \
        arch/common/drivers/stm32f2_f4_i2c.cpp       \
        arch/common/drivers/servo_stm32.cpp          \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F205_GENERIC
        CXXFLAGS_BASE += -D_BOARD_STM32F205_GENERIC

        ## Clock frequency
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_120MHz=120000000
        
        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture supports processes
    POSTLD := mx-postlinker

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=cortex-m3 -mthumb
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM3_STM32F2 $(CLOCK_FREQ) $(XRAM) $(CPU) \
                     $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM3_STM32F2 $(CLOCK_FREQ) $(XRAM) $(CPU) \
                     $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map         \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT)  \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/core/mpu_cortexMx.cpp                        \
    arch/common/drivers/serial_stm32.cpp                     \
    arch/common/drivers/dcc.cpp                              \
    arch/common/drivers/stm32_hardware_rng.cpp               \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/drivers/stm32_gpio.cpp                       \
    arch/common/core/stm32_32bit_os_timer.cpp                \
    arch/common/CMSIS/Device/ST/STM32F2xx/Source/Templates/system_stm32f2xx.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM3_stm32l1
##
else ifeq ($(ARCH),cortexM3_stm32l1)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM3_stm32l1/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32l151c8_als_mainboard
    ##
    ifeq ($(OPT_BOARD),stm32l151_als_mainboard)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32l1/stm32l151c8_als_mainboard

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_64k+10k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_ALS_MAINBOARD -DSTM32L151xB
        CXXFLAGS_BASE += -D_BOARD_ALS_MAINBOARD -DSTM32L151xB

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_16MHz=16000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture does not support processes
    #POSTLD :=

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=cortex-m3 -mthumb
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM3_STM32L1 $(CLOCK_FREQ) $(XRAM) $(CPU) \
                     $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM3_STM32L1 $(CLOCK_FREQ) $(XRAM) $(CPU) \
                     $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map         \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT)  \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/drivers/serial_stm32.cpp                     \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    arch/common/drivers/stm32_gpio.cpp                       \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/core/stm32_16bit_os_timer.cpp                \
    arch/common/CMSIS/Device/ST/STM32L1xx/Source/Templates/system_stm32l1xx.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM3_efm32gg
##
else ifeq ($(ARCH),cortexM3_efm32gg)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM3_efm32gg/common

    ##-------------------------------------------------------------------------
    ## BOARD: efm32gg332f1024_wandstem
    ##
    ifeq ($(OPT_BOARD),efm32gg332f1024_wandstem)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_efm32gg/efm32gg332f1024_wandstem

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                                 \
        $(BOARD_INC)/core/stage_1_boot.cpp                          \
        $(BOARD_INC)/interfaces-impl/bsp.cpp                        \
        $(BOARD_INC)/interfaces-impl/spi.cpp                        \
        $(BOARD_INC)/interfaces-impl/power_manager.cpp              \
        $(BOARD_INC)/interfaces-impl/os_timer.cpp                   \
        $(BOARD_INC)/interfaces-impl/timer_interface.cpp            \
        $(BOARD_INC)/interfaces-impl/rtc.cpp                        \
        $(BOARD_INC)/interfaces-impl/hrtb.cpp                       \
        $(BOARD_INC)/interfaces-impl/gpio_timer.cpp                 \
        $(BOARD_INC)/interfaces-impl/transceiver_timer.cpp          \
        $(BOARD_INC)/interfaces-impl/gpioirq.cpp                    \
        $(BOARD_INC)/interfaces-impl/transceiver.cpp                \
        $(BOARD_INC)/interfaces-impl/flopsync_vht.cpp               \
        $(BOARD_INC)/interfaces-impl/vht.cpp                        \
        $(BOARD_INC)/interfaces-impl/virtual_clock.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -DEFM32GG332F1024 -D_BOARD_WANDSTEM
        CXXFLAGS_BASE += -DEFM32GG332F1024 -D_BOARD_WANDSTEM

        ## Clock frequency
        CLOCK_FREQ := -DEFM32_HFXO_FREQ=48000000 -DEFM32_LFXO_FREQ=32768

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= wandstem-flash -m u -f $(if $(ROMFS_DIR), image.bin, main.bin)

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture supports processes
    POSTLD := mx-postlinker

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=cortex-m3 -mthumb
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM3_EFM32GG $(CLOCK_FREQ) $(CPU) \
                     $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM3_EFM32GG $(CLOCK_FREQ) $(CPU) \
                     $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/core/mpu_cortexMx.cpp                        \
    arch/common/drivers/efm32_serial.cpp                     \
    arch/common/drivers/efm32_adc.cpp                        \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    arch/common/drivers/efm32_gpio.cpp                       \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/CMSIS/Device/SiliconLabs/EFM32GG/Source/system_efm32gg.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM7_stm32f7
##
else ifeq ($(ARCH),cortexM7_stm32f7)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM7_stm32f7/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f746zg_nucleo
    ##
    ifeq ($(OPT_BOARD),stm32f746zg_nucleo)
        ## The stm32f746 lacks the double precision FPU, so we will build for m4
        CPU := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
    
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM7_stm32f7/stm32f746zg_nucleo

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F746ZG_NUCLEO
        CXXFLAGS_BASE += -D_BOARD_STM32F746ZG_NUCLEO

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_216MHz=216000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f767zi_nucleo
    ##
    else ifeq ($(OPT_BOARD),stm32f767zi_nucleo)
        ## The stm32f767 has the double precision FPU, so we will build for m7
        CPU := -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16
    
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM7_stm32f7/stm32f767zi_nucleo

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_2m+384k_ram.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                           \
        $(BOARD_INC)/core/stage_1_boot.cpp    \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F767ZI_NUCLEO
        CXXFLAGS_BASE += -D_BOARD_STM32F767ZI_NUCLEO

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_216MHz=216000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f769ni_discovery
    ##
    else ifeq ($(OPT_BOARD),stm32f769ni_discovery)
        ## The stm32f769 has the double precision FPU, so we will build for m7
        CPU := -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16
    
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM7_stm32f7/stm32f769ni_discovery

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_2m+16m_xram.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                           \
        $(BOARD_INC)/core/stage_1_boot.cpp    \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F769NI_DISCO
        CXXFLAGS_BASE += -D_BOARD_STM32F769NI_DISCO

        ## Enables the initialization of the external 16MB SDRAM memory
        XRAM := -D__ENABLE_XRAM

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=25000000 -DSYSCLK_FREQ_216MHz=216000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
    
    ##-------------------------------------------------------------------------
    ## BOARD: stm32f765ii_marco_ram_board
    ##
    else ifeq ($(OPT_BOARD),stm32f765ii_marco_ram_board)
        ## The stm32f767 has the double precision FPU, so we will build for m7
        CPU := -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16
    
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM7_stm32f7/stm32f765ii_marco_ram_board

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                           \
        $(BOARD_INC)/core/stage_1_boot.cpp    \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F765II_MARCO_RAM_BOARD
        CXXFLAGS_BASE += -D_BOARD_STM32F765II_MARCO_RAM_BOARD

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=25000000 -DSYSCLK_FREQ_216MHz=216000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG_BIN = $(if $(ROMFS_DIR),image.bin,main.bin)
        PROG ?= openocd \
                -f '$(KPATH)/$(BOARD_INC)/openocd.cfg' \
                -c 'program $(PROG_BIN) 0x8000000 verify reset' \
                -c shutdown

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture supports processes
    POSTLD := mx-postlinker

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM7_STM32F7 $(CLOCK_FREQ) $(XRAM) \
                     $(SRAM_BOOT) $(CPU) $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM7_STM32F7 $(CLOCK_FREQ) $(XRAM) \
                     $(SRAM_BOOT) $(CPU) $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map         \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT)  \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/core/mpu_cortexMx.cpp                        \
    arch/common/core/cache_cortexMx.cpp                      \
    arch/common/drivers/serial_stm32.cpp                     \
    arch/common/drivers/sd_stm32f2_f4_f7.cpp                 \
    arch/common/drivers/dcc.cpp                              \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/drivers/stm32_gpio.cpp                       \
    arch/common/core/stm32_32bit_os_timer.cpp                \
    arch/common/CMSIS/Device/ST/STM32F7xx/Source/Templates/system_stm32f7xx.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM7_stm32h7
##
else ifeq ($(ARCH),cortexM7_stm32h7)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM7_stm32h7/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32h753xi_eval
    ##
    ifeq ($(OPT_BOARD),stm32h753xi_eval)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM7_stm32h7/stm32h753xi_eval

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32H753XI_EVAL
        CXXFLAGS_BASE += -D_BOARD_STM32H753XI_EVAL

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=25000000 -DSYSCLK_FREQ_400MHz=400000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32h723zg_nucleo
    ##
    else ifeq ($(OPT_BOARD),stm32h723zg_nucleo)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM7_stm32h7/stm32h723zg_nucleo

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32H723ZG_NUCLEO
        CXXFLAGS_BASE += -D_BOARD_STM32H723ZG_NUCLEO

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_550MHz=550000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32h755zi_nucleo
    ##
    else ifeq ($(OPT_BOARD),stm32h755zi_nucleo)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM7_stm32h7/stm32h755zi_nucleo

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        arch/common/drivers/stm32_hardware_rng.cpp   \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32H755ZI_NUCLEO
        CXXFLAGS_BASE += -D_BOARD_STM32H755ZI_NUCLEO

        ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
        ## While the chip can run up to 480MHz, the board uses the SMPS feature
        ## that limits clock speed to 400MHz
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_400MHz=400000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture supports processes
    POSTLD := mx-postlinker

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM7_STM32H7 $(CLOCK_FREQ) $(XRAM) \
                     $(SRAM_BOOT) $(CPU) $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM7_STM32H7 $(CLOCK_FREQ) $(XRAM) \
                     $(SRAM_BOOT) $(CPU) $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map         \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT)  \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/core/mpu_cortexMx.cpp                        \
    arch/common/drivers/serial_stm32.cpp                     \
    arch/common/drivers/sd_stm32h7.cpp                       \
    arch/common/core/cache_cortexMx.cpp                      \
    arch/common/drivers/serial_stm32.cpp                     \
    arch/common/drivers/dcc.cpp                              \
    $(ARCH_INC)/drivers/pll.cpp                              \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/drivers/stm32_gpio.cpp                       \
    arch/common/core/stm32_32bit_os_timer.cpp                \
    arch/common/CMSIS/Device/ST/STM32H7xx/Source/Templates/system_stm32h7xx.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM0_stm32f0
##
else ifeq ($(ARCH),cortexM0_stm32f0)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM0_stm32f0/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f072rb_stm32f0discovery
    ##
    ifeq ($(OPT_BOARD),stm32f072rb_stm32f0discovery)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM0_stm32f0/stm32f072rb_stm32f0discovery

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_128k+16k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp         \
        arch/common/core/stm32_32bit_os_timer.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F072RB_DISCO -DSTM32F072xB
        CXXFLAGS_BASE += -D_BOARD_STM32F072RB_DISCO -DSTM32F072xB

        ## Select clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_32MHz=32000000 -DRUN_WITH_HSI

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f030r8_stm32f0discovery
    ##
    else ifeq ($(OPT_BOARD),stm32f030r8_stm32f0discovery)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM0_stm32f0/stm32f030r8_stm32f0discovery

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_64k+8k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp         \
        arch/common/core/stm32_16bit_os_timer.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F030R8_DISCO -DSTM32F030x8
        CXXFLAGS_BASE += -D_BOARD_STM32F030R8_DISCO -DSTM32F030x8

        ## Select clock frequency
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_32MHz=32000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture does not support processes
    #POSTLD :=

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=cortex-m0 -mthumb
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM0_STM32F0 $(CLOCK_FREQ) $(CPU) \
                     $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM0_STM32F0 $(CLOCK_FREQ) $(CPU) \
                     $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common    

    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/drivers/serial_stm32.cpp                     \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/drivers/stm32_gpio.cpp                       \
    arch/common/CMSIS/Device/ST/STM32F0xx/Source/Templates/system_stm32f0xx.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM4_stm32f3
##
else ifeq ($(ARCH),cortexM4_stm32f3)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM4_stm32f3/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f303vc_stm32f3discovery
    ##
    ifeq ($(OPT_BOARD),stm32f303vc_stm32f3discovery)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f3/stm32f303vc_stm32f3discovery

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_256k+48k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32F3DISCOVERY
        CXXFLAGS_BASE += -D_BOARD_STM32F3DISCOVERY

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture supports processes
    POSTLD := mx-postlinker

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM4_STM32F3 $(CLOCK_FREQ) $(XRAM) $(CPU) \
                     $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM4_STM32F3 $(CLOCK_FREQ) $(XRAM) $(CPU) \
                     $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map                \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT)         \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/core/mpu_cortexMx.cpp                        \
    arch/common/drivers/serial_stm32.cpp                     \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    arch/common/drivers/stm32_gpio.cpp                       \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/core/stm32_32bit_os_timer.cpp                \
    arch/common/CMSIS/Device/ST/STM32F3xx/Source/Templates/system_stm32f3xx.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM4_stm32l4
##
else ifeq ($(ARCH),cortexM4_stm32l4)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM4_stm32l4/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32l476rg_nucleo
    ##
    ifeq ($(OPT_BOARD),stm32l476rg_nucleo)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32l4/stm32l476rg_nucleo

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_1m+128k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32L476RG_NUCLEO
        CXXFLAGS_BASE += -D_BOARD_STM32L476RG_NUCLEO

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## BOARD: stm32l4r9zi_sensortile
    ##
    else ifeq ($(OPT_BOARD),stm32l4r9zi_sensortile)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32l4/stm32l4r9zi_sensortile

        ## Select linker script
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp         \
        arch/common/drivers/sd_stm32l4.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32L4R9ZI_SENSORTILE
        CXXFLAGS_BASE += -D_BOARD_STM32L4R9ZI_SENSORTILE

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture supports processes
    POSTLD := mx-postlinker

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM4_STM32L4 $(CLOCK_FREQ) $(XRAM) $(CPU) \
                     $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM4_STM32L4 $(CLOCK_FREQ) $(XRAM) $(CPU) \
                     $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map                \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT)         \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/core/mpu_cortexMx.cpp                        \
    arch/common/drivers/serial_stm32.cpp                     \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    arch/common/drivers/stm32_gpio.cpp                       \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/core/stm32_32bit_os_timer.cpp                \
    arch/common/CMSIS/Device/ST/STM32L4xx/Source/Templates/system_stm32l4xx.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM4_atsam4l
##
else ifeq ($(ARCH),cortexM4_atsam4l)
    ## Base directory with header files for this board
    ARCH_INC := arch/cortexM4_atsam4l/common

    ##-------------------------------------------------------------------------
    ## BOARD: atsam4lc2aa_generic
    ##
    ifeq ($(OPT_BOARD),atsam4lc2aa_generic)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_atsam4l/atsam4lc2aa_generic

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/stage_1_boot.cpp           \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_ATSAM4LC2AA_GENERIC
        CXXFLAGS_BASE += -D_BOARD_ATSAM4LC2AA_GENERIC

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        ## BUG: flashes ok but requires powercycle
        PROG_BIN = $(if $(ROMFS_DIR),image.bin,main.bin)
        PROG ?= openocd \
                -f '$(KPATH)/$(BOARD_INC)/openocd.cfg' \
                -c 'program $(PROG_BIN) 0x4000 reset'  \
                -c shutdown

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture supports processes
    POSTLD := mx-postlinker

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    ## NOTE: although the atsam4l are cortex M4, they have no FPU, so the
    ## closest multilib we have is cortex M3, and that's basically what they are
    CPU := -mcpu=cortex-m3 -mthumb
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM4_ATSAM4L $(CPU) $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM4_ATSAM4L $(CPU) $(OPT_OPTIMIZATION) \
                     $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map         \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT)  \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/core/mpu_cortexMx.cpp                        \
    arch/common/drivers/serial_atsam4l.cpp                   \
    $(ARCH_INC)/drivers/clock.cpp                            \
    $(ARCH_INC)/drivers/lcd.cpp                              \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/os_timer.cpp                 \
    $(ARCH_INC)/interfaces-impl/deep_sleep.cpp               \
    $(ARCH_INC)/interfaces-impl/gpio_impl.cpp                \
    $(ARCH_INC)/interfaces-impl/delays.cpp
    
##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM3_efm32g
##
else ifeq ($(ARCH),cortexM3_efm32g)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM3_efm32g/common

    ifeq ($(OPT_BOARD),efm32g222f128_generic)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_efm32g/efm32g222f128_generic

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/efm32_128k+16k_rom_bootloader.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                                 \
        $(BOARD_INC)/core/stage_1_boot.cpp                          \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -DEFM32G222F128
        CXXFLAGS_BASE += -DEFM32G222F128

        ## Clock frequency
        CLOCK_FREQ := -DEFM32_HFRCO_FREQ=28000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= echo 'make program not supported.'

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture supports processes
    POSTLD := mx-postlinker

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    CPU := -mcpu=cortex-m3 -mthumb
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM3_EFM32G $(CLOCK_FREQ) $(CPU) \
                     $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM3_EFM32G $(CLOCK_FREQ) $(CPU) \
                     $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/core/mpu_cortexMx.cpp                        \
    arch/common/core/efm32_os_timer.cpp                      \
    arch/common/drivers/efm32_serial.cpp                     \
    arch/common/drivers/efm32_adc.cpp                        \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    arch/common/drivers/efm32_gpio.cpp                       \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/CMSIS/Device/SiliconLabs/EFM32G/Source/system_efm32g.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM0plus_stm32l0
##
else ifeq ($(ARCH),cortexM0plus_stm32l0)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM0plus_stm32l0/common

    ifeq ($(OPT_BOARD),stm32l053r8_nucleo)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM0plus_stm32l0/stm32l053r8_nucleo

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/stm32_64k+8k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                                 \
        $(BOARD_INC)/core/stage_1_boot.cpp                          \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -DBOARD_STM32L053R8_NUCLEO -DSTM32L053xx
        CXXFLAGS_BASE += -DBOARD_STM32L053R8_NUCLEO -DSTM32L053xx

        ## Clock frequency
        ## External clock is 8Mhz provided from MCO output of ST-LINK. Actual
        ## HSE crystal (X3) is not fitted.
        CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_32MHz=32000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= st-flash --connect-under-reset --reset write \
                $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture does not support processes
    #POSTLD :=

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    # TODO arch is actually -mcpu=cortex-m0plus but compiler doesn't support it yet
    CPU := -mcpu=cortex-m0 -mthumb
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM0PLUS_STM32L0 $(CLOCK_FREQ) $(CPU) \
                     $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM0PLUS_STM32L0 $(CLOCK_FREQ) $(CPU) \
                     $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common    

    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    arch/common/drivers/serial_stm32.cpp                     \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/drivers/stm32_gpio.cpp                       \
    arch/common/core/stm32_16bit_os_timer.cpp                \
    arch/common/CMSIS/Device/ST/STM32L0xx/Source/Templates/system_stm32l0xx.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM0plus_rp2040
##
else ifeq ($(ARCH),cortexM0plus_rp2040)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM0plus_rp2040/common

    ifeq ($(OPT_BOARD),rp2040_raspberry_pi_pico)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM0plus_rp2040/rp2040_raspberry_pi_pico

        ## Select linker script
        LINKER_SCRIPT := $(BOARD_INC)/rp2040_2M+264k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                                 \
        $(BOARD_INC)/core/stage_1_boot.cpp                          \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -DBOARD_RP2040_RASPBERRY_PI_PICO -D$(BOARD_VARIANT)
        CXXFLAGS_BASE += -DBOARD_RP2040_RASPBERRY_PI_PICO -D$(BOARD_VARIANT)

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROG ?= picotool load -x $(if $(ROMFS_DIR), image.bin, main.bin)

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX := arm-miosix-eabi-
    ## This architecture does not support processes
    #POSTLD :=

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    # TODO arch is actually -mcpu=cortex-m0plus but compiler doesn't support it yet
    CPU := -mcpu=cortex-m0 -mthumb
    AFLAGS_BASE   := $(CPU)
    CFLAGS_BASE   += -D_ARCH_CORTEXM0PLUS_RP2040 $(CLOCK_FREQ) $(CPU) \
                     $(OPT_OPTIMIZATION) -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM0PLUS_RP2040 $(CLOCK_FREQ) $(CPU) \
                     $(OPT_OPTIMIZATION) $(OPT_EXCEPT) -c
    LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                     -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common    

    ARCH_SRC +=                                              \
    arch/common/core/interrupts_cortexMx.cpp                 \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    arch/common/core/rp2040_os_timer.cpp                     \
    arch/common/drivers/rp2040_serial.cpp                    \
    arch/common/CMSIS/Device/RaspberryPi/RP2040/Source/system_RP2040.c

##-----------------------------------------------------------------------------
## end of architecture list
##
endif

## From compiler prefix form the name of the compiler and other tools
CC     := $(PREFIX)gcc
CXX    := $(PREFIX)g++
LD     := $(PREFIX)ld
AR     := $(PREFIX)ar
AS     := $(PREFIX)as
CP     := $(PREFIX)objcopy
OD     := $(PREFIX)objdump
SZ     := $(PREFIX)size
STRIP  := $(PREFIX)strip

ifeq ("$(VERBOSE)","1")
Q :=
ECHO := @true
else
Q := @
ECHO := @echo
endif
