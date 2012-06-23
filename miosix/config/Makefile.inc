##
## Makefile for Miosix np embedded OS
## TFT:Terraneo Federico Technlogies
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
OPT_BOARD := stm32f103ze_stm3210e-eval
#OPT_BOARD := stm32f103ve_mp3v2
#OPT_BOARD := stm32f100rb_stm32vldiscovery
#OPT_BOARD := stm32f103ve_strive_mini
#OPT_BOARD := stm32f103ze_redbull_v2
#OPT_BOARD := stm32f407vg_stm32f4discovery
#OPT_BOARD := stm32f207ig_stm3220g-eval
#OPT_BOARD := stm32f207zg_ethboard_v2
#OPT_BOARD := stm32f207ze_als_camboard
#OPT_BOARD := stm32l151_als_mainboard

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
    ##    forwards interrrupts @ 0x68000000 (see miosix/bootloaders for one).
    ##    The microcontroller must have an external memory interface.
    ## Warning! enable external ram if you use a linker script that requires it
    ## (see the XRAM flag below)
    LINKER_SCRIPT_PATH := arch/cortexM3_stm32/stm32f103ze_stm3210e-eval/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+64k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+64k_xram.ld
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
    LINKER_SCRIPT_PATH := arch/cortexM3_stm32/stm32f103ve_mp3v2/
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
    LINKER_SCRIPT_PATH := arch/cortexM4_stm32f4/stm32f407vg_stm32f4discovery/
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+192k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+192k_ram.ld

    ## This causes the interrupt vector table to be relocated in SRAM, must be
    ## uncommented when using the ram linker script
    #SRAM_BOOT := -DVECT_TAB_SRAM

    ## Select clock frequency (HSE_VALUE is the xtal on board, fixed)
    CLOCK_FREQ := -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000
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
    ##    forwards interrrupts @ 0x64000000 (see miosix/bootloaders for one).
    ##    The microcontroller must have an external memory interface.
    ## Warning! enable external ram if you use a linker script that requires it
    ## (see the XRAM flag below)
    LINKER_SCRIPT_PATH := arch/cortexM3_stm32f2/stm32f207ig_stm3220g-eval/
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_rom.ld
    #LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram.ld
    LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_all_in_xram.ld

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

    ## Linker script type, there are three options
    ## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
    ##    the most common choice, available for all microcontrollers
    ## 2) Code in external RAM, stack + heap in internal RAM
    ##    (file *_code_in_xram.ld) useful for debugging. Code runs
    ##    *very* slow compared to FLASH. Works only with a booloader that
    ##    forwards interrrupts @ 0x60000000 (see miosix/bootloaders for one).
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

############################################################################
## From the options selected above, now fill all the variables needed to  ##
## build Miosix. You should modify something here only if you are adding  ##
## a new board or porting Miosix to a new architecture                    ##
############################################################################

ifneq ($(MAKEFILE_VERSION),1.01)
    $(error You are using an incompatible makefile. Make sure it matches \
      the one distributed with the current version of the kernel)
endif

##
## First, auto guess architecture name from board name
##
ifeq ($(OPT_BOARD),lpc2138_miosix_board)
    ARCH := arm7_lpc2000
else ifeq ($(OPT_BOARD),stm32f103ze_stm3210e-eval)
    ARCH := cortexM3_stm32
else ifeq ($(OPT_BOARD),stm32f103ve_mp3v2)
    ARCH := cortexM3_stm32
else ifeq ($(OPT_BOARD),stm32f100rb_stm32vldiscovery)
    ARCH := cortexM3_stm32
else ifeq ($(OPT_BOARD),stm32f103ve_strive_mini)
    ARCH := cortexM3_stm32
else ifeq ($(OPT_BOARD),stm32f103ze_redbull_v2)
    ARCH := cortexM3_stm32
else ifeq ($(OPT_BOARD),stm32f407vg_stm32f4discovery)
    ARCH := cortexM4_stm32f4
else ifeq ($(OPT_BOARD),stm32f207ig_stm3220g-eval)
    ARCH := cortexM3_stm32f2
else ifeq ($(OPT_BOARD),stm32f207zg_ethboard_v2)
    ARCH := cortexM3_stm32f2
else ifeq ($(OPT_BOARD),stm32f207ze_als_camboard)
    ARCH := cortexM3_stm32f2
