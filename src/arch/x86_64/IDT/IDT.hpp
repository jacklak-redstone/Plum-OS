#pragma once
#include "uacpi/types.h"

namespace IDT {
    struct IDTR {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed));

    struct IDTEntry {
        uint16_t offset_low;  // Offset bits 0-15
        uint16_t selector;    // Code segment selector (e.g., 0x08 or 0x18)
        uint8_t  ist;         // IST index in TSS
        uint8_t  type_attr;   // Type and attributes (0x8E for Interrupt Gate)
        uint16_t offset_mid;  // Offset bits 16-31
        uint32_t offset_high; // Offset bits 32-63 (Must be 32 bits!)
        uint32_t zero;        // Reserved
    } __attribute__((packed));

    struct ISR_Registers {
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rdi, rsi, rbp, rbx, rdx, rcx, rax;
        uint64_t int_no, error_code;
        uint64_t rip, cs, rflags, rsp, ss;
    } __attribute__((packed));


    extern IDTEntry idt[256];

    // Handlers
    extern "C" void* isr_table[256];

    // Custom Handlers
    using isr_t = void(*)(const ISR_Registers*);
    extern isr_t custom_handlers[256][4];
    extern uint8_t custom_handlers_count[256];

    // uACPI Handlers
    extern uacpi_interrupt_handler uacpi_handlers[256];
    extern uacpi_handle uacpi_handlers_ctx[256];

    void set_IDT_entry(IDTEntry& entry, void* handler);
    void IDT_Install();
    void Install_handler(isr_t handler, uint8_t irq_no);
    void install_uacpi_handler(uacpi_interrupt_handler handler, uint8_t irq_no, uacpi_handle ctx);
    void PIC_Remap(uint8_t offset1, uint8_t offset2);

    extern bool PIC_enabled;

    constexpr uint64_t TIMER_IRQ = 32;
}