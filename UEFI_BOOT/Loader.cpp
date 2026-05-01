#include "Loader.hpp"
#include "boot.hpp"
#include <efi.h>

void fill_screen(const EFI_GRAPHICS_OUTPUT_PROTOCOL* gop, const UINT32 color) {
    auto* m_fb = reinterpret_cast<UINT32 *>(gop->Mode->FrameBufferBase);
    const UINT32  pps = gop->Mode->Info->PixelsPerScanLine;
    const UINT32  w   = gop->Mode->Info->HorizontalResolution;
    const UINT32  h   = gop->Mode->Info->VerticalResolution;
    for (UINT32 y = 0; y < h; y++)
        for (UINT32 x = 0; x < w; x++)
            m_fb[y * pps + x] = color;
}

EFI_FILE* open_kernel_file(EFI_HANDLE ImageHandle, CHAR16 *path) {
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    BS->HandleProtocol(ImageHandle, &gEfiLoadedImageGuid, reinterpret_cast<void **>(&loaded_image));

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    BS->HandleProtocol(loaded_image->DeviceHandle, &gEfiSimpleFileSystemGuid, reinterpret_cast<void **>(&fs));

    EFI_FILE* root;
    fs->OpenVolume(fs, &root);

    EFI_FILE* kernel_file;
    const EFI_STATUS status = root->Open(
        root, &kernel_file,
        path,
        EFI_FILE_MODE_READ, 0
    );

    if (EFI_ERROR(status)) return nullptr;
    return kernel_file;
}

UINT64 load_elf_kernel(EFI_FILE* kernel_file, EFI_GRAPHICS_OUTPUT_PROTOCOL* gop) {
    if (!kernel_file) {
        fill_screen(gop, 0x000000FF); // BLUE = kernel file not found
        while (true) __asm__("hlt");
    }

    EFI_FILE_INFO* info;
    UINTN info_size = sizeof(EFI_FILE_INFO) + 256;
    BS->AllocatePool(EfiLoaderData, info_size, (void**)&info);
    kernel_file->GetInfo(kernel_file, &gEfiFileInfoGuid, &info_size, info);

    UINTN  file_size = info->FileSize;
    UINT8* elf_data;
    BS->AllocatePool(EfiLoaderData, file_size, (void**)&elf_data);
    kernel_file->Read(kernel_file, &file_size, elf_data);
    BS->FreePool(info);

    // Validate ELF
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)elf_data;
    if (ehdr->e_ident[0] != 0x7F ||
        ehdr->e_ident[1] != 'E'  ||
        ehdr->e_ident[2] != 'L'  ||
        ehdr->e_ident[3] != 'F'  ||
        ehdr->e_ident[4] != 2) {
        fill_screen(gop, 0x00FFFF00); // YELLOW = bad ELF magic
        while (true) __asm__("hlt");
    }

    // Load PT_LOAD segments
    Elf64_Phdr* phdrs = (Elf64_Phdr*)(elf_data + ehdr->e_phoff);

    for (UINT16 i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr* ph = &phdrs[i];
        if (ph->p_type != 1) continue;

        UINTN pages = (ph->p_memsz + 0xFFF) / 0x1000;
        EFI_PHYSICAL_ADDRESS seg_addr = ph->p_paddr;

        EFI_STATUS s = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &seg_addr);
        if (EFI_ERROR(s)) {
            fill_screen(gop, 0x00FF8000); // ORANGE = AllocatePages failed
            while (true) __asm__("hlt");  // ← this is the most likely failure point
        }

        BS->CopyMem((void*)seg_addr, elf_data + ph->p_offset, ph->p_filesz);

        if (ph->p_memsz > ph->p_filesz)
            BS->SetMem((void*)(seg_addr + ph->p_filesz), ph->p_memsz - ph->p_filesz, 0);
    }

    UINT64 entry = ehdr->e_entry;
    BS->FreePool(elf_data);
    return entry;
}