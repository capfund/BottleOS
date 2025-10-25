# ============================
# BottleOS Build Makefile
# ============================

BUILD_DIR = build

CC = gcc
LD = ld
AS = nasm

ARCH := i386

ENTRY_POINT = src/entry.asm
ENTRY_OBJ = $(BUILD_DIR)/entry.o

KERNEL_SRC = $(wildcard src/*.c)
KERNEL_OBJ = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(KERNEL_SRC))

LINKER_SCRIPT = src/link.ld

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -Iinclude
LINKER_FLAGS =
ASFLAGS =

ISO_DIR = $(BUILD_DIR)/iso
GRUB_CFG = $(ISO_DIR)/boot/grub/grub.cfg
ISO_IMAGE = $(BUILD_DIR)/BottleOS.iso

ifeq ($(ARCH), x86_64)
CFLAGS += -m64
LINKER_FLAGS += -m elf_x86_64
ASFLAGS += -f elf64
else
CFLAGS += -m32
LINKER_FLAGS += -m elf_i386
ASFLAGS += -f elf32
endif

.PHONY: all clean run iso run-iso dirs

# ==================================
# Build kernel binary
# ==================================

all: dirs $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: $(ENTRY_OBJ) $(KERNEL_OBJ)
	$(LD) $(LINKER_FLAGS) -T $(LINKER_SCRIPT) -o $@ $^

$(ENTRY_OBJ): $(ENTRY_POINT)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $< -MMD -MF $(@:.o=.d)

# ==================================
# ISO generation section
# ==================================

iso: dirs $(ISO_IMAGE)

$(ISO_IMAGE): $(BUILD_DIR)/kernel.bin $(GRUB_CFG)
	grub-mkrescue -o $@ $(ISO_DIR) --modules="part_msdos part_gpt normal multiboot multiboot2" \
		--product-name="BottleOS" --product-version="1.0"

$(GRUB_CFG):
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD_DIR)/kernel.bin $(ISO_DIR)/boot/
	echo 'set timeout=0' > $(GRUB_CFG)
	echo 'set default=0' >> $(GRUB_CFG)
	echo 'menuentry "BottleOS" {' >> $(GRUB_CFG)
	echo '    multiboot /boot/kernel.bin' >> $(GRUB_CFG)
	echo '    boot' >> $(GRUB_CFG)
	echo '}' >> $(GRUB_CFG)

# ==================================
# Run targets
# ==================================

run: $(BUILD_DIR)/kernel.bin
	qemu-system-x86_64 -kernel $<

run-iso: iso
	qemu-system-x86_64 -cdrom $(ISO_IMAGE)

# ==================================
# Utility targets
# ==================================

dirs:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)
