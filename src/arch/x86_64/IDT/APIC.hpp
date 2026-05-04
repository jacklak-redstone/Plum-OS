#pragma once
#include "std/types.hpp"

namespace IDT {
    extern volatile uint32_t* apic;
    constexpr uint32_t SVR = 0xF0;

    uint32_t read_apic(uint32_t reg);

    void write_apic(uint32_t reg, uint32_t value);

    bool aPIC_Init();

    bool IOAPIC_Init();

    bool ioapic_route_irq(uint8_t irq, uint8_t vector, uint8_t dest);

    void smt();
}
