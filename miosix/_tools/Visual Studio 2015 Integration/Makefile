#Note: VisualGDB will automatically update this file when you add new sources to the project.
#All other changes you make in this file will be preserved.
#Visit http://visualgdb.com/makefiles for more details

#VisualGDB: AutoSourceFiles		#<--- remove this line to disable auto-updating of SOURCEFILES and EXTERNAL_LIBS
SOURCEFILES := main.cpp
EXTERNAL_LIBS := 
EXTERNAL_LIBS_COPIED := $(foreach lib, $(EXTERNAL_LIBS),$(BINARYDIR)/$(notdir $(lib)))

TARGETNAME := MiosixProject.elf
#TARGETTYPE used by VisualGDB
TARGETTYPE := APP

to_lowercase = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

#Include debug/release makefile, from now on referred to as BuildConfigMakefile, ...
CONFIG ?= DEBUG
CONFIGURATION_FLAGS_FILE := $(call to_lowercase,$(CONFIG)).mak
include $(CONFIGURATION_FLAGS_FILE)
#and check its validity
ifeq ($(BINARYDIR),)
error:
	$(error Invalid configuration, please check your inputs)
endif

#Include miosix configuration makefile, from now on referred to as Makefile.inc,
#which will set some required variables
include $(MIOSIX_KERNEL_PATH)/config/Makefile.inc

#Use BuildConfigMakefile TOOLCHAIN_ROOT variable to update executables locations
CC  := $(addprefix $(TOOLCHAIN_ROOT)/bin/, $(CC))
CXX := $(addprefix $(TOOLCHAIN_ROOT)/bin/, $(CXX))
LD  := $(addprefix $(TOOLCHAIN_ROOT)/bin/, $(LD))
AR  := $(addprefix $(TOOLCHAIN_ROOT)/bin/, $(AR))
AS  := $(addprefix $(TOOLCHAIN_ROOT)/bin/, $(AS))
CP  := $(addprefix $(TOOLCHAIN_ROOT)/bin/, $(CP))
OD  := $(addprefix $(TOOLCHAIN_ROOT)/bin/, $(OD))
SZ  := $(addprefix $(TOOLCHAIN_ROOT)/bin/, $(SZ))

#Add include dirs needed by miosix, as stated by Makefile.inc 
MIOSIX_INCLUDE_DIRS := 	. \
						$(MIOSIX_KERNEL_PATH) \
						$(MIOSIX_KERNEL_PATH)/config/$(BOARD_INC) \
						$(MIOSIX_KERNEL_PATH)/arch/common \
						$(MIOSIX_KERNEL_PATH)/$(ARCH_INC) \
						$(MIOSIX_KERNEL_PATH)/$(BOARD_INC)
INCLUDE_DIRS += $(MIOSIX_INCLUDE_DIRS)
LIBRARY_DIRS += $(MIOSIX_KERNEL_PATH)
#and also add base flags set by it
CXXFLAGS += $(CXXFLAGS_BASE)
CFLAGS   += $(CFLAGS_BASE)
ASFLAGS  += $(AFLAGS_BASE)
LDFLAGS  += $(LFLAGS_BASE)

#Add commonflags from BuildConfigMakefile
CFLAGS += $(COMMONFLAGS)
CXXFLAGS += $(COMMONFLAGS)
ASFLAGS += $(COMMONFLAGS)
LDFLAGS += $(COMMONFLAGS)
#and DFLAGS
CFLAGS += $(DFLAGS)
CXXFLAGS += $(DFLAGS)

#Add library includes to compilator
CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
CXXFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
#and library dirs to linker
#LDFLAGS will be passed directly to LD. (no -Wl, before)
LDFLAGS += $(addprefix -L,$(LIBRARY_DIRS))
###let's add even static libraries
###LDFLAGS += $(STATIC_LIBS)
#before going on, we need to add library needed by miosix
LIBRARY_NAMES += miosix stdc++ c m gcc
#ok, we can build up Linker inputs, starting by libraries and adding ADDITIONAL_LINKER_INPUTS from BuildConfigMakefile
#LIBRARY_LDFLAGS will be passed as arguments directly to the linker, by using START_GROUP and END_GROUP
LIBRARY_LDFLAGS = $(addprefix -l,$(LIBRARY_NAMES))
LIBRARY_LDFLAGS += $(EXTERNAL_LIBS)
LIBRARY_LDFLAGS += $(ADDITIONAL_LINKER_INPUTS)

ifeq ($(GENERATE_MAP_FILE),1)
LDFLAGS += -Wl,-Map=$(BINARYDIR)/$(basename $(TARGETNAME)).map
endif

#add miosix and eventually mxgui to the build recursive list
RECURSIVE_BUILD_SUBDIRS += miosix
ifneq (,$(findstring mxgui\libmxgui.a, $(STATIC_LIBS)))
	RECURSIVE_BUILD_SUBDIRS += mxgui
endif

#let's get started to compile. We want to put all generated .o files in the BINARYDIR specified in BuildConfigMakefile
all_source_files := $(SOURCEFILES)
source_obj1 := $(all_source_files:.cpp=.o)
source_obj2 := $(source_obj1:.c=.o)
source_obj3 := $(source_obj2:.s=.o)
source_obj4 := $(source_obj3:.S=.o)
source_obj5 := $(source_obj4:.cc=.o)
source_objs := $(source_obj5:.cxx=.o)
all_objs := $(addprefix $(BINARYDIR)/, $(notdir $(source_objs)))

