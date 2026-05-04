#include "APIC.hpp"
#include "std/types.hpp"
#include "arch/x86_64/Common/Common.hpp"
#include "kernel/log.h"
#include "kernel/Paging.hpp"
#include <uacpi/tables.h>
#include <uacpi/acpi.h>

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

        Paging::Map_memory_vp(apic_addr, apic_addr, 0x1000, Paging::Profile::MMIO);

        apic = reinterpret_cast<volatile uint32_t *>(apic_addr);

        constexpr uint32_t APIC_ENABLE = 1 << 8;
        constexpr uint32_t SPURIOUS_VECTOR = 0xFF;
        constexpr uint32_t TIMER_VECTOR = 32;

        write_apic(SVR, APIC_ENABLE | SPURIOUS_VECTOR);

        write_apic(0x3E0, 0x3);
        write_apic(0x320, TIMER_VECTOR);
        write_apic(0x380, 0xFFFFFFFF);

        return true;
    }

    // ── IOAPIC register offsets ───────────────────────────────────
    constexpr uint32_t IOAPIC_REGSEL  = 0x00;
    constexpr uint32_t IOAPIC_IOWIN   = 0x10;

    // IOAPIC registers
    constexpr uint32_t IOAPIC_ID      = 0x00;
    constexpr uint32_t IOAPIC_VER     = 0x01;
    constexpr uint32_t IOAPIC_REDTBL  = 0x10; // + 2*n for entry n

    // ── IOAPIC instance ───────────────────────────────────────────
    struct IOAPIC {
        uint8_t  id;
        uint32_t address;           // physical address
        uint32_t gsi_base;          // global system interrupt base
        uint8_t  max_redirects;     // filled after reading VERSION register

        volatile uint32_t* regs;   // mapped virtual address
    };

    // ── ISO (Interrupt Source Override) ──────────────────────────
    struct ISO {
        uint8_t  bus;
        uint8_t  irq;               // source IRQ
        uint32_t gsi;               // mapped GSI
        uint16_t flags;             // polarity + trigger mode
    };

    constexpr uint8_t MAX_IOAPICS = 8;
    constexpr uint8_t MAX_ISOS    = 32;

    static IOAPIC ioapics[MAX_IOAPICS];
    static ISO    isos[MAX_ISOS];
    static uint8_t ioapic_count = 0;
    static uint8_t iso_count    = 0;

    // ── IOAPIC read/write ─────────────────────────────────────────
    static uint32_t ioapic_read(const IOAPIC& io, uint8_t reg) {
        io.regs[IOAPIC_REGSEL / 4] = reg;
        return io.regs[IOAPIC_IOWIN / 4];
    }

    static void ioapic_write(const IOAPIC& io, uint8_t reg, uint32_t val) {
        io.regs[IOAPIC_REGSEL / 4] = reg;
        io.regs[IOAPIC_IOWIN / 4]  = val;
    }

    // ── Redirection entry ─────────────────────────────────────────
    struct RedirEntry {
        uint8_t  vector;
        uint8_t  delivery_mode; // 0=fixed, 1=lowest, 2=SMI, 4=NMI, 5=INIT, 7=ExtINT
        bool     logical_dest;
        bool     active_low;    // false=active high
        bool     level_trigger; // false=edge
        bool     masked;
        uint8_t  dest;          // APIC ID of destination CPU
    };

    static void ioapic_set_redirect(IOAPIC& io, uint8_t gsi_offset, const RedirEntry& e) {
        uint64_t entry = e.vector;
        entry |= ((uint64_t)e.delivery_mode  & 0x7) << 8;
        entry |= ((uint64_t)e.logical_dest   & 0x1) << 11;
        entry |= ((uint64_t)e.active_low     & 0x1) << 13;
        entry |= ((uint64_t)e.level_trigger  & 0x1) << 15;
        entry |= ((uint64_t)e.masked         & 0x1) << 16;
        entry |= ((uint64_t)e.dest)                 << 56;

        uint8_t reg = IOAPIC_REDTBL + gsi_offset * 2;
        ioapic_write(io, reg,     (uint32_t)(entry & 0xFFFFFFFF));
        ioapic_write(io, reg + 1, (uint32_t)(entry >> 32));
    }

    // ── Find IOAPIC responsible for a GSI ────────────────────────
    static IOAPIC* ioapic_for_gsi(uint32_t gsi) {
        for (uint8_t i = 0; i < ioapic_count; i++) {
            IOAPIC& io = ioapics[i];
            if (gsi >= io.gsi_base && gsi < io.gsi_base + io.max_redirects)
                return &io;
        }
        return nullptr;
    }

    // ── Find ISO for a given IRQ ──────────────────────────────────
    static const ISO* iso_for_irq(uint8_t irq) {
        for (uint8_t i = 0; i < iso_count; i++)
            if (isos[i].irq == irq) return &isos[i];
        return nullptr;
    }

    // ── MADT parser ───────────────────────────────────────────────
    bool IOAPIC_Init() {
        uacpi_table madt_table;
        uacpi_status status = uacpi_table_find_by_signature("APIC", &madt_table);
        if (status != UACPI_STATUS_OK) {
            log::error("MADT not found");
            return false;
        }

        auto* madt = reinterpret_cast<acpi_madt*>(madt_table.ptr);
        uint8_t* entry = reinterpret_cast<uint8_t*>(madt + 1);
        uint8_t* end   = reinterpret_cast<uint8_t*>(madt) + madt->hdr.length;

        while (entry < end) {
            uint8_t type   = entry[0];
            uint8_t length = entry[1];

            switch (type) {
                case 1: { // IOAPIC
                    if (ioapic_count >= MAX_IOAPICS) break;

                    auto* rec = reinterpret_cast<acpi_madt_ioapic*>(entry);
                    IOAPIC& io = ioapics[ioapic_count++];

                    io.id       = rec->id;
                    io.address  = rec->address;
                    io.gsi_base = rec->gsi_base;

                    // Map IOAPIC registers
                    Paging::Map_memory_vp(
                        io.address, io.address,
                        0x1000,
                        Paging::Profile::MMIO
                    );
                    io.regs = reinterpret_cast<volatile uint32_t*>(
                        static_cast<uint64_t>(io.address)
                    );

                    // Read max redirections from VERSION register
                    uint32_t ver    = ioapic_read(io, IOAPIC_VER);
                    io.max_redirects = ((ver >> 16) & 0xFF) + 1;

                    log::info("IOAPIC id=%d addr=0x%x gsi_base=%d max_redirects=%d",
                        io.id, io.address, io.gsi_base, io.max_redirects);
                    break;
                }

                case 2: { // Interrupt Source Override (ISO)
                    if (iso_count >= MAX_ISOS) break;

                    auto* rec = reinterpret_cast<acpi_madt_interrupt_source_override*>(entry);
                    ISO& iso = isos[iso_count++];

                    iso.bus   = rec->bus;
                    iso.irq   = rec->source;
                    iso.gsi   = rec->gsi;
                    iso.flags = rec->flags;

                    log::info("ISO irq=%d -> gsi=%d flags=0x%x",
                        iso.irq, iso.gsi, iso.flags);
                    break;
                }

                default:
                    break;
            }

            entry += length;
        }

        if (ioapic_count == 0) {
            log::error("No IOAPICs found in MADT");
            return false;
        }

        // Mask all redirection entries on all IOAPICs
        for (uint8_t i = 0; i < ioapic_count; i++) {
            IOAPIC& io = ioapics[i];
            for (uint8_t j = 0; j < io.max_redirects; j++) {
                uint8_t reg = IOAPIC_REDTBL + j * 2;
                ioapic_write(io, reg, 1 << 16); // masked
                ioapic_write(io, reg + 1, 0);
            }
        }

        return true;
    }

    // ── Public API — route an IRQ to a vector ────────────────────
    // irq    = legacy IRQ number (0-15) or GSI directly
    // vector = IDT vector to deliver to (32-255)
    // dest   = destination LAPIC ID (usually 0 for BSP)
    bool ioapic_route_irq(uint8_t irq, uint8_t vector, uint8_t dest) {
        uint32_t gsi = irq;
        bool     active_low    = false;
        bool     level_trigger = false;

        // Check if there's an ISO remapping this IRQ
        const ISO* iso = iso_for_irq(irq);
        if (iso) {
            gsi = iso->gsi;
            // Flags bits [1:0] = polarity, [3:2] = trigger mode
            if ((iso->flags & 0x3) == 0x3) active_low    = true;
            if ((iso->flags & 0xC) == 0xC) level_trigger = true;
        }

        IOAPIC* io = ioapic_for_gsi(gsi);
        if (!io) {
            log::error("No IOAPIC for GSI %d", gsi);
            return false;
        }

        RedirEntry e {
            .vector        = vector,
            .delivery_mode = 0,       // fixed
            .logical_dest  = false,
            .active_low    = active_low,
            .level_trigger = level_trigger,
            .masked        = false,
            .dest          = dest,
        };

        ioapic_set_redirect(*io, gsi - io->gsi_base, e);
        return true;
    }

    // ── Mask/unmask a GSI ─────────────────────────────────────────
    void ioapic_mask(uint32_t gsi) {
        IOAPIC* io = ioapic_for_gsi(gsi);
        if (!io) return;
        uint8_t reg = IOAPIC_REDTBL + (gsi - io->gsi_base) * 2;
        uint32_t low = ioapic_read(*io, reg);
        ioapic_write(*io, reg, low | (1 << 16));
    }

    void ioapic_unmask(uint32_t gsi) {
        IOAPIC* io = ioapic_for_gsi(gsi);
        if (!io) return;
        uint8_t reg = IOAPIC_REDTBL + (gsi - io->gsi_base) * 2;
        uint32_t low = ioapic_read(*io, reg);
        ioapic_write(*io, reg, low & ~(1 << 16));
    }
}
