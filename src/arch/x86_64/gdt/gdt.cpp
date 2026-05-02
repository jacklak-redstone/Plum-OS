#include "gdt.h"

#include "kernel/linker_info.hpp"
#include "std/printf.hpp"

constexpr uint64_t gdt_entry(const uint32_t base, const uint32_t limit, const uint8_t access, const uint8_t flags)
{
    return
        (static_cast<uint64_t>(limit & 0x0000FFFF) << 0)   |
        (static_cast<uint64_t>(base & 0x00FFFFFF) << 16)   |
        (static_cast<uint64_t>(access)              << 40) |
        (static_cast<uint64_t>(limit & 0x000F0000) << 32)  |
        (static_cast<uint64_t>(flags & 0x0F)       << 52)  |
        (static_cast<uint64_t>(base >> 24)          << 56);
}

tss_entry tss = {
    .rsp0 = reinterpret_cast<u64>(&Linker::stack_top),
};

__attribute__((aligned(8)))
uint64_t gdt[] = {
    0,
    gdt_entry(0, 0xFFFFF, ACCESS_PRESENT | ACCESS_RING0 | ACCESS_CODE_SEG | ACCESS_READABLE, FLAG_GRANULARITY | FLAG_32BIT), // 0x08
    gdt_entry(0, 0xFFFFF, ACCESS_PRESENT | ACCESS_RING0 | ACCESS_DATA_SEG | ACCESS_WRITABLE, FLAG_GRANULARITY | FLAG_32BIT), // 0x10
    gdt_entry(0, 0, ACCESS_PRESENT | ACCESS_RING0 | ACCESS_CODE_SEG | ACCESS_READABLE, FLAG_64BIT), // 0x18
    gdt_entry(0, 0, ACCESS_PRESENT | ACCESS_RING0 | ACCESS_DATA_SEG | ACCESS_WRITABLE, FLAG_64BIT), // 0x20
    gdt_entry(0, 0, ACCESS_PRESENT | ACCESS_RING3 | ACCESS_DATA_SEG | ACCESS_WRITABLE, FLAG_64BIT), // 0x28
    gdt_entry(0, 0, ACCESS_PRESENT | ACCESS_RING3 | ACCESS_CODE_SEG | ACCESS_READABLE, FLAG_64BIT), // 0x30
    0, // 0x38, TSS low
    0, // 0x40, TSS high
};

gdtd gdt_descriptor = {
    .limit = sizeof(gdt) - 1,
    .base = reinterpret_cast<uint64_t>(&gdt),
};

void init_tss() {
    auto base = reinterpret_cast<uint64_t>(&tss);
    uint16_t limit = sizeof(tss_entry) - 1;

    gdt[7] =
        static_cast<uint64_t>(limit & 0xFFFF) |
        (static_cast<uint64_t>(base & 0xFFFFFF) << 16) |
        (static_cast<uint64_t>(ACCESS_TSS) << 40) |
        (static_cast<uint64_t>((base >> 24) & 0xFF) << 56);
    gdt[8] = (base >> 32);

    asm volatile("ltr %0" :: "r"(static_cast<uint16_t>(0x38)));
}