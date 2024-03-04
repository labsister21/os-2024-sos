# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc
MKISO					= genisoimage
QEMU_i386		  = qemu-system-i386
QEMU_img		  = qemu-img

# Directory
SOURCE_FOLDER = src
OUTPUT_FOLDER = bin
C_FOLDER			= code
A_FOLDER			= asm
CCODE_FOLDER 	= $(SOURCE_FOLDER)/$(C_FOLDER)
ACODE_FOLDER	= $(SOURCE_FOLDER)/$(A_FOLDER)
OBJ_FOLDER		= $(OUTPUT_FOLDER)/obj
COBJ_FOLDER		= $(OBJ_FOLDER)/$(C_FOLDER)
AOBJ_FOLDER		= $(OBJ_FOLDER)/$(A_FOLDER)

# File
CCODE = $(call RECUR_WILDCARD,$(CCODE_FOLDER),*.c)
ACODE = $(call RECUR_WILDCARD,$(ACODE_FOLDER),*.s)
KERNEL_NAME		= kernel
ISO_NAME      = os2024
DISK_NAME			= storage
INSERTER_NAME = inserter

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -fshort-wchar -g
INCLUDE_CFLAG = -I$(SOURCE_FOLDER)
SYSTEM_INCLUDE_CFLAG	= -isystem src/include
STRIP_CFLAG   = -nostdlib -nostdinc -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) $(INCLUDE_CFLAG) $(SYSTEM_INCLUDE_CFLAG) -m32 -c
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386
QFLAGS_DRIVE	= -drive file=$(OUTPUT_FOLDER)/$(DISK_NAME).bin,format=raw,if=ide,index=0,media=disk
QFLAGS_ISO		= -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso 
QFLAGS_DBG		= -s -S

# Helper functions
RECUR_WILDCARD=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call RECUR_WILDCARD,$d/,$2))
C_TO_O = $(patsubst $(SOURCE_FOLDER)%,$(OBJ_FOLDER)%,$(patsubst %.c,%.o,$1))
A_TO_O = $(patsubst $(SOURCE_FOLDER)%,$(OBJ_FOLDER)%,$(patsubst %.s,%.o,$1))


.PHONY: run rerun dbg inserter all build rebuild disk clean 

# Qemu debug flag: -s and -S
run: all
	@$(QEMU_i386) $(QFLAGS_DRIVE) $(QFLAGS_ISO)
rerun:
	@make clean
	@make run

# gdb $(OUTPUT_FOLDER)/$(KERNEL_NAME) 
dbg: 
	@($(QEMU_i386) $(QFLAGS_DBG) $(QFLAGS_DRIVE) $(QFLAGS_ISO) &)

inserter: $(OUTPUT_FOLDER)/$(INSERTER_NAME)

all: build inserter disk

build: $(OUTPUT_FOLDER)/$(ISO_NAME).iso
rebuild:
	@make clean
	@make build

disk: $(OUTPUT_FOLDER)/$(DISK_NAME).bin

clean:
	rm -rf $(OUTPUT_FOLDER)

$(COBJ_FOLDER)/%.o : $(CCODE_FOLDER)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(AOBJ_FOLDER)/%.o : $(ACODE_FOLDER)/%.s
	@mkdir -p $(@D)
	$(ASM) $(AFLAGS) $< -o $@

$(OUTPUT_FOLDER)/$(KERNEL_NAME): $(call C_TO_O,$(CCODE)) $(call A_TO_O,$(ACODE))
	$(LIN) $(LFLAGS) $^ -o $@

$(OUTPUT_FOLDER)/$(DISK_NAME).bin:
	@$(QEMU_img) create -f raw $@ 4M

$(OUTPUT_FOLDER)/$(ISO_NAME).iso: $(OUTPUT_FOLDER)/$(KERNEL_NAME)
	@mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	@cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	@cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	@cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
	$(MKISO) -R \
		-b boot/grub/grub1 \
		-no-emul-boot \
		-boot-load-size 4 \
		-A os \
		-input-charset utf8 \
		-quiet \
		-boot-info-table \
		-o bin/$(ISO_NAME).iso \
		bin/iso
	@rm -r $(OUTPUT_FOLDER)/iso/

$(OUTPUT_FOLDER)/$(INSERTER_NAME): 
	@mkdir -p $(@D)
	$(CC) $(INCLUDE_CFLAG) $(WARNING_CFLAG) \
		-Wno-builtin-declaration-mismatch -g -o $@ \
		$(SOURCE_FOLDER)/code/stdlib/string.c \
		$(SOURCE_FOLDER)/code/filesystem/fat32.c \
		$(SOURCE_FOLDER)/other/inserter.c \

