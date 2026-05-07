#include "APIC.hpp"
#include "std/types.hpp"
#include "arch/x86_64/Common/Common.hpp"
#include "kernel/log.h"
#include "kernel/Paging.hpp"
#include <uacpi/uacpi.h>

#include "acpi.h"
#include "kernel/system.hpp"
#include "kernel/Memory/mem_helper.h"
#include "uacpi/acpi.h"

namespace IDT {
    volatile uint32_t* apic = nullptr;

    void write_apic(const uint32_t reg, const uint32_t value) {
        apic[reg / 4] = value;
    }

    uint32_t read_apic(const uint32_t reg) {
        return apic[reg / 4];
    }

    bool aPIC_Init() {
        const x64::cpuid_result res = x64::cpuid(1);
        if (!(res.edx & (1 << 9))) {
            log::error("aPIC not supported");
            return false;
        }

        uint64_t apic_base = x64::rdmsr(0x1B);
        apic_base |= (1ULL << 11);
        x64::wrmsr(0x1B, apic_base);

        const uint64_t apic_addr = apic_base & 0xFFFFF000;
        u64 apic_virtual_address = to_virtual(apic_addr);
        Paging::Map_memory_vp(apic_virtual_address, apic_addr, 0x1000, Paging::Profile::MMIO);
        apic = reinterpret_cast<volatile uint32_t*>(apic_virtual_address);

        constexpr uint32_t APIC_ENABLE = 1 << 8;
        constexpr uint32_t SPURIOUS_VECTOR = 0xFF;
        constexpr uint32_t TIMER_VECTOR = 32;

        write_apic(SVR, APIC_ENABLE | SPURIOUS_VECTOR);

        write_apic(0x3E0, 0x3);
        write_apic(0x320, TIMER_VECTOR);
        write_apic(0x380, 0xFFFFFFFF);

        return true;
    }

    bool IOAPIC::init() {
        auto table = drivers::acpi::acpi::acpi::find_table(ACPI_MADT_SIGNATURE);
        auto* madt = static_cast<acpi_madt*>(table.ptr);
        if (madt == nullptr)
            return false;

        acpi_madt_ioapic* ioapic_entry = nullptr;
        uacpi_for_each_subtable(&madt->hdr, sizeof(acpi_madt), [](void* ctx, acpi_entry_hdr* entry) -> uacpi_iteration_decision {
            if (entry->type != ACPI_MADT_ENTRY_TYPE_IOAPIC)
                return UACPI_ITERATION_DECISION_CONTINUE;

            *static_cast<acpi_madt_ioapic**>(ctx) = reinterpret_cast<acpi_madt_ioapic*>(entry);
            return UACPI_ITERATION_DECISION_BREAK;
        }, &ioapic_entry);

        if (!ioapic_entry) {
            log::error("[ IOAPIC ] Failed to find IOAPIC entry inside of MADT!");
            return false;
        }

        phys_regs = ioapic_entry->address;
        virt_address = to_virtual(phys_regs);
        global_intr_base = static_cast<u8>(ioapic_entry->gsi_base);
        Paging::Map_memory_vp(virt_address, phys_regs, 0x1000, Paging::Profile::MMIO);

        id_register id_reg { .raw = read(ID_REGISTER) };
        if (id_reg.id != ioapic_entry->id) {
            id_reg.id = ioapic_entry->id;
            write(ID_REGISTER, id_reg.raw);
        }
        apic_id = ioapic_entry->id;

        const version_register ver_reg { .raw = read(VERSION_REGISTER) };
        apic_version = ver_reg.version;
        redirect_entry_count = ver_reg.max_redir_entry + 1;

        uacpi_table_unref(&table);
        return true;
    }

    void IOAPIC::route(const u8 gsi, u8 vector, DeliveryMode delivery_mode, TriggerMode trigger_mode, bool active_low, bool masked) const {
        const u8 index = gsi - global_intr_base;

        redirection_entry entry {};
        entry.vector = vector;
        entry.delivery_mode = delivery_mode;
        entry.trigger_mode = trigger_mode;
        entry.pin_polarity = active_low;
        entry.mask = masked;
        entry.dest = 0; // CPU 0 (LAPIC ID)

        write(REDIRECTION_TABLE_REGISTER + index * 2, entry.low);
        write(REDIRECTION_TABLE_REGISTER + index * 2 + 1, entry.hi);
    }

    iso_info IOAPIC::resolve_irq(const u8 irq) {
        struct find_result {
            u8 irq {};
            iso_info info {};
        } result { .irq = irq, .info = { .gsi = irq, .active_low = false, .level_triggered = false } };

        auto table = drivers::acpi::acpi::acpi::find_table(ACPI_MADT_SIGNATURE);
        auto* madt = static_cast<acpi_madt*>(table.ptr);
        if (madt == nullptr)
            return result.info;

        uacpi_for_each_subtable(&madt->hdr, sizeof(acpi_madt),
                                [](void* ctx, acpi_entry_hdr* entry) -> uacpi_iteration_decision {
                                    if (entry->type != ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE)
                                        return UACPI_ITERATION_DECISION_CONTINUE;

                                    auto* res = static_cast<find_result*>(ctx);
                                    const auto* iso = reinterpret_cast<acpi_madt_interrupt_source_override*>(entry);

                                    if (iso->source != res->irq)
                                        return UACPI_ITERATION_DECISION_CONTINUE;

                                    res->info.gsi = iso->gsi;
                                    res->info.active_low = (iso->flags & ACPI_MADT_POLARITY_MASK) == ACPI_MADT_POLARITY_ACTIVE_LOW;
                                    res->info.level_triggered = (iso->flags & ACPI_MADT_TRIGGERING_MASK) == ACPI_MADT_TRIGGERING_LEVEL;
                                    return UACPI_ITERATION_DECISION_BREAK;
                                }, &result);

        uacpi_table_unref(&table);
        return result.info;
    }

    void IOAPIC::mask(const u8 gsi) const {
        const u8 index = gsi - global_intr_base;
        redirection_entry entry { .raw = read64(REDIRECTION_TABLE_REGISTER + index * 2) };
        entry.mask = 1;
        write64(REDIRECTION_TABLE_REGISTER + index * 2, entry.raw);
    }

    void IOAPIC::unmask(const u8 gsi) const {
        const u8 index = gsi - global_intr_base;
        redirection_entry entry { .raw = read64(REDIRECTION_TABLE_REGISTER + index * 2) };
        entry.mask = 0;
        write64(REDIRECTION_TABLE_REGISTER + index * 2, entry.raw);
    }
}
