#include "kernel_entry.h"

#include "kernel/linker_info.hpp"
#include "kernel/system.hpp"
#include "libs/limine.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3)

__attribute__((used, section(".limine_requests")))
static volatile limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

u64 hhdm_offset = 0;
u64 kernel_address_phys = 0;
u64 kernel_address_vert = 0;

extern "C" void setup();

extern "C" void kernel_main() {
    if (LIMINE_BASE_REVISION_SUPPORTED == false)
        for(;;) __asm__("hlt");

    if (fb_request.response == nullptr || fb_request.response->framebuffer_count < 1)
        for(;;) __asm__("hlt");

    asm volatile("mov %0, %%rsp\n"
        :
        : "r"(&Linker::stack_top)
        : "memory");

    setup();

    auto* fb = fb_request.response->framebuffers[0];

    framebuffer::framebuffer_info fb_info {
        .base = fb->address,
        .width = static_cast<u32>(fb->width),
        .height = static_cast<u32>(fb->height),
        .pixels_in_scanline = static_cast<uint32_t>(fb->pitch / 4),
        .size = fb->pitch * fb->height
    };

    auto* memmap = memmap_request.response;
    uint64_t heap_addr = 0;
    uint64_t heap_size = 8 * 1024 * 1024; // 8MB

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        auto* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= heap_size) {
            heap_addr = entry->base + hhdm_request.response->offset;
            break;
        }
    }

    if (!heap_addr)
        for(;;) __asm__("hlt");


    hhdm_offset = hhdm_request.response->offset;
    kernel_address_phys = kernel_address_request.response->physical_base;
    kernel_address_vert = kernel_address_request.response->virtual_base;

    systemPL::Init(fb_info, heap_addr);
}
