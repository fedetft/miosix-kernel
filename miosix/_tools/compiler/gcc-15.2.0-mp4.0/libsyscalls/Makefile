##
## Makefile for libsyscalls, the library with userspace syscalls for processes
## This makefile builds libsyscalls.a
##

## Allow `make clean' to always work
ifeq (clean,$(MAKECMDGOALS))
    TARGET=x
    PREFIX=.
    MULTILIB_PATH=.
endif

## TARGET, PREFIX, MULTILIB_PATH and MULTILIB_FLAGS should be specified
## by the caller
ifeq ($(TARGET),)
    $(info TARGET must be a triple like 'arm-miosix-eabi')
    $(error Error: TARGET not specified)
endif
ifeq ($(PREFIX),)
    $(info PREFIX must be a toolchain prefix path like '/opt/arm-miosix-eabi')
    $(error Error: PREFIX not specified)
endif
ifeq ($(MULTILIB_PATH),)
    # Note that MULTILIB_FLAGS can be empty so we can't test for it being
    # undefined. So the help for MULTILIB_FLAGS is here
    $(info MULTILIB_PATH must be a relative library path like 'thumb/v7-m/nofp')
    $(info and MULTILIB_FLAGS should the associated set of GCC flags like)
    $(info '-mthumb -march=armv7-m -mfloat-abi=soft')
    $(error Error: MULTILIB_PATH not specified)
endif

SRC      := crt0.s crt1.cpp memoryprofiling.cpp

CC       := $(TARGET)-gcc
CXX      := $(TARGET)-g++
AR       := $(TARGET)-ar

OPT_OPTIMIZATION ?= -O2
AFLAGS   ?= $(MULTILIB_FLAGS)
CFLAGS   ?= -MMD -MP $(MULTILIB_FLAGS) -ffunction-sections -Wall \
            -Werror=return-type -D_DEFAULT_SOURCE=1 $(OPT_OPTIMIZATION) -g
CXXFLAGS ?= -std=c++14 $(CFLAGS)

OBJ ?= $(addsuffix .o, $(basename $(SRC)))

ifeq ("$(VERBOSE)","1")
Q :=
ECHO := @true
else
Q := @
ECHO := @echo
endif

all: libsyscalls.a

clean:
	$(Q)rm -f $(OBJ) $(OBJ:.o=.d) *.a

install: libsyscalls.a
	install -m 644 libsyscalls.a $(DESTDIR)$(PREFIX)/$(TARGET)/lib/$(MULTILIB_PATH)

libsyscalls.a: $(OBJ)
	$(ECHO) "[AR  ] libsyscalls.a"
	$(Q)$(AR) rcs libsyscalls.a $(OBJ)

%.o: %.cpp
	$(ECHO) "[CXX ] $<"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(ECHO) "[CC  ] $<"
	$(Q)$(CC)  $(CFLAGS) -c $< -o $@

%.o: %.s
	$(ECHO) "[AS  ] $<"
	$(Q)$(CC)  $(AFLAGS) -c $< -o $@

%.o: %.S
	$(ECHO) "[AS  ] $<"
	$(Q)$(CC)  $(AFLAGS) -c $< -o $@

-include $(OBJ:.o=.d)
