cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

project(Plum-OS C CXX)

# =========================================================
# UEFI Boot Loader
set(UEFI_BOOT_FLAGS
        -target x86_64-unknown-windows
        -ffreestanding
        -fshort-wchar
        -mno-red-zone
        -nostdlib
        -fno-exceptions
        -fno-rtti
        -Wl,-entry:efi_main
        -Wl,-subsystem:efi_application
        -fuse-ld=lld-link
)
set(UEFI_BOOT_INCLUDES
        -I/usr/include/efi
        -I/usr/include/efi/protocol
        -I/usr/include/efi/x86_64
        -I/usr/include/efi/ia32
)

file(GLOB_RECURSE CPP_SOURCES_UEFI_BOOT CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/UEFI_BOOT/*.cpp")

add_custom_target(BOOTX64.EFI
        COMMENT "Compiling bootloader -> BOOTX64.EFI"
        COMMAND clang++
        ${UEFI_BOOT_FLAGS}
        ${UEFI_BOOT_INCLUDES}
        ${CPP_SOURCES_UEFI_BOOT}
        -o ${CMAKE_CURRENT_BINARY_DIR}/BOOTX64.EFI
        VERBATIM
)
# ---------------------------------------------------------


# =========================================================
# Dummy
add_library(BOOTX64_ide OBJECT ${CPP_SOURCES_UEFI_BOOT})
target_include_directories(BOOTX64_ide PUBLIC
        /usr/include/efi
        /usr/include/efi/protocol
        /usr/include/efi/x86_64
        /usr/include/efi/ia32
)
target_compile_options(BOOTX64_ide PRIVATE
        -ffreestanding
        -fshort-wchar
        -mno-red-zone
        -fno-exceptions
        -fno-rtti
)
# ---------------------------------------------------------


# =========================================================
# Kernel
file(GLOB_RECURSE CPP_SOURCES_KERNEL CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src_boot/*.cpp")

add_executable(Kernel ${CPP_SOURCES_KERNEL})

target_compile_options(Kernel PRIVATE
        # ---------------------
        # C++/C
        $<$<COMPILE_LANGUAGE:C,CXX>:
        -m64
        -ffreestanding
        -fno-exceptions
        -fno-stack-protector
        -fno-omit-frame-pointer
        -fno-use-cxa-atexit
        -fno-builtin
        -fno-rtti
        -fno-pie
        -fno-pic
        -mcmodel=kernel
        -mno-red-zone
        # Floats
        -msse2
        -mfpmath=sse
        -mno-avx
        >
        # ---------------------
        # NASM
        $<$<COMPILE_LANGUAGE:NASM>:
        -g -F dwarf
        >
        # ---------------------
        # General
        -g
        -Wall
        -O0
)

target_link_options(Kernel PRIVATE
        -m64
        -nostdlib
        -no-pie
        -T ${CMAKE_SOURCE_DIR}/linker.ld
)

set_target_properties(Kernel PROPERTIES
        OUTPUT_NAME "Kernel"
        SUFFIX ".elf"
)
# ---------------------------------------------------------

# All Images
set(FAT_IMG ${CMAKE_SOURCE_DIR}/Images/fat.img)
set(DISK_IMG ${CMAKE_SOURCE_DIR}/Images/disk.img)
set(USB_IMG ${CMAKE_SOURCE_DIR}/Images/usb.img)

set(OVMF ${CMAKE_SOURCE_DIR}/src_boot/ovmf_code_4M.fd)

# =========================================================
# Building

add_custom_target(build
        COMMENT "Building disk image"
        COMMAND dd if=/dev/zero of=${FAT_IMG} bs=1M count=64
        COMMAND mformat -i ${FAT_IMG}
        -h 32 -t 32 -n 64   # heads, tracks, sectors
        -F                  # force FAT32
        ::
        COMMAND mmd    -i ${FAT_IMG} ::/EFI
        COMMAND mmd    -i ${FAT_IMG} ::/EFI/BOOT
        COMMAND mcopy  -i ${FAT_IMG} ${CMAKE_CURRENT_BINARY_DIR}/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
        COMMAND mcopy  -i ${FAT_IMG} ${CMAKE_CURRENT_BINARY_DIR}/Kernel.elf  ::/Kernel.elf
        DEPENDS BOOTX64.EFI Kernel
        VERBATIM
)
# ---------------------------------------------------------


# =========================================================
# QEMU

set(QEMU_FLAGS
        # ----- PC -----
        -cpu max
        -m 512M
        -M q35
        -vga std
        -boot order=c
        # ----- Drives -----
        -drive if=pflash,format=raw,readonly=on,file=${OVMF}
        -drive format=raw,file=${FAT_IMG}
        # ----- AHCI -----
        -device ahci,id=ahci
        -device ide-hd,drive=hd0,bus=ahci.0
        -drive file=${DISK_IMG},format=raw,if=none,id=hd0
        # ----- USB -----
        -device qemu-xhci,id=xhci
        -device usb-storage,drive=usbstick
        -drive if=none,id=usbstick,file=${USB_IMG},format=raw
        -device virtio-keyboard-pci
        # ----- Debug -----
        -serial stdio
        #-d trace:usb_xhci_*
        #-d usb,guest_errors,int
        #-d int # Debug
        #-trace usb_xhci_*
        -s
)

add_custom_target(run
        COMMENT "launching QEMU"
        COMMAND qemu-system-x86_64 ${QEMU_FLAGS}
        DEPENDS BOOTX64.EFI Kernel build
        VERBATIM
)
# ---------------------------------------------------------

#cmake_minimum_required(VERSION 3.28)
#
## Enable NASM compiler
#set(CMAKE_ASM_NASM_COMPILER nasm)
#set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
#
#set(CMAKE_C_STANDARD 17)
#set(CMAKE_CXX_STANDARD 17)
#
#project(kopalnia_os)
#
#enable_language(ASM_NASM)
#
##set(CMAKE_VERBOSE_MAKEFILE ON) # Debug only
#
## All 64bit and 32bit .cpp and .asm files
#file(GLOB_RECURSE CPP_SOURCES_64 CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
#file(GLOB_RECURSE ASM_SOURCES_64 CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.asm")
#file(GLOB_RECURSE ASM_SOURCES_32 CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src_boot/*.asm")
## ISO
#set(ISODIR ${CMAKE_SOURCE_DIR}/isodir)
#
#add_executable(kopalnia_os ${ASM_SOURCES_32} ${CPP_SOURCES_64} ${ASM_SOURCES_64})
#
#target_include_directories(kopalnia_os PUBLIC
#        "${CMAKE_CURRENT_SOURCE_DIR}/src_boot"
#        "${CMAKE_CURRENT_SOURCE_DIR}/src"
#        "${CMAKE_CURRENT_SOURCE_DIR}/src/libs"
#)
#
#target_compile_options(kopalnia_os PRIVATE
#        # ---------------------
#        # C++/C
#        $<$<COMPILE_LANGUAGE:C,CXX>:
#        -m64
#        -ffreestanding
#        -fno-exceptions
#        -fno-stack-protector
#        -fno-omit-frame-pointer
#        -fno-use-cxa-atexit
#        -fno-builtin
#        -fno-rtti
#        -fno-pie
#        -fno-pic
#        -mcmodel=kernel
#        -mno-red-zone
#        # Floats
#        -msse2
#        -mfpmath=sse
#        -mno-avx
#        >
#        # ---------------------
#        # NASM
#        $<$<COMPILE_LANGUAGE:NASM>:
#        -g -F dwarf
#        >
#        # ---------------------
#        # General
#        -g
#        -Wall
#        -O0
#)
#
#set_target_properties(kopalnia_os PROPERTIES
#        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/isodir/boot
#)
#
## Linker
#target_link_options(kopalnia_os PRIVATE
#        -m64
#        -nostdlib
#        -no-pie
#        -T ${CMAKE_SOURCE_DIR}/linker.ld
#)
#
#set(QEMU_FLAGS
#        # PC
#        -cpu max
#        -m 512M
#        -M q35
#        -vga std
#        -boot order=dc
#        # drive
#        -cdrom ${CMAKE_BINARY_DIR}/mykernel.iso
#        #ahci
#        -device ahci,id=ahci
#        -device ide-hd,drive=hd0,bus=ahci.0
#        -drive file=../disk.img,format=raw,if=none,id=hd0
#        # USB
#        -device qemu-xhci,id=xhci
#        -device usb-storage,drive=usbstick
#        -drive if=none,id=usbstick,file=../usb.img,format=raw
#        -device virtio-keyboard-pci
#        # Debug
#        -serial stdio
#        #-d trace:usb_xhci_*
#        #-d usb,guest_errors,int
#        #-d int # Debug
#        #-trace usb_xhci_*
#        -s
#)
#
## Make ISO and run it
#add_custom_target(run
#        COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/mykernel.iso ${ISODIR}
#        COMMAND qemu-system-x86_64 ${QEMU_FLAGS}
#        COMMENT "Building ISO and running QEMU"
#        DEPENDS ${CMAKE_PROJECT_NAME}
#)