#What do we want to generate? We check the variables in BuildConfigMakefile
PRIMARY_OUTPUTS := $(BINARYDIR)/$(TARGETNAME)
ifeq ($(GENERATE_BIN_FILE),1)
PRIMARY_OUTPUTS += $(BINARYDIR)/$(basename $(TARGETNAME)).bin
endif
ifeq ($(GENERATE_IHEX_FILE),1)
PRIMARY_OUTPUTS += $(BINARYDIR)/$(basename $(TARGETNAME)).ihex
endif
#and add condequently objects to PRIMARY_OUTPUTS
PRIMARY_OUTPUTS :=
ifeq ($(GENERATE_BIN_FILE),1)
PRIMARY_OUTPUTS += $(BINARYDIR)/$(basename $(TARGETNAME)).bin
endif
ifeq ($(GENERATE_IHEX_FILE),1)
PRIMARY_OUTPUTS += $(BINARYDIR)/$(basename $(TARGETNAME)).ihex
endif
ifeq ($(PRIMARY_OUTPUTS),)
PRIMARY_OUTPUTS := $(BINARYDIR)/$(TARGETNAME)
endif

#check other makefiles in var ADDITIONAL_MAKE_FILES from BuildConfigMakefile
all_make_files := $(firstword $(MAKEFILE_LIST)) $(CONFIGURATION_FLAGS_FILE) $(ADDITIONAL_MAKE_FILES)

#goals region
all: build-recursive build-program size-stats

build-recursive:
	$(foreach i,$(RECURSIVE_BUILD_SUBDIRS),($(MAKE) -C $(i) \
	  KPATH=$(shell perl $(MIOSIX_KERNEL_PATH)/_tools/relpath.pl $(i) $(MIOSIX_KERNEL_PATH)) \
	  CONFPATH=$(shell perl $(MIOSIX_KERNEL_PATH)/_tools/relpath.pl $(i) $(MIOSIX_KERNEL_PATH)) \
	  OPT_OPTIMIZATION=$(OPT_OPTIMIZATION) \
	  || exit 1) &&) @echo Recursive build of subdirs done

build-program: $(PRIMARY_OUTPUTS)
	
size-stats:
	$(SZ) $(BINARYDIR)/$(TARGETNAME)

clean: clean-project clean-recursive

clean-project:
ifeq ($(USE_DEL_TO_CLEAN),1)
	cmd /C del /S /Q $(BINARYDIR)
else
	rm -rf $(BINARYDIR)
endif

clean-recursive:
	$(foreach i,$(RECURSIVE_BUILD_SUBDIRS),($(MAKE) -C $(i) \
	  KPATH=$(shell perl $(MIOSIX_KERNEL_PATH)/_tools/relpath.pl $(i) $(MIOSIX_KERNEL_PATH)) \
	  CONFPATH=$(shell perl $(MIOSIX_KERNEL_PATH)/_tools/relpath.pl $(i) $(MIOSIX_KERNEL_PATH)) \
	  clean || exit 1) &&) @echo Recursive clean of subdirs done
	
empty:=
space:= $(empty) $(empty)
print-includes:
	$(info Add the following folders to includes in VS to get IntelliSense working:)
	$(info $(subst $(space),;,$(subst .,,$(MIOSIX_INCLUDE_DIRS))))

#recipes
$(BINARYDIR):
	mkdir $(BINARYDIR)

$(BINARYDIR)/$(basename $(TARGETNAME)).bin: $(BINARYDIR)/$(TARGETNAME)
	$(CP) -O binary $< $@

$(BINARYDIR)/$(basename $(TARGETNAME)).ihex: $(BINARYDIR)/$(TARGETNAME)
	$(CP) -O ihex $< $@
	
$(BINARYDIR)/$(TARGETNAME): build-recursive $(EXTERNAL_LIBS) $(all_objs)
	$(CXX) -o $@ $(all_objs) $(MIOSIX_KERNEL_PATH)/$(BOOT_FILE) $(LDFLAGS) $(START_GROUP) $(LIBRARY_LDFLAGS) $(STATIC_LIBS) $(END_GROUP)
#default was: $(LD) -o $@ $(LDFLAGS) $(START_GROUP) $(all_objs) $(LIBRARY_LDFLAGS) $(END_GROUP)

#pull in dependecy info for existing .o files
-include $(all_objs:.o=.dep)

#VisualGDB: FileSpecificTemplates		#<--- VisualGDB will use the following lines to define rules for source files in subdirectories
$(BINARYDIR)/%.o : %.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) $< -o $@
#DFLAGS already added to CXXFLAGS before; -c argument aready added by Makefile.inc
#default was: $(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $< -o $@
#DFLAGS already added to CCFLAGS before; -c argument aready added by Makefile.inc
#default was: $(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.S $(all_make_files) |$(BINARYDIR)
	$(AS) $(AFLAGS) $< -o $@
#default was: $(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.s $(all_make_files) |$(BINARYDIR)
	$(AS) $(AFLAGS) $< -o $@
#default was: $(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.cc $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.cxx $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

#VisualGDB: GeneratedRules				#<--- All lines below are auto-generated
