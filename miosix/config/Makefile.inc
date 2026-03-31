##
## Configuration makefile for Miosix embedded OS
##

##
## Target board, choose one.
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
#OPT_BOARD := stm32l151c8_als_mainboard
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
#OPT_BOARD := stm32f103c8_bluepill
#OPT_BOARD := stm32f100c8_microboard
#OPT_BOARD := stm32f469ni_stm32f469i-disco
#OPT_BOARD := stm32f429zi_skyward_homeone
#OPT_BOARD := stm32f401re_nucleo
#OPT_BOARD := stm32f746zg_nucleo
#OPT_BOARD := stm32h753xi_eval
#OPT_BOARD := stm32h723zg_nucleo
#OPT_BOARD := stm32f407vg_thermal_test_chip
#OPT_BOARD := stm32f205_generic
#OPT_BOARD := stm32f103xb_generic
#OPT_BOARD := stm32f072rb_stm32f0discovery
#OPT_BOARD := stm32f100xb_generic
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
#OPT_BOARD := stm32l152re_nucleo
#OPT_BOARD := stm32l010rb_nucleo
#OPT_BOARD := stm32h503rb_generic
#OPT_BOARD := nrf52840_generic
#OPT_BOARD := stm32u585ci_generic
#OPT_BOARD := stm32u535cb_rrc

##
## Optimization flags, choose one.
## -O0 produces large and slow code, but is useful for in circuit debugging.
## -O2 is recomended otherwise, as it provides a good balance between code
## size and speed
##
#OPT_OPTIMIZATION ?= -O0
OPT_OPTIMIZATION ?= -O2
#OPT_OPTIMIZATION ?= -O3
#OPT_OPTIMIZATION ?= -Os

##
## C++ Exception/rtti support disable flags.
## To save code size if not using C++ exceptions (nor some STL code which
## implicitly uses it) uncomment this option.
## the -D__NO_EXCEPTIONS is used by Miosix to know if exceptions are used.
##
#OPT_EXCEPT := -fno-exceptions -fno-rtti -D__NO_EXCEPTIONS

## C++ Exception/rtti support disable flags for userspace processes
#PROC_OPT_EXCEPT := -fno-exceptions -fno-rtti -D__NO_EXCEPTIONS

##
## Other user-configurable C/C++ flags
##
CFLAGS_BASE   ?= -ffunction-sections -fdata-sections \
                 -Wall -Werror=return-type -g
CXXFLAGS_BASE ?= -std=c++23 -ffunction-sections -fdata-sections \
                 -Wall -Werror=return-type -g
