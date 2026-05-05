#pragma once
#include "libs/std/types.hpp"

namespace heap {
	struct alignas(16) Block {
		uint64_t size;
		bool free;
		Block* next;
		Block* prev;
	};
	static_assert(sizeof(Block) % 16 == 0);

	struct AlignHeader {
		void* raw;
	};

	extern uint64_t heap_start;
	extern uint64_t heap_end;
	extern Block* heap_head;

	void heap_init(uint64_t size, u64 heap_addr);

	void* malloc(uint64_t size);
	void* malloc_align(uint64_t size, uint64_t align);
	void* malloc_boundry(uint64_t size, uint64_t align, uint64_t boundry);

	void free(void* ptr);
	void free_align(void* ptr);
	void free_boundry(void* ptr);

	uint64_t check_free_heap();
	uint64_t check_used_heap();
	uint64_t check_heap();

	void dump_heap(bool show_all);
}


inline void* operator new(const size_t size) {
	return heap::malloc(size);
}

inline void operator delete(void* ptr) noexcept {
	heap::free(ptr);
}

inline void* operator new(size_t, void* ptr) noexcept {
	return ptr;
}

inline void operator delete(void*, void*) noexcept {}

inline void* operator new[](const size_t size) {
	return heap::malloc(size);
}

inline void operator delete[](void* ptr) noexcept {
	heap::free(ptr);
}