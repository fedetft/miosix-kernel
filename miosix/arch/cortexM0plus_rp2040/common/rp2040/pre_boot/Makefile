.SUFFIXES: # disable builtin rules

E_CC?=arm-none-eabi-gcc
E_OBJCOPY?=arm-none-eabi-objcopy

OUTPUT:=boot2
OBJ_DIR = obj
SRC_DIR = .

ASM_SRC := ./boot2_w25q080.S
LD_SCRIPT = ./boot_stage2.ld

INCLUDE := \
    -I.. \
    -Iasminclude

ASM_OBJ = $(patsubst $(SRC_DIR)/%, $(OBJ_DIR)/%, $(ASM_SRC:.S=.o))
OBJ = $(ASM_OBJ)
DEPS = $(OBJ:.o=.d)

CPUFLAGS := -mcpu=cortex-m0plus -mthumb
ASMFLAGS += -ffreestanding $(CPUFLAGS) $(INCLUDE)
LDFLAGS += -ffreestanding -nolibc -nostartfiles $(CPUFLAGS) -Wl,-Map=$(OUTPUT_MAP)

OUTPUT_ELF = $(OUTPUT).elf
OUTPUT_BIN = $(OUTPUT).bin
OUTPUT_MAP = $(OUTPUT).map
OUTPUT_C   = $(OUTPUT).h

.PHONY: all
all: $(OUTPUT_C)

-include $(DEPS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S
	$(E_CC) $(ASMFLAGS) -MMD -c $< -o $@

$(OUTPUT_ELF): $(OBJ) $(LD_CONFIG)
	$(E_CC) $(LDFLAGS) -T$(LD_SCRIPT) $(OBJ) -o $@

%.bin: %.elf
	$(E_OBJCOPY) -O binary $< $@

$(OUTPUT_C): $(OUTPUT_BIN)
	./pad_checksum.py -s 0xffffffff $< $@

$(OBJ): | $(OBJ_DIR)

$(OBJ_DIR):
	mkdir -p $(dir $(OBJ))

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(OUTPUT_ELF) $(OUTPUT_BIN) $(OUTPUT_MAP) $(OUTPUT_C)
