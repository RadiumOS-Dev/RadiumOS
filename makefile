#!/usr/bin/make -f

# Compiler and tools configuration
NASM = nasm
CC = clang
LD = ld.lld
QEMU = qemu-system-i386

# Compiler flags
NASMFLAGS = -f elf32
CFLAGS = -c -target i686-none-elf -ffreestanding -mno-sse -Wall
LDFLAGS = -T linker.ld -static -nostdlib

# Directories
SRC_DIR = src
BOOT_DIR = $(SRC_DIR)/boot
GRUB_DIR = $(SRC_DIR)/grub
ISO_DIR = iso

# Source files
BOOT_ASM = $(BOOT_DIR)/boot.asm
GRUB_ASM = $(GRUB_DIR)/grub.asm
C_SOURCES = $(wildcard $(SRC_DIR)/*/*.c)

# Object files
BOOT_OBJ = $(BOOT_DIR)/boot.o
GRUB_OBJ = $(GRUB_DIR)/grub.o
C_OBJECTS = $(C_SOURCES:.c=.o)

# Output files
KERNEL_BIN = os.bin
ISO_FILE = os.iso
DISK_IMG = disk.img

# QEMU flags
QEMU_FLAGS = -rtc base=utc,clock=host,driftfix=slew \
             -cdrom $(ISO_FILE) \
             -boot order=d \
             -machine q35 \
             -m 128 \
             -smp 1 \
             -serial stdio \
             -netdev user,id=net0 \
             -device rtl8139,netdev=net0 \
             -vga std \
             -display gtk,zoom-to-fit=off,window-close=off \
             -monitor none \
             -full-screen

# Default target
.PHONY: all
all: $(ISO_FILE) $(DISK_IMG)

# Build kernel binary
$(KERNEL_BIN): $(BOOT_OBJ) $(GRUB_OBJ) $(C_OBJECTS)
	@echo "Linking"
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(GRUB_OBJ) $(C_OBJECTS)

# Build boot object
$(BOOT_OBJ): $(BOOT_ASM)
	@echo "Building boot.asm"
	$(NASM) $(NASMFLAGS) $< -o $@

# Build GRUB object
$(GRUB_OBJ): $(GRUB_ASM)
	@echo "Building grub.asm"
	$(NASM) $(NASMFLAGS) $< -o $@

# Build C object files
%.o: %.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) $< -o $@

# Create bootable ISO
$(ISO_FILE): $(KERNEL_BIN)
	@echo "Making a bootable ISO"
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/$(KERNEL_BIN)
	grub-mkrescue -o $@ $(ISO_DIR)

# Create FAT12 disk image
$(DISK_IMG):
	@echo "Creating FAT16 disk image"
	@rm -f $@
	@echo "Creating 1.44MB FAT12 floppy disk image..."
	dd if=/dev/zero of=$@ bs=1024 count=1440 2>/dev/null
	mkfs.fat -F 12 -n "TOMATOOS" $@
	@echo "Verifying disk image..."
	@if command -v file >/dev/null 2>&1; then \
		file $@; \
	fi
	@echo "Boot signature check:"
	xxd -s 510 -l 2 $@
	@if command -v mtools >/dev/null 2>&1; then \
		echo "Adding test file..."; \
		echo "Hello from TomatoOS!" > test.txt; \
		mcopy -i $@ test.txt ::test.txt 2>/dev/null || echo "Could not add test file"; \
		rm -f test.txt; \
	fi

# Run with QEMU
.PHONY: run
run: $(ISO_FILE) $(DISK_IMG)
	@echo "Booting $(ISO_FILE) with FAT disk attached"
	$(QEMU) $(QEMU_FLAGS)
	@echo "QEMU started."

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning up object files"
	rm -f $(BOOT_OBJ) $(GRUB_OBJ) $(C_OBJECTS)
	rm -f $(KERNEL_BIN) $(ISO_FILE) $(DISK_IMG)
	rm -f test.txt

# Clean only object files (like original script)
.PHONY: clean-objects
clean-objects:
	@echo "Cleaning up object files"
	rm -f $(BOOT_OBJ) $(C_OBJECTS)

# Build only the kernel
.PHONY: kernel
kernel: $(KERNEL_BIN)

# Build only the ISO
.PHONY: iso
iso: $(ISO_FILE)

# Build only the disk image
.PHONY: disk
disk: $(DISK_IMG)

# Show build information
.PHONY: info
info:
	@echo "C Sources: $(C_SOURCES)"
	@echo "C Objects: $(C_OBJECTS)"
	@echo "Boot Object: $(BOOT_OBJ)"
	@echo "GRUB Object: $(GRUB_OBJ)"
	@echo "Kernel Binary: $(KERNEL_BIN)"
	@echo "ISO File: $(ISO_FILE)"
	@echo "Disk Image: $(DISK_IMG)"

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all          - Build everything (default)"
	@echo "  kernel       - Build only the kernel binary"
	@echo "  iso          - Build only the ISO file"
	@echo "  disk         - Build only the disk image"
	@echo "  run          - Build and run with QEMU"
	@echo "  clean        - Clean all build artifacts"
	@echo "  clean-objects- Clean only object files"
	@echo "  info         - Show build information"
	@echo "  help         - Show this help message"