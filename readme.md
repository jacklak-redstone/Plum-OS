# Plum-OS
- Very basic 64bit UEFI kernel made by PLSiorbpl and Imated in **C++**
- _**Should**_ work on all modern and older Computers supporting UEFI
- Boots in **UEFI** **(be careful on real machine)**

## Structure
```bash
Plum-OS/
├── src/      # Kernel Source code
│   ├── arch/       # All arch dependent stuff
│   │   └── x86/
│   │
│   ├── Drivers/    # Drivers (xHCI, aHCI, GPU, ATA)
│   ├── kernel/     # kernel systems
│   ├── libs/       # Libraries
│   │   └── std/    # Standard library (std)
│   ├── kernel.cpp/ # Kernel start
│   └── kernel.h 
│
├── CMakeLists.txt    # Build UEFI kernel and runs it in QEMU
└── Linker.ld   # Linker script
```

### Commands
- help
- clear
- poweroff (QEMU only)
- sleep
- heap
- pci
- size
- usb
- colors

## Building
Using CMake:
```bash
> mkdir build/
> cd build/
> cmake ..
> make run    # Automatically runs qemu (VM) and build ISO

# Alternative: Make only needed Kernel.elf and BOOTX64.EFI
> make build
```