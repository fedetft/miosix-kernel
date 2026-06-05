##
## Makefile for Miosix networking stack
## This file builds the networking stack as a static library
##

## KPATH and CONFPATH are forwarded by the parent Makefile
MAKEFILE_VERSION := 3.01
include $(KPATH)/Makefile.kcommon

NET_DIR := $(KPATH)/network

# Variable needed by lwIP makefiles
LWIPDIR := $(NET_DIR)/lwip/src
include $(LWIPDIR)/Filelists.mk

# Recursively populate include dirs for lwIP
INCLUDE_DIRS := -I$(LWIPDIR)/include -I$(NET_DIR)/port/include

# lwIP source files + Miosix porting layer
NET_SRC := $(COREFILES)                         \
           $(CORE4FILES)                        \
           $(NETIFFILES)                        \
           $(APIFILES)                          \
           $(NET_DIR)/port/sys_arch.cpp         \
           $(NET_DIR)/network.cpp

# lwIP driver source files
NET_SRC += $(NET_DIR)/drivers/stm32_ethif.cpp   \
           $(NET_DIR)/drivers/stm32_phy.cpp \

NET_OBJ ?= $(addsuffix .o, $(basename $(NET_SRC)))

all: libmiosix_network.a

clean:
	$(Q)rm -f $(NET_OBJ) $(NET_OBJ:.o=.d) libmiosix_network.a

libmiosix_network.a: $(NET_OBJ)
	$(ECHO) "[AR  ] $@"
	$(Q)$(AR) rcs $@ $^

-include $(NET_OBJ:.o=.d)
