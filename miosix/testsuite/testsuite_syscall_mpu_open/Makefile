##
## Makefile for writing PROGRAMS for the Miosix embedded OS
## TFT:Terraneo Federico Technlogies
##

SRC := \
main.c

## Replaces both "foo.cpp"-->"foo.o" and "foo.c"-->"foo.o"
OBJ := $(addsuffix .o, $(basename $(SRC)))
ELF := $(addsuffix .elf, $(NAME))

AS  := arm-miosix-eabi-as
CC  := arm-miosix-eabi-gcc
CXX := arm-miosix-eabi-g++
SZ  := arm-miosix-eabi-size

AFLAGS   := -mcpu=cortex-m3 -mthumb
CFLAGS   := -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fpie -msingle-pic-base \
            -ffunction-sections -O2 -Wall -c
CXXFLAGS := $(CFLAGS)
LFLAGS   := -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fpie -msingle-pic-base \
            -Wl,--gc-sections,-Map,test.map,-T./miosix.ld,-n,-pie,--spare-dynamic-tags,3 \
            -O2 -nostdlib

LINK_LIBS := -Wl,--start-group -lstdc++ -lc -lm -lgcc -Wl,--end-group

all: $(OBJ) crt0.o
	$(CXX) $(LFLAGS) -o $(ELF) $(OBJ) crt0.o $(LINK_LIBS)
	$(SZ)  $(ELF)
	@arm-miosix-eabi-objdump -Dslx $(ELF) > test.txt
	@mx-postlinker $(ELF) --ramsize=16384 --stacksize=2048 --strip-sectheader
	@xxd -i $(ELF) | sed 's/unsigned char/const unsigned char __attribute__((aligned(8)))/' > prog3.h

clean:
	-rm $(OBJ) crt0.o *.elf test.map test.txt

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
