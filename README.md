# OStrich
Distributed OS

## Prerequisites

### **Linux**
1. **Cross-Compiler:**  
   If you're on x86 Linux (assuming Ubuntu), install the `aarch64-linux-gnu` toolchain:
   ```sh
   sudo apt install gcc-aarch64-linux-gnu
   sudo apt install g++-aarch64-linux-gnu
   ```
   You may also need to install `build-essential` for Ubuntu distributions
   ```sh
   sudo apt install build-essential
   ```

2. **QEMU:**  
   Install QEMU for ARM:
   ```sh
   sudo apt install qemu-system-arm
   ```

3. **Make:**  
   Install `make`:
   ```sh
   sudo apt install make
   ```

4. **GDB:**  
   Install `gdb` for debugging:
   ```sh
   sudo apt install gdb
   ```

---

### **Windows**
1. **Cross-Compiler:**  
   If you're using Windows, I would recommend to use WSL so that you can interact with the Makefile and the commands as you would on a Linux system. If you do it this way, setting everything up should be the same as you would using Linux except to use GDB you may need to use `gdb-multiarch` instead of `gdb`

4. **GDB:**  
   Use `gdb-multiarch` for debugging:
   ```sh
   sudo apt install gdb-multiarch
   ```
   You should also modify the `gdb_start.sh` script and replace `gdb` with `gdb-multiarch`:
   ```bash
   #!/bin/bash
   gdb-multiarch -ex "file build/kernel.elf" -ex "target remote localhost:1234"
   ```

---

### **macOS**
I have not been able to personally test this on macOS, so if anyone who successfully gets it working can provide updates or corrections, that would be great. However, I believe these here should be the general steps to get everything installed:
1. **Cross-Compiler:**  
   On x86 macOS, you should install the `aarch64-elf-gcc` toolchain using Homebrew:
   ```sh
   brew install aarch64-elf-gcc
   ```
   You should also update the `CROSS_COMPILE` variable at the top of the Makefile from `aarch64-linux-gnu-` to `aarch64-elf-`:
   ```makefile
   CROSS_COMPILE = aarch64-elf-
   ```

2. **QEMU:**  
   Install QEMU for ARM using Homebrew:
   ```sh
   brew install qemu
   ```

3. **Make:**  
   You may need to install `make` on macOS
   ```sh
   xcode-select --install
   ```

4. **GDB:**  
   Install `gdb` using Homebrew:
   ```sh
   brew install gdb
   ```

---

### **If Your System Compiles Natively for ARM**
If your computer compiles natively for ARM, you can simplify the Makefile. Change the `CROSS_COMPILE` variable at the top of the Makefile from:
```makefile
CROSS_COMPILE = aarch64-linux-gnu-
```
to:
```makefile
CROSS_COMPILE =
```
This way the Makefile will use the default system compilers (`gcc`, `g++`) instead of cross-compilers.

---
## Makefile

1. **Build the Kernel:**
   To compile the kernel and generate the kernel image (`kernel8.img`), run:
   ~~~sh
   make
   ~~~
   This will compile all .S and .cpp files and then link all of the compiled files into a kernel.elf file. Then it will convert the kernel.elf file into the kernel8.img file. The kernel8.img file will be placed in a directory named `build`

2. **Clean the Build Directory:**
   to remove all generated files and clean up the build directory, run:
   ~~~sh
   make clean
   ~~~
   This will delete the `build` directory and its contents.

3. **Run the Kernel in QEMU:**
   To emulate the kernel on a Raspberry Pi using QEMU, run:
   ~~~sh
   make run
   ~~~
   This will launch QEMU emulating the Raspberry Pi and display output through the serial console in the terminal.

4. **Debug the Kernel:**
   To debug the kernel with GDB, run:
   ~~~sh
   make debug
   ~~~
   This will start QEMU in debugging mode with a GDB server listening on tcp::1234.  
    You can connect to the GDB session by running the gdb_start.sh script in a separate terminal window. This script loads the kernel.elf file, which contains the debugging information, and makes a connection to the GDB server:
   ~~~sh
   ./gdb_start.sh
   ~~~
   The script contains this command:
   ~~~sh
    gdb -ex "file build/kernel.elf" -ex "target remote localhost:1234"
    ~~~

