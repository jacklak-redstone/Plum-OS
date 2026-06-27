#include "lapic.h"

#include "APIC.hpp"
#include "Drivers/hpet/hpet.h"
#include "kernel/log.h"

namespace lapic {

    #define LAPIC_REG_ID 0x20 // LAPIC ID
    #define LAPIC_REG_EOI 0x0b0 // End of interrupt
    #define LAPIC_REG_SPURIOUS 0x0f0
    #define LAPIC_REG_CMCI 0x2f0 // LVT Corrected machine check interrupt
    #define LAPIC_REG_ICR0 0x300 // Interrupt command register
    #define LAPIC_REG_ICR1 0x310
    #define LAPIC_REG_LVT_TIMER 0x320
    #define LAPIC_REG_TIMER_INITCNT 0x380 // Initial count register
    #define LAPIC_REG_TIMER_CURCNT 0x390 // Current count register
    #define LAPIC_REG_TIMER_DIV 0x3e0
    #define LAPIC_EOI_ACK 0x00

    void calibrate_lapic_timer() {
        apic::write_apic(LAPIC_REG_TIMER_DIV, 0b1011); // Divide by 1

        apic::write_apic(LAPIC_REG_LVT_TIMER, 1 << 16); // Mask timer (no interrupts)
        apic::write_apic(LAPIC_REG_TIMER_INITCNT, 0xFFFFFFFF);

        const u64 start = *hpet::read(hpet::MAIN_COUNTER_REGISTER_OFFSET);
        const u64 target = start + hpet::ticks_per_ms * 50; // 50ms
        while (*hpet::read(hpet::MAIN_COUNTER_REGISTER_OFFSET) < target) {}

        const u32 lapic_elapsed = 0xFFFFFFFF - apic::read_apic(LAPIC_REG_TIMER_CURCNT);
        u32 ticks_per_ms = lapic_elapsed / 50;

        if (ticks_per_ms == 0) {
            log::error("[ aPIC ] 0 ticks per ms detected!!");
            ticks_per_ms = 1;
        }

        const u32 initial = (ticks_per_ms * 1000) / 100;

        apic::write_apic(LAPIC_REG_LVT_TIMER, 32 | (1 << 17)); // periodic
        apic::write_apic(LAPIC_REG_TIMER_INITCNT, initial);
    }

    void lapic_timer_stop() {
        apic::write_apic(LAPIC_REG_TIMER_INITCNT, 0);
        apic::write_apic(LAPIC_REG_LVT_TIMER, 1 << 16);
    }
}
