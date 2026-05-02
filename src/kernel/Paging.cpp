#include "Paging.hpp"

#include "Memory/heap.hpp"
#include "../libs/std/types.hpp"

extern u64 hddm_offset;
extern u64 kernel_address_vert;
extern u64 kernel_address_phys;

namespace Paging {
    alignas(4096) uint64_t PML4[512];

    void Init() {
        for (uint64_t& i : PML4)
            i = 0;
    }

    uint64_t alloc_page() {
        const auto virt = reinterpret_cast<uint64_t>(heap::malloc(4096 + 4095));

        const uint64_t aligned = (virt + 4095) & ~4095ULL;

        // Zero the page — a non-zeroed table has stray Present bits that cause
        // spurious faults during the page walk.
        auto *page = reinterpret_cast<uint64_t *>(aligned);
        for (int i = 0; i < 512; i++) page[i] = 0;

        return aligned - hddm_offset; // page table entries need physical addresses
    }

    // TODO
    // fix being able to map only 0-136MB
    void Map_memory(uint64_t start, uint64_t end, const uint64_t flags) {
        start = start & ~4095ULL;
        end   = (end + 4095) & ~4095ULL;

        for (uint64_t addr = start; addr < end; addr += 4096) {
            const uint64_t pml4_i = (addr >> 39) & 0x1FF;
            const uint64_t pdpt_i = (addr >> 30) & 0x1FF;
            const uint64_t pd_i   = (addr >> 21) & 0x1FF;
            const uint64_t pt_i   = (addr >> 12) & 0x1FF;

            if (!(PML4[pml4_i] & Present)) {
                uint64_t phys = alloc_page(); // returns physical addr, already zeroed
                PML4[pml4_i] = phys | Present | Writable | User;
            }
            auto *PDPT = reinterpret_cast<uint64_t *>((PML4[pml4_i] & ~0xFFFULL) + hddm_offset);

            if (!(PDPT[pdpt_i] & Present)) {
                uint64_t phys = alloc_page();
                PDPT[pdpt_i] = phys | Present | Writable | User;
            }
            auto* PD = reinterpret_cast<uint64_t *>((PDPT[pdpt_i] & ~0xFFFULL) + hddm_offset);

            if (!(PD[pd_i] & Present)) {
                uint64_t phys = alloc_page();
                PD[pd_i] = phys | Present | Writable | User;
            }
            auto *PT = reinterpret_cast<uint64_t *>((PD[pd_i] & ~0xFFFULL) + hddm_offset);

            PT[pt_i] = addr | flags; // caller is responsible for all flags
        }
    }

    void Map_memory_vp(uint64_t virt, uint64_t phys, uint64_t size, uint64_t flags) {
        for (uint64_t offset = 0; offset < size; offset += 4096) {
            uint64_t v = virt + offset;
            uint64_t p = phys + offset;

            const uint64_t pml4_i = (v >> 39) & 0x1FF;
            const uint64_t pdpt_i = (v >> 30) & 0x1FF;
            const uint64_t pd_i   = (v >> 21) & 0x1FF;
            const uint64_t pt_i   = (v >> 12) & 0x1FF;

            if (!(PML4[pml4_i] & Present)) {
                uint64_t phys = alloc_page(); // returns physical addr, already zeroed
                PML4[pml4_i] = phys | Present | Writable | User;
            }
            auto *PDPT = reinterpret_cast<uint64_t *>((PML4[pml4_i] & ~0xFFFULL) + hddm_offset);

            if (!(PDPT[pdpt_i] & Present)) {
                uint64_t phys = alloc_page();
                PDPT[pdpt_i] = phys | Present | Writable | User;
            }
            auto *PD = reinterpret_cast<uint64_t *>((PDPT[pdpt_i] & ~0xFFFULL) + hddm_offset);

            if (!(PD[pd_i] & Present)) {
                uint64_t phys = alloc_page();
                PD[pd_i] = phys | Present | Writable | User;
            }
            auto *PT = reinterpret_cast<uint64_t *>((PD[pd_i] & ~0xFFFULL) + hddm_offset);

            PT[pt_i] = p | flags; // caller is responsible for all flags
        }
    }

    void Enable_paging() {
        auto pml4_phys = reinterpret_cast<uint64_t>(PML4) - kernel_address_vert + kernel_address_phys;
        asm volatile("mov %0, %%cr3" :: "r"(pml4_phys));
    }
}
