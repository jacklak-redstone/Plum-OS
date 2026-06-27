#include "IDT.hpp"
#include "arch/x86_64/Common/Common.hpp"
#include "APIC.hpp"

namespace IDT {
    bool PIC_enabled = false;

    IDTEntry idt[256] __attribute__((aligned(16)));

    void set_IDT_entry(IDTEntry &entry, void *handler) {
        const auto addr = reinterpret_cast<uint64_t>(handler);
        entry.offset_low = addr & 0xFFFF;
        entry.selector = 0x18; // 0x18 64bit code, 0x08 32bit code
        entry.ist = 0;
        entry.type_attr = 0x8E;
        entry.offset_mid = (addr >> 16) & 0xFFFF;
        entry.offset_high = (addr >> 32) & 0xFFFFFFFF;
        entry.zero = 0;
    }

    void IDT_Install() {
        x64::set_INT_flag(false);
        for (uint32_t i = 0; i < 256; i++) {
            set_IDT_entry(idt[i], isr_table[i]);
        }

        IDTR idtr __attribute__((aligned(16)));
        idtr.limit = sizeof(idt) - 1;
        idtr.base  = reinterpret_cast<uint64_t>(&idt);
        asm volatile("lidt %0" : : "m"(idtr));

        PIC_Remap(0x20, 0x28); // 0x20 Master 0x28 Slave
        PIC_enabled = true;
        apic::aPIC_Init();

        x64::outb(0x21, 0xFF);
        x64::outb(0xA1, 0xFF);
        x64::outb(0x20, 0x20);
        x64::outb(0xA0, 0x20);
        PIC_enabled = false;
        x64::set_INT_flag(true);
    }

    void PIC_Remap(const uint8_t offset1, const uint8_t offset2) {
        // ICW1: Start initialization in cascade mode
        x64::outb_safe(0x20, 0x11);
        x64::outb_safe(0xA0, 0x11);

        // ICW2: Set vector offsets (Master at 32, Slave at 40)
        x64::outb_safe(0x21, offset1);
        x64::outb_safe(0xA1, offset2);

        // ICW3: Tell Master PIC there is a slave PIC at IRQ 2
        x64::outb_safe(0x21, 0x04);
        // Tell Slave PIC its cascade identity (2)
        x64::outb_safe(0xA1, 0x02);

        // ICW4: Set mode to 8086/8088 (MCS-80/85) mode
        x64::outb_safe(0x21, 0x01);
        x64::outb_safe(0xA1, 0x01);

        // Restore masks (0x00 enables all IRQs, 0xFF masks all)
        x64::outb(0x21, 0x00);
        x64::outb(0xA1, 0x00);
    }
}
