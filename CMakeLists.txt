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

cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(Plum-OS C CXX)

add_executable(BOOTX64.EFI
        src_boot/boot.cpp
)

target_include_directories(BOOTX64.EFI PUBLIC
        "/usr/include/efi"
        "/usr/include/efi/protocol"
        "/usr/include/efi/x86_64"
        "/usr/include/efi/ia32"
)

target_compile_options(BOOTX64.EFI PRIVATE
        # ---------------------
        # C++/C
        $<$<COMPILE_LANGUAGE:C,CXX>:
        -target x86_64-unknown-windows
        -ffreestanding
        -fshort-wchar
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

target_link_libraries(BOOTX64.EFI PRIVATE
        /usr/lib/libefi.a
        /usr/lib/libgnuefi.a
)

target_link_options(BOOTX64.EFI PRIVATE
        -target x86_64-unknown-windows
        -nostdlib
        -Wl,-entry:efi_main
        -Wl,-subsystem:efi_application
        -fuse-ld=lld-link
)

add_custom_target(run
        COMMAND dd if=/dev/zero of=../Images/fat.img bs=1M count=64
        COMMAND mformat -i ../Images/fat.img -F -v EFI ::
        COMMAND mmd -i ../Images/fat.img ::/EFI
        COMMAND mmd -i ../Images/fat.img ::/EFI/BOOT
        COMMAND mcopy -i ../Images/fat.img BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
        COMMAND qemu-system-x86_64
        -drive if=pflash,format=raw,readonly=on,file=../src_boot/ovmf_code_4M.fd
        -hda ../Images/fat.img
        -serial stdio
        DEPENDS BOOTX64.EFI
)