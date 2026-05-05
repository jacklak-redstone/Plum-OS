#include "ps2.h"

#include <asm-generic/errno-base.h>

#include "arch/x86_64/Common/Common.hpp"
#include "arch/x86_64/IDT/IDT.hpp"
#include "Drivers/Keyboard.hpp"
#include "kernel/log.h"
#include "uacpi/acpi.h"
#include "uacpi/resources.h"
#include "uacpi/tables.h"

namespace drivers::ps2 {
    bool extended = false;

    struct irq_context {
        u8 irq = 0;
        bool found = false;
    };

    uacpi_iteration_decision find_irq(void* ctx, uacpi_resource* res) {
        auto* irq_ctx = static_cast<irq_context*>(ctx);
        if (res->type == UACPI_RESOURCE_TYPE_IRQ) {
            irq_ctx->irq = res->irq.irqs[0];
            irq_ctx->found = true;
            return UACPI_ITERATION_DECISION_BREAK;
        }
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    i32 probe(uacpi_namespace_node* node, uacpi_namespace_node_info* info) {
        log::info("[ PS2 ] Starting to probe PS2...");
        x64::set_INT_flag(false);

        uacpi_resources *kb_res;
        auto table = acpi::acpi::acpi::find_table(ACPI_FADT_SIGNATURE);
        auto* fadt = static_cast<acpi_fadt*>(table.ptr);
        if (fadt == nullptr) {
            x64::set_INT_flag(true);
            return -ENODEV;
        }

        const uacpi_status st = uacpi_get_current_resources(node, &kb_res);
        if (uacpi_unlikely_error(st)) {
            log::error("[ PS2 ] Unable to retrieve PS2 keyboard resources: %s", uacpi_status_to_string(st));
            x64::set_INT_flag(true);
            return -ENODEV;
        }

        // check if ps2 controller exists
        if ((fadt->iapc_boot_arch & 0x2) == false) // 8042 flag
        {
            x64::set_INT_flag(true);
            return -ENODEV;
        }

        // disable devices
        log::info("[ PS2 ] Disabling ports...");
        x64::outb(INPUT_PORT, 0xAD);
        wait_for_command_completion();
        x64::outb(INPUT_PORT, 0xA7);
        wait_for_command_completion();

        // flush
        log::info("[ PS2 ] Flushing buffers...");
        while (read_status().output_buffer_status == true)
            x64::inb(OUTPUT_PORT);

        // do config stuf
        log::info("[ PS2 ] Configuring...");
        auto config = read_config();
        config.first_port_interrupt = false;
        config.second_port_interrupt = false;
        config.first_port_translation = false;
        config.first_port_clock = false;
        write_config(config);

        // self test
        log::info("[ PS2 ] Performing self-test...");
        const auto saved_config = read_config();
        x64::outb(INPUT_PORT, 0xAA);
        wait_for_command_completion();
        wait_for_read();
        auto self_test = x64::inb(OUTPUT_PORT);
        if (self_test != 0x55) {
            log::error("[ PS2 ] Failed to perform self test: %x", self_test);
            x64::set_INT_flag(true);
            return -ENODEV;
        }
        write_config(saved_config);

        // check port count (1 or 2 bc like its hardcoded max 2 smh old hardware bad)
        bool dual_channel = false;
        x64::outb(INPUT_PORT, 0xA8);
        wait_for_command_completion();
        config = read_config();
        if (config.second_port_clock == false) {
            dual_channel = true;
            x64::outb(INPUT_PORT, 0xA7);
            wait_for_command_completion();
            config.second_port_interrupt = false;
            config.second_port_clock = false;
            write_config(config);
        }

        // reenable
        log::info("[ PS2 ] Enabling ports...");
        auto final_config = read_config();
        x64::outb(INPUT_PORT, 0xAE);
        wait_for_command_completion();
        final_config.first_port_interrupt = true;
        final_config.first_port_clock = false;
        if (dual_channel) {
            x64::outb(INPUT_PORT, 0xA8);
            wait_for_command_completion();
            final_config.second_port_interrupt = true;
            final_config.second_port_clock = false;
        }
        write_config(final_config);

        // reset
        log::info("[ PS2 ] Resetting devices...");
        reset_device(1);

        send_data_to_device(1, 0xF5); // Disable scanning
        wait_for_read();
        const auto ack = x64::inb(OUTPUT_PORT);
        if (ack != 0xFA)
            log::warn("[ PS2 ] Failed to disable PS2 scanning!");

        send_data_to_device(1, 0xF2); // Identify
        wait_for_read();
        const auto identify_ack = x64::inb(OUTPUT_PORT);
        if (identify_ack != 0xFA)
            log::warn("[ PS2 ] Identify ACK unexpected: %x", identify_ack);

        wait_for_read();
        auto id1 = x64::inb(OUTPUT_PORT);
        wait_for_read();
        auto id2 = x64::inb(OUTPUT_PORT);

        send_data_to_device(1, 0xF4);
        wait_for_read();
        const auto enable_ack = x64::inb(OUTPUT_PORT);
        if (enable_ack != 0xFA)
            log::warn("[ PS2 ] Enable scanning ACK unexpected: %x", enable_ack);

        send_data_to_device(1, 0xF0);
        send_data_to_device(1, 1);
        wait_for_read();
        const auto set_scancode_ack = x64::inb(OUTPUT_PORT);
        if (set_scancode_ack != 0xFA)
            log::warn("[ PS2 ] Set scancode ACK unexpected: %x", set_scancode_ack);

        if (id1 == 0xAB) {
            log::info("[ PS2 ] Keyboard on port 1");
            irq_context ctx;
            uacpi_for_each_resource(kb_res, find_irq, &ctx);

            if (ctx.found) {
                IDT::Install_handler(keyboard_interrupt, ctx.irq);
            } else {
                log::warn("[ PS2 ] No IRQ found in resources, defaulting to IRQ 1");
                IDT::Install_handler(keyboard_interrupt, 1);
            }
        } else if (id1 == 0x00 || id1 == 0x03 || id1 == 0x04) {
            log::info("[ PS2 ] Mouse on port 1");
        } else {
            log::warn("[ PS2 ] Unknown device %x, %x on port 1", id1, id2);
        }
        if (dual_channel) {
            reset_device(2);
        }

        uacpi_free_resources(kb_res);
        x64::set_INT_flag(true);
        x64::inb(DATA_PORT);
        return UACPI_STATUS_OK;
    }

    void init(acpi::acpi& acpi) {
        acpi.register_driver({
            .device_name = "PS2 Keyboard",
            .pnp_ids = ps2k_pnp_ids,
            .device_probe = probe,
        });
    }

    status_register read_status() {
        status_register status {};
        const auto raw = x64::inb(INPUT_PORT);
        mem::memcpy(&status, &raw, sizeof(status));
        return status;
    }

    config_register read_config() {
        x64::outb(INPUT_PORT, 0x20);
        wait_for_read();
        config_register config = {};
        const auto raw = x64::inb(OUTPUT_PORT);
        mem::memcpy(&config, &raw, sizeof(config));
        return config;
    }

    void write_config(config_register config) {
        u8 config_bits = 0;
        mem::memmove(&config_bits, &config, sizeof(config));
        x64::outb(INPUT_PORT, 0x60);
        x64::outb(DATA_PORT, config_bits);
        wait_for_command_completion();
    }

    void wait_for_command_completion() {
        for (int i = 0; i < 1000000; ++i) {
            if (read_status().input_buffer_status == false)
                return;
        }
        log::warn("[ PS2 ] Command timed out! This time out is probably maybe fine?");
    }

    void wait_for_read() {
        for (int i = 0; i < 10000000; ++i) {
            if (read_status().output_buffer_status == true)
                return;
        }
        log::warn("[ PS2 ] Command timed out!");
    }

    void send_data_to_device(const i8 device, const u8 data) {
        if (device == 1) {
            wait_for_command_completion();
            x64::outb(DATA_PORT, data);
            wait_for_command_completion();
        }
        else if (device == 2) {
            x64::outb(INPUT_PORT, 0xD4);
            wait_for_command_completion();
            x64::outb(DATA_PORT, data);
            wait_for_command_completion();
        }
    }

    bool reset_device(i8 device) {
        send_data_to_device(device, 0xFF);
        wait_for_read();
        const auto byte1 = x64::inb(OUTPUT_PORT);
        wait_for_read();
        const auto byte2 = x64::inb(OUTPUT_PORT);
        const auto bat = (byte1 == 0xFA) ? byte2 : byte1;
        if (bat == 0xFC) {
            log::warn("[ PS2 ] Device on port %d failed to reset!", device);
            return false;
        }

        return true;
    }

    void keyboard_interrupt(const IDT::ISR_Registers* regs) {
        const u8 raw = x64::inb(OUTPUT_PORT);

        if (raw == 0xE0) {
            extended = true;
            return;
        }

        const bool release = (raw & 0x80) != 0;
        const u8 sc = raw & 0x7F;
        const u8 code = extended ? sc + 0x59 : sc;

        if (!release)
            kb::buf.push(code);

        extended = false;
    }
}
