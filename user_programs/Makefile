LIBC_FILES=./ostrich-libc
INCLUDE_DIR=$(LIBC_FILES)/include
LIB_DIR=$(LIBC_FILES)/lib
LINKER_SCRIPT=app.ld

CURRENT_OS = $(strip $(shell uname -s)) # semi portable ig.
ifeq ($(findstring Darwin,$(CURRENT_OS)),Darwin) # MACOS
    TOOLCHAIN_PREFIX = aarch64-elf-
else
    TOOLCHAIN_PREFIX = aarch64-linux-gnu-
endif

CC=$(TOOLCHAIN_PREFIX)gcc
LD=$(TOOLCHAIN_PREFIX)ld
OBJCOPY=$(TOOLCHAIN_PREFIX)objcopy

CFLAGS=-Wall -Wextra -I$(INCLUDE_DIR) -g \
       -mcpu=cortex-a53 -march=armv8-a+crc -mstrict-align -mno-outline-atomics \
       -fno-exceptions -O0

# Used when a stdlib is not present (it is now 😎)
# -no-stdlib
# -ffreestanding
    
LDFLAGS=-z max-page-size=0x1000 -L $(LIB_DIR) -T $(LINKER_SCRIPT)
# -T $(LINKER_SCRIPT), when we want to use our own linker script
# -latomic, for a library (.a / .so) called "atomics

all: user_prog

libc_patching.o: libc_patching.c
	$(CC) $(CFLAGS) -c libc_patching.c -o libc_patching.o

user_prog.o: user_prog.c
	$(CC) $(CFLAGS) -c user_prog.c -o user_prog.o 

fs_syscall_test.o: fs_syscall_test.c
	$(CC) $(CFLAGS) -c fs_syscall_test.c -o fs_syscall_test.o

exit.o: exit.c
	$(CC) $(CFLAGS) -c exit.c -o exit.o 

malloc.o: malloc.c
	$(CC) $(CFLAGS) -c malloc.c -o malloc.o	

newlib_stubs.o: newlib_stubs.S
	$(CC) $(CFLAGS) -c newlib_stubs.S -o newlib_stubs.o	

user_prog: $(LIB_DIR)/crt0.o user_prog.o newlib_stubs.o malloc.o
	$(LD) $(LDFLAGS) -o user_prog $(LIB_DIR)/crt0.o user_prog.o newlib_stubs.o malloc.o -lc -entry=main
	cp user_prog ../ramfs/files

exit: $(LIB_DIR)/crt0.o exit.o newlib_stubs.o
	$(LD) $(LDFLAGS) -o exit $(LIB_DIR)/crt0.o exit.o newlib_stubs.o -lc -entry=main

fs_syscall_test: $(LIB_DIR)/crt0.o fs_syscall_test.o newlib_stubs.o libc_patching.o
	$(LD) $(LDFLAGS) -o fs_syscall_test $(LIB_DIR)/crt0.o fs_syscall_test.o newlib_stubs.o libc_patching.o -lc -entry=main
	cp fs_syscall_test ../ramfs/files/
	aarch64-elf-objdump -d fs_syscall_test > fs_syscall.dump


clean:
	rm -f *.o user_prog fs_syscall_test fs_syscall.dump