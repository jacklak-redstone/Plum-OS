#pragma once
#include "std/types.hpp"

namespace hpet {
    constexpr i32 CAPABILITY_REGISTER_OFFSET = 0x0;
    constexpr i32 CONFIGURATION_REGISTER_OFFSET = 0x10;
    constexpr i32 INTERRUPT_STATUS_REGISTER_OFFSET = 0x20;
    constexpr i32 MAIN_COUNTER_REGISTER_OFFSET = 0xF0;

    constexpr i32 TIMER_N_CONFIGURATION_REGISTER = 0x100;
    constexpr i32 TIMER_N_VALUE_REGISTER = 0x108;

    union capability_register {
        struct {
            u64 revision_id : 8;
            u64 num_timers : 5;
            u64 supports_64b : 1;
            u64 reserved : 1;
            u64 supports_legacy_replacement_mapping : 1;
            u64 vendor_id : 16;
            u64 counter_clock_period : 32;
        };

        u64 raw;
    };

    union configuration_register {
        struct {
            u64 enabled : 1;
            u64 legacy_replacement_mapping : 1;
            u64 reserved : 62;
        };

        u64 raw;
    };

    struct interrupt_status_register {
        u64 interrupt_status : 32;
        u64 reserved : 32;
    };

    union timer_configuration_register {
        struct {
            u64 reserved0 : 1;
            u64 interrupt_type : 1; // 0 = edge, 1 = level
            u64 enable_interrupts : 1;
            u64 enable_periodic : 1;
            u64 supports_periodic : 1;
            u64 supports_64b : 1;
            u64 tn_val_set_cnf : 1;
            u64 reserved1 : 1;
            u64 enable_32b : 1;
            u64 ioapic_route : 5;
            u64 enable_fsb : 1;
            u64 supports_fsb : 1;
            u64 reserved2 : 1;
            u64 supported_interrupt_vectors : 32;
        };

        u64 raw;
    };

    class timer {
    public:
        timer() = default;
        ~timer() = default;

        void init(u8 num);
        [[nodiscard]] bool does_exists() const { return exists; };
        [[nodiscard]] u64 vector() const { return vec; }
        void set(u64 value) const;

    private:
        [[nodiscard]] volatile u64* read(u64 offset) const;
        void write(u64 offset, u64 value) const;

        u64 vec = 0xFF;
        u8 timer_num {};
        bool exists = false;
        bool has_fired = false;
    };

    volatile u64* read(u64 offset);
    void write(u64 offset, u64 value);
    bool init();
    void sleep(u64 ticks);
    void sleep_ms(u64 ms);
    void sleep_us(u64 us);
}
