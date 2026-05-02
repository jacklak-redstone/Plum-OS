#pragma once
#include "../../libs/std/types.hpp"

extern u64 hhdm_offset;

inline u64 to_physical(const u64 virtual_address) {
    return virtual_address - hhdm_offset;
}

inline u64 to_virtual(const u64 physical_address) {
    return physical_address + hhdm_offset;
}

inline u64 to_physical(const void* virtual_address) {
    return reinterpret_cast<u64>(virtual_address) - hhdm_offset;
}

inline u64 to_virtual(const void* physical_address) {
    return reinterpret_cast<u64>(physical_address) + hhdm_offset;
}
