##
## Makefile for writing PROGRAMS for the Miosix embedded OS
## TFT:Terraneo Federico Technlogies
##

SRC := \
main.c

## Replaces both "foo.cpp"-->"foo.o" and "foo.c"-->"foo.o"
OBJ := $(addsuffix .o, $(basename $(SRC)))

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
DFLAGS    := -MMD -MP

LINK_LIBS := -Wl,--start-group -lstdc++ -lc -lm -lgcc -Wl,--end-group

all: $(OBJ) crt0.o
	$(CXX) $(LFLAGS) -o test.elf $(OBJ) crt0.o $(LINK_LIBS)
	$(SZ)  test.elf
	@arm-miosix-eabi-objdump -Dslx test.elf > test.txt
	@mx-postlinker test.elf --ramsize=16384 --stacksize=2048 --strip-sectheader
	@xxd -i test.elf | sed 's/unsigned char/const unsigned char __attribute__((aligned(8)))/' > mpuTest.h

clean:
	-rm $(OBJ) crt0.o test.elf test.map test.txt mpuTest.h $(OBJ:.o=.d)

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o : %.c
	$(CC) $(DFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) $(DFLAGS) $(CXXFLAGS) $< -o $@

#pull in dependecy info for existing .o files
-include $(OBJ:.o=.d)
