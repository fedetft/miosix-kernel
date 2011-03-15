##
## Makefile for STM32
## TFT:Terraneo Federico Technlogies
##

include ../../Makefile.inc

CFLAGS= $(CFLAGS_BASE)

SRC=  \
core_cm3.c  \
system_stm32f10x.c

OBJ = $(SRC:.c=.o)

all: stm32_cmsis

clean:
	rm libstm32_cmsis.a *.o

stm32_cmsis: $(OBJ)
	$(AR) rcs libstm32_cmsis.a *.o

$(OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $<

