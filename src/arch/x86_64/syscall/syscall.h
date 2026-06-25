#pragma once
#include "Drivers/Keyboard.hpp"
#include "std/types.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"

inline u64 syscall(u64 id, u64 a1 = 0, u64 a2 = 0, u64 a3 = 0) {
    u64 ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(id), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory"
    );
    return ret;
}

inline u64 sys_write(const char* str, u64 color) {
    return syscall(0, reinterpret_cast<u64>(str), color);
}

inline u64 sys_put_char(char c, u64 color) {
    return syscall(1, (u64)static_cast<u8>(c), color);
}

inline u64 sys_serial_write(const char* c) {
    return syscall(2, reinterpret_cast<u64>(c));
}

inline u64 sys_serial_put_char(char c) {
    return syscall(3, (u64)static_cast<u8>(c));
}

inline kb::key_code sys_get_key(bool wait = true) {
    return static_cast<kb::key_code>(syscall(4, (u64)wait));
}

inline void sys_exit() {
    syscall(5);
}

inline void sys_sleep(u64 milliseconds) {
    syscall(6, milliseconds);
}

inline void sys_pci_test() {
    syscall(7);
}

inline void sys_heap_dump(bool show_all) {
    syscall(8, show_all);
}

inline void sys_swap_framebuffer() {
    syscall(9);
}

inline void sys_list_parts() {
    syscall(10);
}

inline void sys_openPL(OpenPL::Context *ctx, uint32_t Operation) {
    syscall(21, reinterpret_cast<u64>(ctx), Operation);
}