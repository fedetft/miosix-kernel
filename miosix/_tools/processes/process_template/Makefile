##
## Makefile for writing PROCESSES for the Miosix embedded OS
##

BIN := example_program

SRC := \
main.cpp

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
            -Wl,--gc-sections,-Map,main.map,-T./miosix.ld,-n,-pie,--spare-dynamic-tags,3,--target2=mx-data-rel \
            -O2 -nostdlib
DFLAGS    := -MMD -MP

LINK_LIBS := -Wl,--start-group -lstdc++ -lc -lm -lgcc -Wl,--end-group

all: $(OBJ) crt0.o crt1.o
	$(CXX) $(LFLAGS) -o $(BIN) $(OBJ) crt0.o crt1.o $(LINK_LIBS)
	$(SZ)  $(BIN)
# 	@arm-miosix-eabi-objdump -Dslx main.elf > main.txt
	@arm-miosix-eabi-strip $(BIN)
	@mx-postlinker $(BIN) --ramsize=16384 --stacksize=2048 --strip-sectheader

clean:
	-rm -f $(OBJ) crt0.o crt1.o crt1.d $(BIN) main.map main.txt $(OBJ:.o=.d)

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o : %.c
	$(CC) $(DFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) $(DFLAGS) $(CXXFLAGS) $< -o $@

#pull in dependecy info for existing .o files
-include $(OBJ:.o=.d)
