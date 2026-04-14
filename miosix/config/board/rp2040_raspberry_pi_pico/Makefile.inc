##
## Options for board rp2040_raspberry_pi_pico
## Included by the board's makefile
##

# Board variant for selecting either the "plain" Pico or Pico W
#BOARD_VARIANT := BOARD_VARIANT_PICO
BOARD_VARIANT := BOARD_VARIANT_PICO_W

## Select linker scripts
LINKER_SCRIPT := ldscripts/miosix-flash-ram-rp2040.ld
BOARD_LINKER_SCRIPT := unikernel.ld
#BOARD_LINKER_SCRIPT := processes.ld
