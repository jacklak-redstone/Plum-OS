#include <efi.h>
#include <efilib.h>

#include "Loader.hpp"
#include "boot.hpp"

EFI_SYSTEM_TABLE     *ST = nullptr;
EFI_BOOT_SERVICES    *BS = nullptr;
EFI_RUNTIME_SERVICES *RT = nullptr;

EFI_GUID gEfiFileInfoGuid         = EFI_FILE_INFO_ID;
EFI_GUID gEfiLoadedImageGuid      = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID gEfiSimpleFileSystemGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiGraphicsOutputGuid   = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    ST = SystemTable;
    BS = SystemTable->BootServices;
    RT = SystemTable->RuntimeServices;

    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = nullptr;
    BS->LocateProtocol(&gEfiGraphicsOutputGuid, nullptr, (void**)&gop);
    if (!gop) return EFI_UNSUPPORTED;

    fill_screen(gop, 0x0000FF00); // Bootloader Alive

    EFI_FILE* kernel_file = open_kernel_file(ImageHandle, (CHAR16*)L"\\Kernel.elf");
    UINT64    entry_point = load_elf_kernel(kernel_file, gop);

    fill_screen(gop, 0x0000FFFF); // Kernel Loaded

    // Fill FrameBuffer info
    fb.base                = reinterpret_cast<void *>(gop->Mode->FrameBufferBase);
    fb.size                = gop->Mode->FrameBufferSize;
    fb.width               = gop->Mode->Info->HorizontalResolution;
    fb.height              = gop->Mode->Info->VerticalResolution;
    fb.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;

    // Memory map + exit boot services
    UINTN map_size = 0, map_key, desc_size;
    UINT32 desc_version;
    EFI_MEMORY_DESCRIPTOR* map = nullptr;

    BS->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_version);
    map_size += 2 * desc_size;
    BS->AllocatePool(EfiLoaderData, map_size, reinterpret_cast<void **>(&map));
    BS->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_version);

    UINT8* kernel_stack;
    BS->AllocatePool(EfiLoaderData, 64 * 1024, reinterpret_cast<void **>(&kernel_stack));
    auto stack_top = reinterpret_cast<UINT64>(kernel_stack + 64 * 1024);

    EFI_STATUS status = BS->ExitBootServices(ImageHandle, map_key);
    if (EFI_ERROR(status)) {
        map_size += 2 * desc_size;
        BS->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_version);
        // Exit Boot service
        BS->ExitBootServices(ImageHandle, map_key);
    }

    asm volatile(
        "mov %0, %%rsp\n"   // switch stack
        "and $-16, %%rsp\n" // align to 16 B
        "xor %%rbp, %%rbp\n"// clear frame pointer
        "mov %1, %%rdi\n"   // first arg = &g_fb
        "jmp *%2\n"         // jump to kernel entry
        :
        : "r"(stack_top), "r"(reinterpret_cast<UINT64>(&fb)), "r"(entry_point)
        : "rdi", "rbp", "memory"
    );

    while (true) __asm__("hlt");
    return EFI_SUCCESS;
}