#include "IDT.hpp"
#include "std/types.hpp"
#include "arch/x86_64/Common/Common.hpp"
#include "kernel/log.h"
#include "kernel/Sleep.hpp"
#include "kernel/system.hpp"
#include "APIC.hpp"

namespace IDT {
    const char* get_exception_name(const uint64_t int_no) {
        switch(int_no) {
            case 0: return "Divide Error";
            case 1: return "Debug";
            case 2: return "Non Maskable Interrupt";
            case 3: return "Breakpoint";
            case 4: return "Overflow";
            case 5: return "Bound Range Exceeded";
            case 6: return "Invalid Opcode";
            case 7: return "Device Not Available";
            case 8: return "Double Fault";
            case 10: return "Invalid TSS";
            case 11: return "Segment Not Present";
            case 12: return "Stack-Segment Fault";
            case 13: return "General Protection";
            case 14: return "Page Fault";
            case 16: return "x87 FPU Floating-Point";
            case 17: return "Alignment Check";
            case 18: return "Machine Check";
            case 19: return "SIMD Floating-Point";
            case 20: return "Virtualization Exception";
            case 21: return "Control Protection Exception (yes its acronym is #CP)";
            case 28: return "Hypervisor Injection Exception";
            case 29: return "VMM Communication Exception";
            case 30: return "Security Exception";
            default: return "Unknown";
        }
    }

    static bool extended = false;

    isr_t custom_handlers[256][4] = {{nullptr}};
    uint8_t custom_handlers_count[256] = {0};

    void Install_handler(const isr_t handler, const uint8_t irq_no) {
        if (!handler) {
            log::error("[ IDT ] Install_handler &cERROR&f: &anull handler");
            return;
        }

        //if (irq_no >= 24) { // PIC IRQ 0–15
        //    log::error("[ IDT ] Install_handler &cERROR&f: &cinvalid &firq &e%u", irq_no);
        //    return;
        //}

        const uint8_t vector = irq_no + 32;

        if (custom_handlers_count[vector] >= 4) {
            log::error("[ IDT ] Install_handler &cERROR&f: Max handlers for IRQ: %u", irq_no);
            return;
        }

        if (custom_handlers_count[vector] == 0) {
            log::info("[ IDT ] Installed &afirst &7handler for IRQ &a%u", irq_no);
            const auto iso = apic::IOAPIC::resolve_irq(irq_no);
            systemPL::ioapic.route(iso.gsi, vector, apic::IOAPIC::Fixed,
                                   iso.level_triggered ? apic::IOAPIC::TriggerMode::LEVEL : apic::IOAPIC::TriggerMode::EDGE,
                                   iso.active_low, false);
        } else {
            log::success("[ IDT ] &aAdded shared &7handler for IRQ &a%u", irq_no);
        }

        custom_handlers[vector][custom_handlers_count[vector]] = handler;
        custom_handlers_count[vector]++;
    }

    // NOTE do not add [[noreturn]] to this function
    extern "C" void isr_common(const ISR_Registers* regs) {
        if (regs->int_no <= 31) {
            log::error("%s &c%x\n&4Caused by RIP: &e%x", get_exception_name(regs->int_no), regs->error_code, regs->rip);
            u64 cr2;
            asm volatile("mov %%cr2, %0" : "=r"(cr2));
            log::error("CR2: %x", cr2);
            systemPL::fb.swap();
            asm volatile("cli; hlt");
        }

        for (int i = 0; i < custom_handlers_count[regs->int_no]; i++) {
            custom_handlers[regs->int_no][i](regs);
        }

        if (uacpi_handlers[regs->int_no] != nullptr) {
            uacpi_handlers[regs->int_no](uacpi_handlers_ctx[regs->int_no]);
        }

        // Handlers here
        if (regs->int_no == 32) { // Timer
            Time::tick++;
        }

        if (regs->int_no >= 32) {
            apic::write_apic(0xB0, 0);
            if (PIC_enabled)
                x64::pic_send_eoi(regs->int_no - 32);
        }
    }
}
