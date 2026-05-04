#include "PIT.hpp"
#include "arch/x86_64/Common/Common.hpp"
#include "libs/std/types.hpp"
#include "APIC.hpp"
#include "kernel/Sleep.hpp"

namespace PIT {
    uint32_t ticks_per_ms = 0;

    void set_PIT_timer_freq(const uint32_t hz) {
        const uint32_t divisor = 1193182 / hz;

        x64::outb(0x43, 0x36);

        x64::outb(0x40, static_cast<uint8_t>(divisor & 0xFF)); // Low byte
        x64::outb(0x40, static_cast<uint8_t>((divisor >> 8) & 0xFF)); // High byte
    }

    void set_aPIC_timer_freq(uint32_t hz) {
        if (hz == 0) return;
        uint32_t ticks_per_sec = ticks_per_ms * 1000;
        if (hz > ticks_per_sec) return;

        IDT::write_apic(0x380, ticks_per_sec / hz);
    }

    void calibrate_aPIC_timer() {
        constexpr int scale = 1;
        set_PIT_timer_freq(100*scale);

        IDT::write_apic(0x320, 0);
        IDT::write_apic(0x380, 0xFFFFFFFF);

        Time::tick = 0;
        while (Time::tick < 1*scale) {}

        uint32_t lapic_elapsed = 0xFFFFFFFF - IDT::read_apic(0x390);
        ticks_per_ms = lapic_elapsed / 10;
        uint32_t initial = ticks_per_ms * 10;

        IDT::write_apic(0x380, initial);
        IDT::write_apic(0x320, 32 | (1 << 17)); // periodic bit
    }
}
