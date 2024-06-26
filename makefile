# Compiler & Linker
ASM = nasm
LIN = ld
CC = gcc
NATIVE_CC = gcc
MKISO = genisoimage
QEMU_i386 = qemu-system-i386
QEMU_img = qemu-img

# Name
KERNEL_NAME = kernel
DISK_NAME = storage.bin
INSERTER_NAME = inserter
ISO_NAME = os2024.iso

SOURCE_PATH = src
OUTPUT_PATH = bin
OBJECT_PATH = $(OUTPUT_PATH)/obj

# C Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG = -fshort-wchar -g
TARGET_CFLAG = -m32
SHARED_INCLUDE_CFLAG = -isystem $(SOURCE_PATH)/shared/header
STRIP_CFLAG = -nostdlib -nostdinc -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding -fno-pie

# Flags
ASM_FLAGS = -f elf32 -g -F dwarf
C_FLAGS = $(WARNING_CFLAG) $(DEBUG_CFLAG) $(STRIP_CFLAG) $(SHARED_INCLUDE_CFLAG) $(TARGET_CFLAG) -c
KERNEL_C_FLAGS = $(C_FLAGS) -I $(SOURCE_PATH)/kernel/header
PROGRAM_C_FLAGS = $(C_FLAGS)
SHARED_C_FLAGS = $(C_FLAGS)

LINKER_FLAGS = -T $(SOURCE_PATH)/kernel/linker.ld -melf_i386

.SECONDARY:
.SECONDEXPANSION:
.PHONY: 

run: all
	$(QEMU_i386) \
		-m 2G \
		-drive file=$(OUTPUT_PATH)/$(DISK_NAME),format=raw,if=ide,index=0,media=disk \
		-cdrom $(OUTPUT_PATH)/$(ISO_NAME)

clean:
	rm -rf bin

all: iso disk

# Fn
SOURCE_TO_OBJECT = $(patsubst $(SOURCE_PATH)%,$(OBJECT_PATH)%,$1)
C_TO_O = $(call SOURCE_TO_OBJECT,$(patsubst %.c,%.o,$1))
ASM_TO_O = $(call SOURCE_TO_OBJECT,$(patsubst %.s,%.o,$1))
RECUR_WILDCARD=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call RECUR_WILDCARD,$d/,$2))
PROGRAM_CODE = $(call C_TO_O,$(call RECUR_WILDCARD,$(SOURCE_PATH)/program/$1,*.c))

# Object file recipe
$(OBJECT_PATH)/shared/%.o: $(SOURCE_PATH)/shared/%.c
	@mkdir -p $(@D)
	$(CC) $(SHARED_C_FLAGS) -c -o $@ $<
$(OBJECT_PATH)/program/%.o: $(SOURCE_PATH)/program/%.c
	@mkdir -p $(@D)
	$(CC) $(PROGRAM_C_FLAGS) -I $(dir $<) -c -o $@ $<
$(OBJECT_PATH)/kernel/%.o: $(SOURCE_PATH)/kernel/%.c
	@mkdir -p $(@D)
	$(CC) $(KERNEL_C_FLAGS) -c -o $@ $<

$(OBJECT_PATH)/%.o: $(SOURCE_PATH)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -c -o $@ $<


$(OBJECT_PATH)/%.o: $(SOURCE_PATH)/%.s
	@mkdir -p $(@D)
	$(ASM) $(ASM_FLAGS) $< -o $@

SHARED_C = $(call C_TO_O,$(call RECUR_WILDCARD,$(SOURCE_PATH)/shared/code,*.c))
KERNEL_C = $(call C_TO_O,$(call RECUR_WILDCARD,$(SOURCE_PATH)/kernel/c,*.c))
KERNEL_ASM = $(call ASM_TO_O,$(call RECUR_WILDCARD,$(SOURCE_PATH)/kernel/asm,*.s))
$(OUTPUT_PATH)/$(KERNEL_NAME): $(KERNEL_C) $(KERNEL_ASM) $(SHARED_C)
	@mkdir -p $(@D)
	$(LIN) $(LINKER_FLAGS) $^ -o $@
