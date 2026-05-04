cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

project(Plum-OS C CXX)

enable_language(ASM_NASM)

# =========================================================
# Kernel

file(GLOB_RECURSE CPP_SOURCES_KERNEL CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src_boot/*.cpp")
file(GLOB_RECURSE CPP_SOURCES_64 CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE ASM_SOURCES_64 CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.asm")
file(GLOB_RECURSE ASM_SOURCES_BOOT CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src_boot/*.asm")

include(vendor/uacpi/uacpi.cmake)

add_executable(Kernel ${CPP_SOURCES_KERNEL} ${CPP_SOURCES_64} ${ASM_SOURCES_64} ${ASM_SOURCES_BOOT} ${UACPI_SOURCES})

target_include_directories(Kernel PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src_boot"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/libs"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers/acpi"
        ${UACPI_INCLUDES}
)

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
        # ---------------------
        # uACPI
        -DUACPI_OVERRIDE_CONFIG
)

target_link_options(Kernel PRIVATE
        -m64
        -nostdlib
        -no-pie
        -fuse-ld=lld
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
        COMMAND mmd    -i ${FAT_IMG} ::/boot
        COMMAND mmd    -i ${FAT_IMG} ::/boot/limine
        COMMAND mcopy  -i ${FAT_IMG} ${CMAKE_SOURCE_DIR}/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
        COMMAND mcopy  -i ${FAT_IMG} ${CMAKE_CURRENT_BINARY_DIR}/Kernel.elf  ::/Kernel.elf
        COMMAND mcopy  -i ${FAT_IMG} ${CMAKE_SOURCE_DIR}/limine.conf  ::/boot/limine/limine.conf
        DEPENDS Kernel
        VERBATIM
)
# ---------------------------------------------------------


# =========================================================
# QEMU

set(QEMU_FLAGS
        # ----- PC -----
        -cpu max
        -m 1G
        -M q35
        -vga std
        -boot order=c
        # ----- Drives -----
        -drive if=pflash,format=raw,readonly=on,file=${OVMF}
        -drive if=none,id=fat,format=raw,file=${FAT_IMG}
        -device virtio-blk-pci,drive=fat
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
        -no-reboot
        -d int,cpu_reset
        -D ${CMAKE_SOURCE_DIR}/qemu.log

        #-d trace:usb_xhci_*
        #-d usb,guest_errors,int
        #-d int # Debug
        #-trace usb_xhci_*
        -s
)

add_custom_target(run
        COMMENT "launching QEMU"
        COMMAND qemu-system-x86_64 ${QEMU_FLAGS}
        DEPENDS Kernel build
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