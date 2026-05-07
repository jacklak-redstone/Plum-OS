#include "hpet.h"

#include "arch/x86_64/Common/Common.hpp"
#include "kernel/log.h"
#include "kernel/Paging.hpp"
#include "kernel/system.hpp"
#include "kernel/Memory/mem_helper.h"
#include "uacpi/acpi.h"

namespace hpet {
    volatile u64* hpet_base = nullptr;
    volatile u32 used_vectors = 0;
    timer timers[32];
    volatile bool is_timer_armed[32];
    volatile u64 frequency = 0;
    volatile u64 ticks_per_ms = 0;
    volatile u64 ticks_per_us = 0;

    void timer::init(const u8 num) {
        timer_num = num;
        timer_configuration_register config {};
        config.raw = *read(TIMER_N_CONFIGURATION_REGISTER);

        config.enable_interrupts = true;
        config.interrupt_type = 0;
        config.enable_32b = false;
        config.enable_fsb = false;
        config.enable_periodic = false;
        for (auto i = 0; i < 30; i++) {
            if ((config.supported_interrupt_vectors & (1 << i)) && !(used_vectors & (1 << i))) {
                used_vectors |= (1 << i);
                vec = i;
                config.ioapic_route = vec;

                IDT::Install_handler([](const IDT::ISR_Registers* regs) {
                    const u8 gsi = regs->int_no - 32;
                    for (auto j = 0; j < 32; j++) {
                        if (timers[j].exists && timers[j].vec == gsi) {
                            is_timer_armed[j] = false;
                            break;
                        }
                    }
                }, vec);
                break;
            }
        }

        if (vec == 0xFF) {
            log::warn("[ HPET ] No available GSI for timer %u", timer_num);
            return;
        }

        write(TIMER_N_CONFIGURATION_REGISTER, config.raw);
        exists = true;
    }

    void timer::set(const u64 value) const {
        is_timer_armed[timer_num] = true;
        write(TIMER_N_VALUE_REGISTER, value);
    }

    volatile u64* timer::read(const u64 offset) const {
        return reinterpret_cast<volatile u64*>(reinterpret_cast<u64>(hpet_base) + offset + 0x20 * timer_num);
    }

    void timer::write(const u64 offset, const u64 value) const {
        *reinterpret_cast<volatile u64*>(reinterpret_cast<u64>(hpet_base) + offset + 0x20 * timer_num) = value;
    }

    volatile u64* read(const u64 offset) {
       return reinterpret_cast<volatile u64*>(reinterpret_cast<u64>(hpet_base) + offset);
    }

    void write(const u64 offset, const u64 value) {
        *reinterpret_cast<volatile u64*>(reinterpret_cast<u64>(hpet_base) + offset) = value;
    }

    bool init() {
        log::info("[ HPET ] Initializing HPET...");
        auto table = drivers::acpi::acpi::acpi::find_table(ACPI_HPET_SIGNATURE);
        auto* hpet = static_cast<acpi_hpet*>(table.ptr);
        if (hpet == nullptr) {
            log::warn("[ HPET ] No HPET found.");
            return false;
        }

        log::success("[ HPET ] Found HPET Device!");

        auto physical_address = hpet->address.address;
        auto virtual_address = to_virtual(physical_address);
        Paging::Map_memory_vp(virtual_address, physical_address, 0x1000, Paging::Profile::MMIO);
        hpet_base = reinterpret_cast<volatile u64*>(virtual_address);

        capability_register cap {};
        cap.raw = *read(CAPABILITY_REGISTER_OFFSET);
        configuration_register conf {};
        conf.raw = *read(CONFIGURATION_REGISTER_OFFSET);
        conf.enabled = true;

        frequency = 1000000000000000ULL / cap.counter_clock_period;
        ticks_per_ms = frequency / 1000;
        ticks_per_us = frequency / 1000000;
        log::info("[ HPET ] Frequency set to %l, with %l ticks per ms and %l ticks per microsecond.", frequency, ticks_per_ms, ticks_per_us);

        write(MAIN_COUNTER_REGISTER_OFFSET, 0);
        write(CONFIGURATION_REGISTER_OFFSET, conf.raw);

        auto n = cap.num_timers;
        log::info("[ HPET ] Initializing %u timers...", n + 1);
        for (auto i = 0; i <= n; i++) {
            timers[i].init(i);
        }

        return true;
    }

    void sleep(const u64 ticks) {
        for (u8 i = 0; i < 32; i++) {
            auto& timer = timers[i];
            if (!timer.does_exists() || is_timer_armed[i] == true)
                continue;
            timer.set(*read(MAIN_COUNTER_REGISTER_OFFSET) + ticks);
            while (is_timer_armed[i] == true) { x64::halt(); }
            break;
        }
    }

    void sleep_ms(const u64 ms) {
        sleep(ms * ticks_per_ms);
    }

    void sleep_us(const u64 us) {
        sleep(us * ticks_per_us);
    }
}
