#pragma once
#include "std/types.hpp"

namespace apic {
    extern volatile uint32_t* apic;
    constexpr uint32_t SVR = 0xF0;

    uint32_t read_apic(uint32_t reg);
    void write_apic(uint32_t reg, uint32_t value);
    bool aPIC_Init();

    constexpr u32 REG_SELECT  = 0x0;
    constexpr u32 IOWIN = 0x10;

    constexpr uint32_t ID_REGISTER = 0x00;
    constexpr uint32_t VERSION_REGISTER = 0x01;
    constexpr uint32_t REDIRECTION_TABLE_REGISTER = 0x10;

    struct iso_info {
        u32 gsi;
        bool active_low;
        bool level_triggered;
    };

    union redirection_entry {
        struct {
            u64 vector : 8;
            u64 delivery_mode : 3;
            u64 destination_mode : 1;
            u64 delivery_status : 1;
            u64 pin_polarity : 1;
            u64 remote_irr : 1;
            u64 trigger_mode : 1;
            u64 mask : 1;
            u64 reserved : 39;
            u64 dest : 8;
        }  __attribute__((__packed__));

        struct
        {
            u32 low;
            u32 hi;
        };

        u64 raw;
    };

    union id_register {
        struct {
            u32 reserved0 : 24;
            u32 id : 4;
            u32 reserved1 : 4;
        } __attribute__((__packed__));

        u32 raw;
    };

    union version_register {
        struct {
            u32 version : 8;
            u32 reserved0 : 8;
            u32 max_redir_entry : 8;
            u32 reserved1 : 8;
        }__attribute__((__packed__));

        u32 raw;
    };

    class IOAPIC {
    public:
        enum TriggerMode {
            EDGE = 0,
            LEVEL = 1,
        };

        enum DeliveryMode {
            Fixed = 0,
            LowPriority = 1,
            SMI = 2,
            NMI = 3,
            INIT = 4,
            ExtINT = 5
        };

        enum DestinationMode {
            PHYSICAL = 0,
            LOGICAL = 1
        };

        IOAPIC() = default;
        ~IOAPIC() = default;

        bool init();

        void route(u8 gsi, u8 vector, DeliveryMode delivery_mode, TriggerMode trigger_mode, bool active_low, bool masked) const;
        [[nodiscard]] static iso_info resolve_irq(u8 irq);
        void mask(u8 gsi) const;
        void unmask(u8 gsi) const;
    private:
        /*
         * This field contains the physical-base address for the IOAPIC
         * can be found using an IOAPIC-entry in the ACPI 2.0 MADT.
         */
        u64 phys_regs;

        /*
         * Holds the base address of the registers in virtual memory. This
         * address is non-cacheable (see paging).
         */
        u64 virt_address;

        /*
         * Software has complete control over the apic-id. Also, hardware
         * won't automatically change its apic-id so we could cache it here.
         */
        u8 apic_id;

        /*
         * Hardware-version of the apic, mainly for display purpose. ToDo: specify
         * more purposes.
         */
        u8 apic_version;

        /*
         * Although entries for current IOAPIC is 24, it may change. To retain
         * compatibility make sure you use this.
         */
        u8 redirect_entry_count;

        /*
         * The first IRQ which this IOAPIC handles. This is only found in the
         * IOAPIC entry of the ACPI 2.0 MADT. It isn't found in the IOAPIC
         * registers.
         */
        u8 global_intr_base;

        [[nodiscard]] u64 read64(const u8 offset) const {
            const u64 hi = read(offset + 1);
            const u64 lo = read(offset);
            return (hi << 32) | lo;
        }

        [[nodiscard]] u32 read(const u8 offset) const {
            *reinterpret_cast<u32 volatile*>(virt_address + REG_SELECT) = offset;
            return *reinterpret_cast<u32 volatile*>(virt_address + IOWIN);
        }

        void write(const u8 offset, const u32 data) const {
            *reinterpret_cast<u32 volatile*>(virt_address + REG_SELECT) = offset;
            *reinterpret_cast<u32 volatile*>(virt_address + IOWIN) = data;
        }

        void write64(const u8 offset, const u64 data) const {
            write(offset + 1, static_cast<u32>(data >> 32));
            write(offset, static_cast<u32>(data));
        }
    };
}
