#pragma once
#include "libs/std/types.hpp"

namespace x64 {
    inline void outb(uint16_t port, uint8_t val) {
        asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
    }
    inline void outl(uint16_t port, uint32_t val) {
        asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
    }
    inline void outw(uint16_t port, u16 val) {
        asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
    }

    inline void io_wait() {
        outb(0x80, 0);
    }

    inline void outb_safe(const uint16_t port, const uint8_t val) {
        outb(port, val);
        io_wait();
    }

    inline uint8_t inb(uint16_t port) {
        uint8_t ret;
        asm volatile ("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
        return ret;
    }
    inline uint32_t inl(uint16_t port) {
        uint32_t ret;
        asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
        return ret;
    }
    inline u16 inw(uint16_t port) {
        u16 ret;
        asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
        return ret;
    }

    inline void halt() {
        asm volatile("sti; hlt");
    }

    void set_INT_flag(bool flag);
    bool get_INT_flag();
    void pic_send_eoi(uint8_t irq);

    struct cpuid_result {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    };

    inline cpuid_result cpuid(uint32_t leaf, uint32_t subleaf = 0) {
        cpuid_result r;
        uint32_t ebx_temp;

        asm volatile("cpuid"
            : "=a"(r.eax), "=b"(ebx_temp), "=c"(r.ecx), "=d"(r.edx)
            : "a"(leaf), "c"(subleaf)
        );

        r.ebx = ebx_temp;
        return r;
    }

    inline uint64_t rdmsr(uint32_t msr) {
        uint32_t low, high;

        asm volatile("rdmsr"
            : "=a"(low), "=d"(high)
            : "c"(msr)
        );

        return (static_cast<uint64_t>(high) << 32) | low;
    }

    inline void wrmsr(uint32_t msr, uint64_t value) {
        uint32_t low  = static_cast<uint32_t>(value);
        uint32_t high = static_cast<uint32_t>(value >> 32);

        asm volatile("wrmsr"
            :
            : "c"(msr), "a"(low), "d"(high)
        );
    }
}