# Directories
SRC_DIR     := kernel
BUILD_DIR   := build
ISO_DIR     := $(BUILD_DIR)/iso
BOOT_DIR    := $(ISO_DIR)/boot
GRUB_DIR    := $(BOOT_DIR)/grub

# Source files
ASM_SRC     := $(SRC_DIR)/entry.asm
C_SRC       := $(SRC_DIR)/main.c
VGA_SRC     := $(SRC_DIR)/vga.c
LINKER      := $(SRC_DIR)/linker.ld

# Object files
ASM_OBJ     := $(BUILD_DIR)/entry.o
C_OBJ       := $(BUILD_DIR)/main.o
VGA_OBJ     := $(BUILD_DIR)/vga.o

# Output files
KERNEL_ELF  := $(BUILD_DIR)/kernel.elf
KERNEL_BIN  := $(BUILD_DIR)/kernel.bin
ISO         := $(BUILD_DIR)/slops.iso
GRUB_CFG    := $(GRUB_DIR)/grub.cfg

# Compiler/Linker flags
CFLAGS      := -ffreestanding -m32 -O2 -Wall -Wextra -fno-pie -fno-pic
LDFLAGS     := -T $(LINKER) -melf_i386

# Tools
AS          := nasm
CC          := gcc
LD          := ld
OBJCOPY     := objcopy
QEMU        := qemu-system-i386

# Default target
all: $(ISO)

# Ensure directories exist
$(BUILD_DIR) $(ISO_DIR) $(BOOT_DIR) $(GRUB_DIR):
	mkdir -p $@

# Compile assembly
$(ASM_OBJ): $(ASM_SRC) | $(BUILD_DIR)
	$(AS) -f elf32 $< -o $@

# Compile C files
$(C_OBJ): $(C_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(VGA_OBJ): $(VGA_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel ELF
$(KERNEL_ELF): $(ASM_OBJ) $(C_OBJ) $(VGA_OBJ) $(LINKER) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(ASM_OBJ) $(C_OBJ) $(VGA_OBJ)

# Optional: raw binary
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

# GRUB config (unchanged from your version)
$(GRUB_CFG): | $(GRUB_DIR)
	echo 'set timeout=5' > $@
	echo 'set default=0' >> $@
	echo 'set gfxpayload=text' >> $@
	echo 'terminal_output console' >> $@
	echo 'menuentry "SLOPS" {' >> $@
	echo '  echo "GRUB loaded successfully"' >> $@
	echo '  multiboot2 /boot/kernel.elf' >> $@
	echo '  boot' >> $@
	echo '}' >> $@

# Create ISO (depends on kernel + grub.cfg)
$(ISO): $(KERNEL_ELF) $(GRUB_CFG) | $(BOOT_DIR)
	cp $(KERNEL_ELF) $(BOOT_DIR)/kernel.elf
	grub-mkrescue -o $@ $(ISO_DIR)

# Run in QEMU with debug port and ISO listing
run: $(ISO)
	@echo "---- ISO /boot ----"
	@xorriso -indev $(ISO) -ls /boot
	@echo "---- ISO /boot/grub ----"
	@xorriso -indev $(ISO) -ls /boot/grub
	$(QEMU) -cdrom $(ISO) -m 512 -vga std -no-reboot \
		-debugcon stdio -global isa-debugcon.iobase=0xe9

# Force full rebuild
rebuild: clean all

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run rebuild