kernel: $(OUTPUT_PATH)/$(KERNEL_NAME)

# User program recipe
$(OBJECT_PATH)/program/crt0.o: $(SOURCE_PATH)/program/crt0.s
	@mkdir -p $(@D)
	$(ASM) $(ASM_FLAGS) $< -o $@

PROGRAM_LINKER_FLAGS = -T $(SOURCE_PATH)/program/user-linker.ld -melf_i386
$(OUTPUT_PATH)/program/%: $$(call PROGRAM_CODE,$$*) $(OBJECT_PATH)/program/crt0.o $(SHARED_C)
	@mkdir -p $(@D)
	$(LIN) $(PROGRAM_LINKER_FLAGS) $^ -o $@
prog.%: $(OUTPUT_PATH)/program/%
	@
insprog.%: disk inserter prog.%
	cd $(OUTPUT_PATH)/program; ../$(INSERTER_NAME) $(patsubst insprog.%,%,$@) 2 ../$(DISK_NAME)

PROGRAM_PATH = $(SOURCE_PATH)/program
PROGRAM_LIST = $(patsubst $(PROGRAM_PATH)/%,%,$(foreach d,$(wildcard $(PROGRAM_PATH)/*),$(if $(wildcard $(d)/*),$(d),)))

all-program: $(addprefix prog.,$(PROGRAM_LIST))
insert-all-program: $(addprefix insprog.,$(PROGRAM_LIST))

# ISO
$(OUTPUT_PATH)/$(ISO_NAME): $(OUTPUT_PATH)/$(KERNEL_NAME)
	@mkdir -p $(OUTPUT_PATH)/iso/boot/grub
	@cp $(OUTPUT_PATH)/kernel $(OUTPUT_PATH)/iso/boot/
	@cp other/grub1 $(OUTPUT_PATH)/iso/boot/grub/
	@cp $(SOURCE_PATH)/menu.lst $(OUTPUT_PATH)/iso/boot/grub/
	$(MKISO) -R \
		-b boot/grub/grub1 \
		-no-emul-boot \
		-boot-load-size 4 \
		-A os \
		-input-charset utf8 \
		-quiet \
		-boot-info-table \
		-o bin/$(ISO_NAME) \
		bin/iso
	@rm -r $(OUTPUT_PATH)/iso/
iso: $(OUTPUT_PATH)/$(ISO_NAME)

# Disk
$(OUTPUT_PATH)/$(DISK_NAME):
	@mkdir -p $(@D)
	@$(QEMU_img) create -f raw $@ 4M
disk: $(OUTPUT_PATH)/$(DISK_NAME)
redisk:
	rm -rf $(OUTPUT_PATH)/$(DISK_NAME)
	make disk

bear-all: all inserter
bear:
	make clean
	rm -rf compile_commands.json
	CC=$(NATIVE_CC) bear --append -- make inserter NATIVE_CC=cc
	CC=$(CC) bear --append -- make all-program CC=cc LIN=$(LIN)
	CC=$(CC) bear --append -- make kernel CC=cc LIN=$(LIN)

# Inserter
KERNEL_CODE = $(SOURCE_PATH)/kernel/c
$(OUTPUT_PATH)/$(INSERTER_NAME): 
	@mkdir -p $(@D)
	$(CC) $(WARNING_CFLAG) \
		-I$(SOURCE_PATH)/kernel/header \
		$(SHARED_INCLUDE_CFLAG) \
		-Wno-builtin-declaration-mismatch -g \
		$(SOURCE_PATH)/helper/old_fat32.c \
		$(SOURCE_PATH)/helper/inserter.c \
		-o $@
inserter:
	@make $(OUTPUT_PATH)/$(INSERTER_NAME) CC=$(NATIVE_CC)
