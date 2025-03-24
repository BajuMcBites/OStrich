# Cross compiler and tools
# aarch64-linux-gnu- for windows, aarch64-elf- for mac
CROSS_COMPILE = aarch64-none-elf-
AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

# Directories
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include
RAMFS_DIR = ramfs

# Automatically find all source files
ASM_SRC = $(wildcard $(SRC_DIR)/*.S)
C_SRC = $(wildcard $(SRC_DIR)/*.c)
CPP_SRC = $(wildcard $(SRC_DIR)/*.cpp)

# Object files
ASM_OBJ = $(ASM_SRC:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_S.o)
C_OBJ = $(C_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
CPP_OBJ = $(CPP_SRC:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%_cpp.o)
RAMFS_OBJ = $(BUILD_DIR)/ramfs.o

# Output files
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_IMG = $(BUILD_DIR)/kernel8.img
RAMFS_IMG = $(BUILD_DIR)/ramfs.img

# Compiler and linker flags
CFLAGS = -Wall -Wextra -nostdlib -ffreestanding -I$(INCLUDE_DIR) -g -mcpu=cortex-a53 -march=armv8-a+crc -latomic -mstrict-align -mno-outline-atomics -fno-rtti -fno-exceptions -fno-rtti
LDFLAGS = -T linker.ld  # Use the custom linker script

# Build rules
all: $(KERNEL_IMG)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Preprocess and assemble the assembly files
$(ASM_OBJ): $(BUILD_DIR)/%_S.o : $(SRC_DIR)/%.S | $(BUILD_DIR)
	$(CC) -E -x assembler-with-cpp -I$(INCLUDE_DIR) $< -o $(BUILD_DIR)/$(<F).i
	$(AS) $(BUILD_DIR)/$(<F).i -o $@
	rm $(BUILD_DIR)/$(<F).i

# Compile the C source files
$(C_OBJ): $(BUILD_DIR)/%_c.o : $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile the C++ source files
$(CPP_OBJ): $(BUILD_DIR)/%_cpp.o : $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

$(RAMFS_IMG) : $(BUILD_DIR)
	cd $(RAMFS_DIR) && g++ build_ramfs.cpp -o ../$(BUILD_DIR)/build_ramfs
	cd $(RAMFS_DIR)/files && find . -type f -exec basename {} \; | xargs ../../$(BUILD_DIR)/build_ramfs
	mv $(RAMFS_DIR)/ramfs.img $(BUILD_DIR)

# Compile ramfs into source file
$(RAMFS_OBJ) : $(RAMFS_IMG)
	$(OBJCOPY) --input binary -O elf64-littleaarch64 $(RAMFS_IMG) $(RAMFS_OBJ)

# Link the object files into the kernel ELF
$(KERNEL_ELF): $(ASM_OBJ) $(C_OBJ) $(CPP_OBJ) $(RAMFS_OBJ)
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(ASM_OBJ) $(C_OBJ) $(CPP_OBJ) $(RAMFS_OBJ)

# Convert the ELF to a binary image
$(KERNEL_IMG): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_IMG)

clean:
	rm -rf $(BUILD_DIR)

run:
	qemu-system-aarch64 -M raspi3b -kernel $(KERNEL_IMG) -smp 4 -serial stdio

debug:
	qemu-system-aarch64 -M raspi3b -kernel $(KERNEL_IMG) -smp 4 -serial stdio -S -gdb tcp::1234

.PHONY: ramfs
ramfs:
	cd $(RAMFS_DIR) && g++ build_ramfs.cpp -o ../$(BUILD_DIR)/build_ramfs
	cd $(RAMFS_DIR)/files && find . -type f -exec basename {} \; | xargs ../../$(BUILD_DIR)/build_ramfs
	mv $(RAMFS_DIR)/ramfs.img $(BUILD_DIR)
