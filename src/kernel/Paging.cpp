#include "Paging.hpp"

#include "Memory/heap.hpp"
#include "../libs/std/types.hpp"
#include "Memory/mem_helper.h"
#include "std/mem_common.hpp"

extern u64 kernel_address_vert;
extern u64 kernel_address_phys;

namespace Paging {
    alignas(4096) uint64_t PML4[512];

    void Init() {
        for (uint64_t& i : PML4)
            i = 0;
    }

    uint64_t alloc_page() {
        const auto virt = reinterpret_cast<uint64_t>(heap::malloc_align(4096, 4096));
        mem::memset(reinterpret_cast<void*>(virt), 0, 4096);
        return virt - hhdm_offset;
    }

    // TODO
    // fix being able to map only 0-136MB
    void Map_memory(uint64_t start, uint64_t end, const uint64_t flags) {
        start = start & ~(4095);
        end = (end + 4095) & ~(4095);

        for (uint64_t addr = start; addr < end; addr += 4096) {
            const uint64_t pml4_i = (addr >> 39) & 0x1FF;
            const uint64_t pdpt_i = (addr >> 30) & 0x1FF;
            const uint64_t pd_i = (addr >> 21) & 0x1FF;
            const uint64_t pt_i   = (addr >> 12) & 0x1FF;

            if (!(PML4[pml4_i] & Present)) {
                const uint64_t new_table = alloc_page();
                PML4[pml4_i] = new_table | Present | Writable | User;
            }

            auto* PDPT = reinterpret_cast<uint64_t*>(to_virtual(PML4[pml4_i] & ~0xFFFULL));
            if (!(PDPT[pdpt_i] & Present)) {
                const uint64_t new_table = alloc_page();
                PDPT[pdpt_i] = new_table | Present | Writable | User;
            }

            auto *PD = reinterpret_cast<uint64_t*>(to_virtual(PDPT[pdpt_i] & ~0xFFFULL));
            if (!(PD[pd_i] & Present)) {
                const uint64_t new_table = alloc_page();
                PD[pd_i] = new_table | Present | Writable | User;
            }

            auto* PT = reinterpret_cast<uint64_t*>(to_virtual(PD[pd_i] & ~0xFFFULL));
            PT[pt_i] = addr | flags; // Physical == Virtual (identity mapping)
        }
    }

    void Map_memory_vp(uint64_t virt, uint64_t physical, uint64_t size, uint64_t flags) {
        for (uint64_t offset = 0; offset < size; offset += 4096) {
            uint64_t v = virt + offset;
            uint64_t p = physical + offset;

            const uint64_t pml4_i = (v >> 39) & 0x1FF;
            const uint64_t pdpt_i = (v >> 30) & 0x1FF;
            const uint64_t pd_i   = (v >> 21) & 0x1FF;
            const uint64_t pt_i   = (v >> 12) & 0x1FF;

            if (!(PML4[pml4_i] & Present)) {
                uint64_t phys = alloc_page(); // returns physical addr, already zeroed
                PML4[pml4_i] = phys | Present | Writable | User;
            }

            auto* PDPT = reinterpret_cast<uint64_t*>(to_virtual(PML4[pml4_i] & ~0xFFFULL));
            if (!(PDPT[pdpt_i] & Present)) {
                uint64_t phys = alloc_page();
                PDPT[pdpt_i] = phys | Present | Writable | User;
            }

            auto *PD = reinterpret_cast<uint64_t*>(to_virtual(PDPT[pdpt_i] & ~0xFFFULL));
            if (!(PD[pd_i] & Present)) {
                uint64_t phys = alloc_page();
                PD[pd_i] = phys | Present | Writable | User;
            }

            auto* PT = reinterpret_cast<uint64_t*>(to_virtual(PD[pd_i] & ~0xFFFULL));
            PT[pt_i] = p | flags; // caller is responsible for all flags
        }
    }

    void Enable_paging() {
        auto pml4_phys = reinterpret_cast<uint64_t>(PML4) - kernel_address_vert + kernel_address_phys;
        asm volatile("mov %0, %%cr3" :: "r"(pml4_phys));
    }
}
