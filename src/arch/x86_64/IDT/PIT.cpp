#include "PIT.hpp"
#include "arch/x86_64/Common/Common.hpp"
#include "libs/std/types.hpp"
#include "kernel/log.h"
#include "APIC.hpp"
#include "kernel/Sleep.hpp"

namespace PIT {
    uint32_t ticks_per_ms = 0;

    void set_PIT_timer_freq(uint32_t hz) {
        if (hz == 0) {
            log::error("[ PIT ] Timer cant be 0hz. Defaulting to 100hz");
            hz = 100;
        }
        const uint32_t divisor = 1193182 / hz;

        x64::outb(0x43, 0x36);

        x64::outb(0x40, static_cast<uint8_t>(divisor & 0xFF)); // Low byte
        x64::outb(0x40, static_cast<uint8_t>((divisor >> 8) & 0xFF)); // High byte
    }

    void set_aPIC_timer_freq(uint32_t hz) {
        if (hz == 0) {
            log::error("[ aPIC ] Timer cant be 0hz. Defaulting to 100hz");
            hz = 100;
        }
        if (ticks_per_ms == 0) {
            log::error("[ aPIC ] 0 ticks per ms detected!!");
            ticks_per_ms = 1;
        }
        uint32_t ticks_per_sec = ticks_per_ms * 1000;
        if (hz > ticks_per_sec) {
            log::error("[ aPIC ] Timer is not fast enough for %uhz (tick per ms: %ut)", hz, ticks_per_ms);
            hz = ticks_per_ms;
        }

        IDT::write_apic(0x380, ticks_per_sec / hz);
        log::success("[ aPIC ] Setting timer to %uhz (initial count: %u)", hz, ticks_per_sec / hz);
    }

    void calibrate_aPIC_timer() {
        constexpr int scale = 1;
        set_PIT_timer_freq(100*scale);

        IDT::write_apic(0x3E0, 0b1011); // Divide by 1

        IDT::write_apic(0x320, 1 << 16); // Mask timer (no interrupts)
        IDT::write_apic(0x380, 0xFFFFFFFF);

        constexpr uint32_t target = 5 * scale; // ~50 ms
        Time::tick = 0;
        while (Time::tick < target) {}

        uint32_t lapic_elapsed = 0xFFFFFFFF - IDT::read_apic(0x390);
        uint32_t ms = target * 10;
        ticks_per_ms = lapic_elapsed / ms;

        if (ticks_per_ms == 0) {
            log::error("[ aPIC ] 0 ticks per ms detected!!");
            ticks_per_ms = 1;
        }

        uint32_t initial = (ticks_per_ms * 1000) / 100;

        IDT::write_apic(0x320, 32 | (1 << 17)); // periodic
        IDT::write_apic(0x380, initial);
    }
}
