#include "heap.hpp"
#include "std/printf.hpp"
#include "std/string.h"

namespace heap {
    Block* heap_head;
    uint64_t heap_start;
    uint64_t heap_end;

    void heap_init(const uint64_t size, u64 heap_addr) {
        heap_head = reinterpret_cast<Block*>(heap_addr);
        heap_head->size = size - sizeof(Block);
        heap_head->free = true;
        heap_head->next = nullptr;
        heap_head->prev = nullptr;

        heap_start = reinterpret_cast<uint64_t>(reinterpret_cast<void*>(heap_addr));
        heap_end = heap_start + size;
    }

    // Block allocator
    void* malloc(uint64_t size) {
        size = (size + 15) & ~15ULL;
        for (Block* block = heap_head; block; block = block->next) {
            if (block->free && block->size >= size) {

                if (block->size >= size + sizeof(Block) + 16) { // 32B+16B is minimum split size
                    // Split
                    const uint64_t new_size = block->size - size - sizeof(Block);
                    auto* new_block = reinterpret_cast<Block *>(reinterpret_cast<uint8_t *>(block+1) + size);
                    new_block->size = new_size;
                    new_block->free = true;
                    new_block->next = block->next;
                    new_block->prev = block;
                    if (new_block->next)
                        new_block->next->prev = new_block;
                    block->size = size;
                    block->next = new_block;
                }

                block->free = false;
                return block+1;
            }
        }
        std::kernel::printf("Nullptr in malloc!\n");
        return nullptr;
    }

    void free(void* ptr) {
        if (!ptr) return;
        Block* old_block = static_cast<Block*>(ptr) - 1;
        old_block->free = true;

        while (old_block->next && old_block->next->free) {
            // Merge Right
            const Block* next = old_block->next;
            old_block->size += next->size + sizeof(Block);
            old_block->next = next->next;
            if (old_block->next)
                old_block->next->prev = old_block;
        }

        while (old_block->prev && old_block->prev->free) {
            old_block->prev->size += old_block->size + sizeof(Block);
            old_block->prev->next = old_block->next;
            if (old_block->next)
                old_block->next->prev = old_block->prev;
            old_block = old_block->prev;
        }
    }

    void* malloc_align(const uint64_t size, uint64_t align) {
        if (align == 0) align = 1;

        const uint64_t total = size + align + sizeof(AlignHeader);

        auto* raw = static_cast<uint8_t *>(malloc(total));
        if (!raw) return nullptr;

        const auto base = reinterpret_cast<uintptr_t>(raw + sizeof(AlignHeader));

        const uintptr_t aligned = (base + align - 1) & ~(align - 1);

        auto* header = reinterpret_cast<AlignHeader *>(aligned - sizeof(AlignHeader));
        header->raw = raw;

        return reinterpret_cast<void *>(aligned);
    }

    void free_align(void* ptr) {
        if (!ptr) return;

        const auto* header =
            reinterpret_cast<AlignHeader *>(static_cast<uint8_t *>(ptr) - sizeof(AlignHeader));

        free(header->raw);
    }

    void* malloc_boundry(const uint64_t size, uint64_t align, const uint64_t boundry) {
        if (size > ~static_cast<uint64_t>(0x0) - align - boundry - sizeof(AlignHeader)) {
            std::kernel::printf("&cmalloc_boundry error &e#1\n");
            return nullptr;
        }

        if (boundry && (boundry & (boundry - 1))) {
            std::kernel::printf("&cmalloc_boundry error &e#2\n");
            return nullptr;
        }

        if (align == 0) align = 1;

        const uint64_t total = size + align + (boundry ? boundry : 0) + sizeof(AlignHeader);

        const auto raw = static_cast<uint8_t *>(malloc(total));
        if (!raw) {
            std::kernel::printf("&cmalloc_boundry error &e#3\n");
            return nullptr;
        }

        const auto base = reinterpret_cast<uintptr_t>(raw + sizeof(AlignHeader));

        // align
        uintptr_t aligned = (base + align - 1) & ~(align - 1);

        // boundary check
        if (boundry) {
            const uint64_t start_block = aligned & ~(boundry - 1);
            uint64_t end_block   = (aligned + size - 1) & ~(boundry - 1);

            if (start_block != end_block) {
                aligned = (aligned + boundry) & ~(boundry - 1);

                end_block = (aligned + size - 1) & ~(boundry - 1);
                if ((aligned & ~(boundry - 1)) != end_block) {
                    std::kernel::printf("&cmalloc_boundry error &e#4\n");
                    free(raw);
                    return nullptr;
                }
            }
        }

        if (aligned + size > reinterpret_cast<uintptr_t>(raw) + total) {
            std::kernel::printf("&cmalloc_boundry error &e#5\n");
            free(raw);
            return nullptr;
        }

        auto* header = reinterpret_cast<AlignHeader *>(aligned - sizeof(AlignHeader));
        header->raw = raw;

        return reinterpret_cast<void *>(aligned);
    }

    void free_boundry(void* ptr) {
        if (!ptr) return;

        const auto* header =
            reinterpret_cast<AlignHeader *>(static_cast<uint8_t *>(ptr) - sizeof(AlignHeader));

        free(header->raw);
    }

    uint64_t check_heap() {
        uint64_t free_bytes = 0;
        for (const Block* b = heap_head; b; b = b->next) {
            free_bytes += b->size;
        }
        return free_bytes;
    }

    uint64_t check_free_heap() {
        uint64_t free_bytes = 0;
        for (const Block* b = heap_head; b; b = b->next) {
            if (b->free) free_bytes += b->size;
        }
        return free_bytes;
    }

    uint64_t check_used_heap() {
        uint64_t free_bytes = 0;
        for (const Block* b = heap_head; b; b = b->next) {
            if (!b->free) free_bytes += b->size;
        }
        return free_bytes;
    }

    void dump_heap(const bool show_all) {
        if (show_all) {
            uint32_t b_count = 0;
            int limit = 10000;
            std::kernel::printf("&bHeap Visualization\n");

            for (Block* b = heap_head; b && limit--; b = b->next) {
                b_count += 1;
                std::kernel::printf("&f\tBlock #&a%u &f@ &7%x ", b_count, reinterpret_cast<uint64_t>(b+1)); // +1 to show real data address

                auto size = static_cast<double>(b->size);
                const char *post_fix = std::format_size(size);
                std::kernel::printf("&fsize: &a%f&f%s ", size, post_fix);
                if (b->free)
                    std::kernel::printf("&afree\n");
                else
                    std::kernel::printf("&cused\n");
            }
        }
        uint32_t b_count = 0;
        for (Block* b = heap_head; b; b = b->next) {
            b_count += 1;
        }

        std::kernel::printf("&f\tBlock total: &a%u\n", b_count);
        std::kernel::printf("&fSummary (&cused / &afree / &ball&f): ");

        auto used_s = static_cast<double>(check_used_heap());
        const char *used_p = std::format_size(used_s);
        auto free_s = static_cast<double>(check_free_heap());
        const char *free_p = std::format_size(free_s);
        auto all_s = static_cast<double>(check_heap());
        const char *all_p = std::format_size(all_s);
        std::kernel::printf("&c%f&f%s / &a%f&f%s / &b%f&f%s\n\n", used_s, used_p, free_s, free_p, all_s, all_p);
    }
}
