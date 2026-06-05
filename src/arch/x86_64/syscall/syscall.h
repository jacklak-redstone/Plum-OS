#pragma once
#include "Drivers/Keyboard.hpp"
#include "std/math_types.hpp"
#include "std/types.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"

inline u64 sys_write(const char* str, u64 color) {
    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(0ULL), "D"(str), "S"(color)
        : "rcx", "r11", "memory");

    return ret;
}

inline u64 sys_put_char(char c, u64 color) {
    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(1ULL), "D"(static_cast<u64>(c)), "S"(color)
        : "rcx", "r11", "memory");

    return ret;
}

inline u64 sys_serial_write(const char* c) {
    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(2ULL), "D"(c)
        : "rcx", "r11", "memory");

    return ret;
}

inline u64 sys_serial_put_char(char c) {
    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(3ULL), "D"(static_cast<u64>(c))
        : "rcx", "r11", "memory");

    return ret;
}

inline kb::key_code sys_get_key() {
    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(4ULL)
        : "rcx", "r11", "memory");

    return static_cast<kb::key_code>(ret);
}

inline void sys_sleep(u64 milliseconds) {
    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(6ULL), "D"(milliseconds)
        : "rcx", "r11", "memory");
}

inline void sys_PCI_TEST() {
    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(7ULL)
        : "rcx", "r11", "memory");
}

inline void sys_heap_dump(bool show_all) {
    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(8ULL), "D"(static_cast<u64>(show_all))
        : "rcx", "r11", "memory");
}

inline void sys_swap_framebuffer() {
    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(9ULL)
        : "rcx", "r11", "memory");
}

inline void sys_list_parts() {
    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(10ULL)
        : "rcx", "r11", "memory");
}

inline void sys_openPL(OpenPL::Context *ctx, uint32_t Operation) {
    u64 ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(21ULL),
        "D"(ctx),
        "S"(Operation)
        : "rcx", "r11", "memory"
    );
}