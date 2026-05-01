#pragma once
#include <efi.h>

struct Elf64_Ehdr {
    UINT8  e_ident[16];
    UINT16 e_type;
    UINT16 e_machine;
    UINT32 e_version;
    UINT64 e_entry;
    UINT64 e_phoff;
    UINT64 e_shoff;
    UINT32 e_flags;
    UINT16 e_ehsize;
    UINT16 e_phentsize;
    UINT16 e_phnum;
    UINT16 e_shentsize;
    UINT16 e_shnum;
    UINT16 e_shstrndx;
};

struct Elf64_Phdr {
    UINT32 p_type;
    UINT32 p_flags;
    UINT64 p_offset;
    UINT64 p_vaddr;
    UINT64 p_paddr;
    UINT64 p_filesz;
    UINT64 p_memsz;
    UINT64 p_align;
};

struct Framebuffer {
    void*   base;
    UINT64  size;
    UINT32  width;
    UINT32  height;
    UINT32  pixels_per_scanline;
};

static Framebuffer fb;

extern EFI_SYSTEM_TABLE     *ST;
extern EFI_BOOT_SERVICES    *BS;
extern EFI_RUNTIME_SERVICES *RT;

extern EFI_GUID gEfiFileInfoGuid;
extern EFI_GUID gEfiLoadedImageGuid;
extern EFI_GUID gEfiSimpleFileSystemGuid;
extern EFI_GUID gEfiGraphicsOutputGuid;