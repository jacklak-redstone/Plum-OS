# Plum-OS
- Very basic 64bit *kernel* made by PLSiorbpl and Imated in **C++** and **asm**.
- _**Should**_ work on all modern and older CPUs supporting long mode (64bit mode).
- Boots in **legacy** mode **(be careful on real machine)**

## Structure
```bash
Plum-OS/
├── isodir/   # used to create bootable .iso
├── src/      # Source code
│   ├── arch/       # All arch dependent stuff
│   │   └── x86/
│   │
│   ├── Drivers/    # Drivers code
│   ├── kernel/     # Kernel code
│   ├── PLib/       # Standard library
│   └── Kernel.cpp  # Main Kernel file
│
├── CMakeLists.txt    # Build Kernel.elf, iso and runs it in qemu
├── build.bash  # Alternative to CMake
└── Linker.ld   # Linker settings
```

### Commands
- help
- clear
- echo
- poweroff (QEMU only)
- sleep
- heap

## Building
Using CMake:
```bash
> mkdir build/
> cd build/
> cmake ..
> make run    # Automatically runs qemu (VM) and build ISO
# Alternatively just make (wont build ISO and run qemu)
> make
```