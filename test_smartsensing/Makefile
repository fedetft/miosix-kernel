##
## Makefile for writing PROGRAMS for the Miosix embedded OS
## TFT:Terraneo Federico Technlogies
##

SRC := \
test1.c \
test2.c

HEADER := $(addsuffix .h, $(basename $(SRC)))
EXE := $(addsuffix .elf, $(basename $(SRC)))
## Replaces both "foo.cpp"-->"foo.o" and "foo.c"-->"foo.o"
OBJ := $(addsuffix .o, $(basename $(SRC)))

TEMP := $(addsuffix .txt, $(EXE))

MAP := $(addsuffix .map, $(basename $(SRC)))


AS  := arm-miosix-eabi-as
CC  := arm-miosix-eabi-gcc
CXX := arm-miosix-eabi-g++
SZ  := arm-miosix-eabi-size

AFLAGS   := -mcpu=cortex-m3 -mthumb
CFLAGS   := -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fpie -msingle-pic-base \
            -ffunction-sections -O2 -Wall -c
CXXFLAGS := $(CFLAGS)
LFLAGS   := -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fpie -msingle-pic-base \
            -Wl,--gc-sections,-T./miosix.ld,-n,-pie,--spare-dynamic-tags,3 \
            -O2 -nostdlib

LINK_LIBS := -Wl,--start-group -lstdc++ -lc -lm -lgcc -Wl,--end-group

all: $(HEADER) crt0.o	

	

clean:
	-rm $(OBJ) crt0.o main.map $(TEMP)

%.h: %.elf
	$(SZ) $<
	@arm-miosix-eabi-objdump -Dslx $< > $<.txt
	@mx-postlinker $< --ramsize=16384 --stacksize=2048 --strip-sectheader
	@xxd -i $< | sed 's/unsigned char/const unsigned char __attribute__((aligned(8)))/' > $@
	

%.elf: %.o crt0.o
	$(CXX) $(LFLAGS) -o $@ $< crt0.o $(LINK_LIBS)

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