else ifeq ($(OPT_BOARD),stm32l151_als_mainboard)
    ARCH := cortexM3_stm32l1
else
    $(error Error: no board specified in miosix/config/Makefile.inc)
endif


##
## Then, initialize C/C++ flags with -D_MIOSIX so that application code can
## know if the OS is MIOSIX
##
CFLAGS_BASE   := -D_MIOSIX=\"$(OPT_BOARD)\"
CXXFLAGS_BASE := -D_MIOSIX=\"$(OPT_BOARD)\"

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

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        LINKER_SCRIPT := arch/arm7_lpc2000/lpc2138_miosix_board/miosix.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/core/interrupts.cpp             \
        $(BOARD_INC)/drivers/serial.cpp              \
        $(BOARD_INC)/interfaces-impl/portability.cpp \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
        $(BOARD_INC)/interfaces-impl/disk.cpp        \
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
    PREFIX=arm-miosix-eabi-

    ## From compiler prefix form the name of the compiler and other tools
    CC  := $(PREFIX)gcc
    CXX := $(PREFIX)g++
    LD  := $(PREFIX)ld
    AR  := $(PREFIX)ar
    AS  := $(PREFIX)as
    CP  := $(PREFIX)objcopy
    OD  := $(PREFIX)objdump
    SZ  := $(PREFIX)size

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    AFLAGS_BASE   := -mcpu=arm7tdmi -mapcs-32 -mfloat-abi=soft
    CFLAGS_BASE   += -D_ARCH_ARM7_LPC2000 -mcpu=arm7tdmi $(OPT_OPTIMIZATION) \
                     -ffunction-sections -Wall -g -c
    CXXFLAGS_BASE += -D_ARCH_ARM7_LPC2000 -mcpu=arm7tdmi $(OPT_OPTIMIZATION) \
                     -ffunction-sections $(OPT_EXCEPT) -Wall -g -c
    LFLAGS_BASE   := -mcpu=arm7tdmi -Wl,--gc-sections,-Map,main.map          \
                     -Wl,-T./miosix/$(LINKER_SCRIPT) $(OPT_EXCEPT)           \
                      $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    #ARCH_SRC += Nothing to add

    ## Select programmer command line
    ## This is the program that is invoked when the user types 'make program'
    ## The command must provide a way to program the board, or print an error
    ## message saying that 'make program' is not supported for that board.
    PROGRAM_CMDLINE := lpc21isp -verify main.hex /dev/ttyUSB0 115200 14746

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM3_stm32
##
else ifeq ($(ARCH),cortexM3_stm32)
    ## Base directory with header files for this board
    ARCH_INC := arch/cortexM3_stm32/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f103ze_stm3210e-eval
    ##
    ifeq ($(OPT_BOARD),stm32f103ze_stm3210e-eval)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32/stm32f103ze_stm3210e-eval

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
        $(ARCH_INC)/interfaces-impl/disk.cpp         \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM3210E_EVAL -DSTM32F10X_HD
        CXXFLAGS_BASE += -D_BOARD_STM3210E_EVAL -DSTM32F10X_HD

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        ifeq ($(LINKER_SCRIPT),$(LINKER_SCRIPT_PATH)stm32_512k+64k_all_in_xram.ld)
        PROGRAM_CMDLINE := miosix/bootloaders/stm32/pc_loader/pc_loader \
        /dev/ttyUSB0 main.bin
        else
        PROGRAM_CMDLINE := stm32flash -w main.hex -v /dev/ttyUSB0
        endif

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f103ve_mp3v2
    ##
    else ifeq ($(OPT_BOARD),stm32f103ve_mp3v2)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32/stm32f103ve_mp3v2

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
        $(ARCH_INC)/interfaces-impl/disk.cpp         \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_MP3V2 -DSTM32F10X_HD
        CXXFLAGS_BASE += -D_BOARD_MP3V2 -DSTM32F10X_HD

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_72MHz=72000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        ifeq ($(LINKER_SCRIPT),$(LINKER_SCRIPT_PATH)stm32_512k+64k_ram.ld)
        PROGRAM_CMDLINE := sudo mp3v2_bootloader --ram main.bin
        else
        PROGRAM_CMDLINE := sudo mp3v2_bootloader --code main.bin
        endif

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f100rb_stm32vldiscovery
    ##
    else ifeq ($(OPT_BOARD),stm32f100rb_stm32vldiscovery)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32/stm32f100rb_stm32vldiscovery

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        LINKER_SCRIPT := $(BOARD_INC)/stm32_128k+8k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STM32VLDISCOVERY -DSTM32F10X_MD_VL 
        CXXFLAGS_BASE += -D_BOARD_STM32VLDISCOVERY -DSTM32F10X_MD_VL

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_24MHz=24000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROGRAM_CMDLINE := sudo vsprog -cstm32_vl -ms -I main.hex -oe -owf -ovf

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f103ve_strive_mini
    ##
    else ifeq ($(OPT_BOARD),stm32f103ve_strive_mini)

        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32/stm32f103ve_strive_mini

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        LINKER_SCRIPT := $(BOARD_INC)/stm32_512k+64k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
        $(ARCH_INC)/interfaces-impl/disk.cpp         \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_STRIVE_MINI -DSTM32F10X_HD
        CXXFLAGS_BASE += -D_BOARD_STRIVE_MINI -DSTM32F10X_HD

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_72MHz=72000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROGRAM_CMDLINE := "c:/Program Files/STMicroelectronics/STM32 ST-LINK Utility/ST-LINK Utility/ST-LINK_CLI.exe" \
        -c JTAG -Rst -P main.hex 0x08000000 -Run

    ##-------------------------------------------------------------------------
    ## BOARD: HY RedBull V2 (or V1)
    ##
    else ifeq ($(OPT_BOARD),stm32f103ze_redbull_v2)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32/stm32f103ze_redbull_v2

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        LINKER_SCRIPT := $(BOARD_INC)/stm32_512k+64k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
        $(ARCH_INC)/interfaces-impl/disk.cpp         \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_REDBULL_V2 -DSTM32F10X_HD
        CXXFLAGS_BASE += -D_BOARD_REDBULL_V2 -DSTM32F10X_HD

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_72MHz=72000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROGRAM_CMDLINE := "C:/Program Files/SEGGER/JLinkARM_V434d/JFlashARM.exe" \
        -openprjSTM32F10xxE.jflash -openmain.hex -auto -exit
        #PROGRAM_CMDLINE := "c:/Program Files/STMicroelectronics/STM32 ST-LINK Utility/ST-LINK Utility/ST-LINK_CLI.exe" \
        #-c JTAG -Rst -P main.hex 0x08000000 -Run

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX=arm-miosix-eabi-

    ## From compiler prefix form the name of the compiler and other tools
    CC  := $(PREFIX)gcc
    CXX := $(PREFIX)g++
    LD  := $(PREFIX)ld
    AR  := $(PREFIX)ar
    AS  := $(PREFIX)as
    CP  := $(PREFIX)objcopy
    OD  := $(PREFIX)objdump
    SZ  := $(PREFIX)size

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    AFLAGS_BASE   := -mcpu=cortex-m3 -mthumb
    CFLAGS_BASE   += -D_ARCH_CORTEXM3_STM32 $(CLOCK_FREQ) $(XRAM)            \
                     -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd            \
                     $(OPT_OPTIMIZATION) -ffunction-sections -Wall -g -c 
    CXXFLAGS_BASE += -D_ARCH_CORTEXM3_STM32 $(CLOCK_FREQ) $(XRAM)            \
                     $(OPT_EXCEPT) -mcpu=cortex-m3 -mthumb                   \
                     -mfix-cortex-m3-ldrd $(OPT_OPTIMIZATION)                \
                     -ffunction-sections -Wall -g -c
    LFLAGS_BASE   := -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd            \
                     -Wl,--gc-sections,-Map,main.map                         \
                     -Wl,-T./miosix/$(LINKER_SCRIPT) $(OPT_EXCEPT)           \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                  \
    arch/common/drivers/dcc.cpp                  \
    $(ARCH_INC)/core/interrupts.cpp              \
    $(ARCH_INC)/drivers/serial.cpp               \
    $(ARCH_INC)/interfaces-impl/portability.cpp  \
    $(ARCH_INC)/interfaces-impl/delays.cpp       \
    $(ARCH_INC)/interfaces-impl/gpio_impl.cpp    \
    $(ARCH_INC)/CMSIS/core_cm3.c                 \
    $(ARCH_INC)/CMSIS/system_stm32f10x.c

