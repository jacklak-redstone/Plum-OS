#pragma once
#include <efi.h>

void fill_screen(const EFI_GRAPHICS_OUTPUT_PROTOCOL* gop, UINT32 color);

EFI_FILE* open_kernel_file(EFI_HANDLE ImageHandle, CHAR16 *path);

UINT64 load_elf_kernel(EFI_FILE* kernel_file, EFI_GRAPHICS_OUTPUT_PROTOCOL* gop);