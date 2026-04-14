##
## Options for board rp2040_raspberry_pi_pico
## Included by the board's makefile
##

# Board variant for selecting either the "plain" Pico or Pico W
#BOARD_VARIANT := BOARD_VARIANT_PICO
BOARD_VARIANT := BOARD_VARIANT_PICO_W

## Select linker script
LINKER_SCRIPT := unikernel.ld
#LINKER_SCRIPT := processes.ld
