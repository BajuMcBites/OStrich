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
   You may also need to install `build-essential` for Ubuntu distributions:
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
   If you're using Windows, it is recommended to use WSL so you can interact with the Makefile and commands as you would on Linux. The setup is similar to Linux, except that for debugging you might need to use `gdb-multiarch` instead of `gdb`.

2. **GDB:**  
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
I have not been able to personally test this on macOS, so if anyone successfully gets it working, updates or corrections would be appreciated. However, these should be the general steps:
1. **Cross-Compiler:**  
   On x86 macOS, install the `aarch64-elf-gcc` toolchain using Homebrew:
   ```sh
   brew install aarch64-elf-gcc
   ```
   Also, update the `CROSS_COMPILE` variable at the top of the Makefile from `aarch64-linux-gnu-` to `aarch64-elf-`:
   ```makefile
   CROSS_COMPILE = aarch64-elf-
   ```

2. **QEMU:**  
   Install QEMU for ARM using Homebrew:
   ```sh
   brew install qemu
   ```

3. **Make:**  
   You may need to install `make` on macOS:
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
This way, the Makefile will use the default system compilers (`gcc`, `g++`) instead of cross-compilers.

---

## Makefile

1. **Build the Kernel:**  
   To compile the kernel and generate the kernel image (`kernel8.img`), run:
   ~~~sh
   make
   ~~~
   This will compile all `.S` and `.cpp` files and then link them into a `kernel.elf` file. Afterwards, it converts `kernel.elf` into the `kernel8.img` file, which will be placed in the `build` directory.

2. **Clean the Build Directory:**  
   To remove all generated files and clean up the build directory, run:
   ~~~sh
   make clean
   ~~~
   This deletes the `build` directory and its contents.

3. **Run the Kernel in QEMU:**  
   To emulate the kernel on a Raspberry Pi using QEMU, run:
   ~~~sh
   make run
   ~~~
   This launches QEMU emulating the Raspberry Pi and displays output through the serial console in the terminal.

4. **Debug the Kernel:**  
   To debug the kernel with GDB, run:
   ~~~sh
   make debug
   ~~~
   This starts QEMU in debugging mode with a GDB server listening on tcp::1234.  
   You can connect to the GDB session by running the `gdb_start.sh` script in a separate terminal window. This script loads the `kernel.elf` file (which contains debugging information) and connects to the GDB server:
   ~~~sh
   ./gdb_start.sh
   ~~~
   The script contains:
   ~~~sh
   gdb -ex "file build/kernel.elf" -ex "target remote localhost:1234"
   ~~~
   On ARM macOS, use:
   ~~~sh
   make debug_mac
   ~~~
   and:
   ~~~sh
   ./gdb_start_mac.sh
   ~~~
   which runs:
   ~~~sh
   lldb build/kernel.elf -o "gdb-remote localhost:1234"
   ~~~

---

## Hardware Deployment Guide

Quick note: Not everything that works in QEMU will work on real hardware. Still, you should be able to see at least minimal HDMI output (text from the framebuffer) if the kernel boots correctly.

Follow these steps to run OStrich on actual Raspberry Pi 3B hardware:

1. **Prepare Your SD Card with Raspberry Pi OS:**  
   - **Download and Flash:**  
     Download the Raspberry Pi OS Imager from [here](https://www.raspberrypi.com/software/). Insert your SD card into your computer and use the Raspberry Pi OS Imager to flash the official Raspberry Pi OS image onto the card.
   - **Boot and Verify:**  
     Insert the SD card into your Raspberry Pi and power it on to verify that Raspberry Pi OS is running correctly.

2. **Replace the Config File:**  
   - Power down your Raspberry Pi and remove the SD card.
   - Insert the SD card into your computer.
   - Locate the `config.txt` file in the boot partition of the SD card.
   - Replace the `config.txt` with the one provided at the root of this branch.

3. **Compile and Update the Kernel:**  
   - On your development system, compile the kernel by running:
     ~~~sh
     make
     ~~~
   - Locate the newly generated `kernel8.img` file in the `build` directory.
   - Copy the new `kernel8.img` onto the SD card, replacing the original kernel image from Raspberry Pi OS.

4. **Final Hardware Setup:**  
   - Reinsert the SD card into your Raspberry Pi.
   - Attach an HDMI cable to connect the Raspberry Pi to a display (important to do this first before connecting the power).
   - Connect the micro USB power cord to power up the Raspberry Pi.
   - Your Raspberry Pi should now boot using the compiled OStrich kernel.

5. **Optional: Enable UART Output for Debugging:**  
   If you would like to view the UART (serial) output for debugging or monitoring, please follow the instructions in [Adafruit's guide for using a console cable with the Raspberry Pi](https://cdn-learn.adafruit.com/downloads/pdf/adafruits-raspberry-pi-lesson-5-using-a-console-cable.pdf).  
   Additionally, we recommend using a USB to TTL serial cable that is known to work reliably. We have had trouble with some other cables in the past. For example, the [JANSANE PL2303TA Serial Console Cable](https://www.amazon.com/JANSANE-PL2303TA-Serial-Console-Raspberry/dp/B07D9R5JFK/ref=sr_1_3?crid=OJXOYINP5UO6&dib=eyJ2IjoiMSJ9.eiTugVkWdjJ7Y-P2KuBSZQrTI3KR4vUJdKZ-jKycdC5Wm2uY7CPNTQTRUEKiEKiF-A5LDD2KkkGTxIv1QthMQViJSxUoUGr4-u0aARTkKhZjA_UTAqrgkoYD54H597mTK_f3p8Jttdi54nuQ04EHR0lM6NHpB55HYqOtBvLQnPoGT6-wRn0vnOYEAwu1lg0Oim0ppcPQrXqs3Fk4v_HOyQ1TZPqrNeUDfOJFoc-cXic.Qmy7RDgy5-rqOFEfhVM3Cnk4XNd5gZMD2f8B6oISIQI&dib_tag=se&keywords=usb+to+ttl+serial+cable&qid=1742672230&sprefix=usb+to+ttl+%2Caps%2C136&sr=8-3) has been tested and is known to work.  
   If you are on Windows and encounter difficulties getting the UART to work, I personally had issues until I watched [this troubleshooting video](https://www.youtube.com/watch?v=Y7JmCKCovMI&ab_channel=RicksTechRepairs), which helped resolve the problem.