##-----------------------------------------------------------------------------
## ARCHITECTURE: cortexM4_stm32f4
##
else ifeq ($(ARCH),cortexM4_stm32f4)
    ## Base directory with else header files for this board
    ARCH_INC := arch/cortexM4_stm32f4/common

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f4discovery
    ##
    ifeq ($(OPT_BOARD),stm32f407vg_stm32f4discovery)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM4_stm32f4/stm32f407vg_stm32f4discovery

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
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
        PROGRAM_CMDLINE := qstlink2 -cqewV ./main.bin

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX=arm-miosix-eabi-

    ## From compiler prefix form the name of the compiler and other tools
    CC  := $(PREFIX)gcc
    CXX := $(PREFIX)g++
    LD  := $(PREFIX)ld
    AR  := $(PREFIX)ar
    AS  := $(PREFIX)as
    CP  := $(PREFIX)objcopy
    OD  := $(PREFIX)objdump
    SZ  := $(PREFIX)size

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    #FIXME: Replace -mcpu=cortex-m3 with -mcpu=cortex-m4 after upgrading GCC
    AFLAGS_BASE   := -mcpu=cortex-m3 -mthumb
    CFLAGS_BASE   += -D_ARCH_CORTEXM4_STM32F4 $(CLOCK_FREQ) $(SRAM_BOOT)     \
                     -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd            \
                     $(OPT_OPTIMIZATION) -ffunction-sections -Wall -g -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM4_STM32F4 $(CLOCK_FREQ) $(SRAM_BOOT)     \
                     $(OPT_EXCEPT) -mcpu=cortex-m3 -mthumb                   \
                     -mfix-cortex-m3-ldrd $(OPT_OPTIMIZATION)                \
                     -ffunction-sections -Wall -g -c
    LFLAGS_BASE   := -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd            \
                     -Wl,--gc-sections,-Map,main.map                         \
                     -Wl,-T./miosix/$(LINKER_SCRIPT) $(OPT_EXCEPT)           \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/drivers/dcc.cpp                              \
    $(ARCH_INC)/core/interrupts.cpp                          \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    $(ARCH_INC)/interfaces-impl/gpio_impl.cpp                \
    $(ARCH_INC)/CMSIS/system_stm32f4xx.c

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

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(ARCH_INC)/interfaces-impl/disk.cpp         \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
        $(BOARD_INC)/interfaces-impl/delays.cpp      \
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
        PROGRAM_CMDLINE := miosix/bootloaders/stm32/pc_loader/pc_loader \
        /dev/ttyUSB0 main.bin
        else
        PROGRAM_CMDLINE := qstlink2 -cqewV ./main.bin
        endif

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f207zg_ethboard_v2
    ##
    else ifeq ($(OPT_BOARD),stm32f207zg_ethboard_v2)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f2/stm32f207zg_EthBoardV2

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        #LINKER_SCRIPT := already selected in board options

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(ARCH_INC)/interfaces-impl/disk.cpp         \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
        $(BOARD_INC)/interfaces-impl/delays.cpp      \
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
        PROGRAM_CMDLINE := miosix/bootloaders/stm32/pc_loader/pc_loader \
        /dev/ttyUSB1 main.bin
        else
        PROGRAM_CMDLINE := stm32flash -w main.hex -v /dev/ttyUSB1
        endif

    ##-------------------------------------------------------------------------
    ## BOARD: stm32f207ze_als_camboard
    ##
    else ifeq ($(OPT_BOARD),stm32f207ze_als_camboard)
        ## Base directory with header files for this board
        BOARD_INC := arch/cortexM3_stm32f2/stm32f207ze_als_camboard

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        LINKER_SCRIPT := $(BOARD_INC)/stm32_1m+128k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
        $(BOARD_INC)/interfaces-impl/delays.cpp      \
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
        PROGRAM_CMDLINE := stm32flash -w main.hex -v /dev/ttyUSB1

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX=arm-miosix-eabi-

    ## From compiler prefix form the name of the compiler and other tools
    CC  := $(PREFIX)gcc
    CXX := $(PREFIX)g++
    LD  := $(PREFIX)ld
    AR  := $(PREFIX)ar
    AS  := $(PREFIX)as
    CP  := $(PREFIX)objcopy
    OD  := $(PREFIX)objdump
    SZ  := $(PREFIX)size

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    AFLAGS_BASE   := -mcpu=cortex-m3 -mthumb
    CFLAGS_BASE   += -D_ARCH_CORTEXM3_STM32F2 $(CLOCK_FREQ) $(XRAM)          \
                     -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd            \
                     $(OPT_OPTIMIZATION) -ffunction-sections -Wall -g -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM3_STM32F2 $(CLOCK_FREQ) $(XRAM)          \
                     $(OPT_EXCEPT) -mcpu=cortex-m3 -mthumb                   \
                     -mfix-cortex-m3-ldrd $(OPT_OPTIMIZATION)                \
                     -ffunction-sections -Wall -g -c
    LFLAGS_BASE   := -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd            \
                     -Wl,--gc-sections,-Map,main.map                         \
                     -Wl,-T./miosix/$(LINKER_SCRIPT) $(OPT_EXCEPT)           \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/drivers/dcc.cpp                              \
    $(ARCH_INC)/core/interrupts.cpp                          \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/gpio_impl.cpp                \
    $(ARCH_INC)/CMSIS/core_cm3.c                             \
    $(ARCH_INC)/CMSIS/system_stm32f2xx.c

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

        ## Select linker script and boot file
        ## Their path must be relative to the miosix directory.
        BOOT_FILE := $(BOARD_INC)/core/stage_1_boot.o
        LINKER_SCRIPT := $(BOARD_INC)/stm32_64k+10k_rom.ld

        ## Select architecture specific files
        ## These are the files in arch/<arch name>/<board name>
        ARCH_SRC :=                                  \
        $(BOARD_INC)/interfaces-impl/console.cpp     \
        $(BOARD_INC)/interfaces-impl/bsp.cpp

        ## Add a #define to allow querying board name
        CFLAGS_BASE   += -D_BOARD_ALS_MAINBOARD
        CXXFLAGS_BASE += -D_BOARD_ALS_MAINBOARD

        ## Clock frequency
        CLOCK_FREQ := -DSYSCLK_FREQ_16MHz=16000000

        ## Select programmer command line
        ## This is the program that is invoked when the user types
        ## 'make program'
        ## The command must provide a way to program the board, or print an
        ## error message saying that 'make program' is not supported for that
        ## board.
        PROGRAM_CMDLINE := stm32flash -w main.bin -v /dev/ttyUSB1

    ##-------------------------------------------------------------------------
    ## End of board list
    ##
    endif

    ## Select compiler
    PREFIX=arm-miosix-eabi-

    ## From compiler prefix form the name of the compiler and other tools
    CC  := $(PREFIX)gcc
    CXX := $(PREFIX)g++
    LD  := $(PREFIX)ld
    AR  := $(PREFIX)ar
    AS  := $(PREFIX)as
    CP  := $(PREFIX)objcopy
    OD  := $(PREFIX)objdump
    SZ  := $(PREFIX)size

    ## Select appropriate compiler flags for both ASM/C/C++/linker
    AFLAGS_BASE   := -mcpu=cortex-m3 -mthumb
    CFLAGS_BASE   += -D_ARCH_CORTEXM3_STM32L1 $(CLOCK_FREQ) $(XRAM)          \
                     -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd            \
                     $(OPT_OPTIMIZATION) -ffunction-sections -Wall -g -c
    CXXFLAGS_BASE += -D_ARCH_CORTEXM3_STM32L1 $(CLOCK_FREQ) $(XRAM)          \
                     $(OPT_EXCEPT) -mcpu=cortex-m3 -mthumb                   \
                     -mfix-cortex-m3-ldrd $(OPT_OPTIMIZATION)                \
                     -ffunction-sections -Wall -g -c
    LFLAGS_BASE   := -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd            \
                     -Wl,--gc-sections,-Map,main.map                         \
                     -Wl,-T./miosix/$(LINKER_SCRIPT) $(OPT_EXCEPT)           \
                     $(OPT_OPTIMIZATION) -nostdlib

    ## Select architecture specific files
    ## These are the files in arch/<arch name>/common
    ARCH_SRC +=                                              \
    arch/common/drivers/dcc.cpp                              \
    $(ARCH_INC)/core/interrupts.cpp                          \
    $(ARCH_INC)/interfaces-impl/portability.cpp              \
    $(ARCH_INC)/interfaces-impl/gpio_impl.cpp                \
    $(ARCH_INC)/interfaces-impl/delays.cpp                   \
    $(ARCH_INC)/CMSIS/system_stm32l1xx.c

##-----------------------------------------------------------------------------
## end of architecture list
##
endif
