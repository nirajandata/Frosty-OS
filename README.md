# ❄️ FrostyOS

> [!NOTE]  
> This is not a Linux-based distro.


FrostyOS is a bare-metal x86_64 operating system kernel written in C++ 23 and Assembly with the Limine bootloader.


##  Prerequisites (Arch Linux)

To build FrostyOS, you need the following packages installed on your Arch system:

```bash
sudo pacman -S git base-devel cmake ninja limine xorriso qemu-full
```

*   **Limine**: Provides the bootloader binaries.
*   **Xorriso**: Used to package the kernel and config into an `.iso` file.
*   **QEMU**: The emulator used to run the OS.
*   **Ninja**: The fast build-generator used by the build script.

---

##  Project Structure

```text
.
├── kernel/
│   ├── main.cpp        # Kernel entry point (kmain)
│   ├── linker.ld       # Kernel memory layout
│   └── limine.h        # Limine boot protocol header (auto-downloaded)
├── build.sh            # Main build automation script
├── CMakeLists.txt      # Modern CMake configuration
├── limine.conf         # Bootloader configuration
└── .gitignore          # Git and JetBrains/CLion exclusions
```

---

## Building and Running

The project includes a `build.sh` script to simplify the CMake and Ninja workflow.

### 1. Build the ISO
This compiles the kernel and generates `build/frosty_os.iso`.
```bash
./build.sh
```

### 2. Build and Run in QEMU
This builds the latest changes and immediately launches QEMU.
```bash
./build.sh all
```

### 3. Run existing build
If you have already built the ISO and just want to launch it:
```bash
./build.sh run
```

##  License
This project is provided for educational purposes. Feel free to "meow" all over the code. 🐱
