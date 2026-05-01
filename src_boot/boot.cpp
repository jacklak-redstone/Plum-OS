#include <efi.h>
#include <efilib.h>

struct Framebuffer {
    void* base;
    uint64_t size;
    uint32_t width;
    uint32_t height;
    uint32_t pixels_per_scanline;
};

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = nullptr;

    EFI_STATUS status = SystemTable->BootServices->LocateProtocol(
        &gopGuid,
        nullptr,
        (void**)&gop
    );

    if (EFI_ERROR(status))
        return status;

    Framebuffer fb;

    fb.base = (void*)gop->Mode->FrameBufferBase;
    fb.size = gop->Mode->FrameBufferSize;
    fb.width = gop->Mode->Info->HorizontalResolution;
    fb.height = gop->Mode->Info->VerticalResolution;
    fb.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;

    uint32_t* screen = (uint32_t*)fb.base;

    for (int y = 0; y < fb.height; y++) {
        for (int x = 0; x < fb.width; x++)
            screen[y * fb.pixels_per_scanline + x] = 0x00FF0000;
    }

    while (true) {}

    return EFI_SUCCESS;
}