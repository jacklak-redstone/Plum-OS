#pragma once
#include "acpi.h"
#include "arch/x86_64/IDT/IDT.hpp"
#include "uacpi/uacpi.h"
#include "uacpi/utilities.h"

namespace drivers::ps2 {
    constexpr const char* const ps2k_pnp_ids[] = {
        "PNP0303",
        nullptr,
    };

    constexpr i32 INPUT_PORT = 0x64;
    constexpr i32 DATA_PORT = 0x60;
    constexpr i32 OUTPUT_PORT = 0x60;

    extern bool extended;

    struct status_register {
        u8 output_buffer_status : 1; // before reading result from output port, make sure this is 1
        u8 input_buffer_status : 1; // before writing command/data to input port, make sure this is 0
        u8 system_flag : 1; // cleared on reset, set by firmware if it POSTs
        u8 command_data : 1; // 0 = data in input buffer is data, 1 = data in input buffer is a command
        u8 reserved0 : 1;
        u8 reserved1 : 1;
        u8 time_out_error : 1;
        u8 parity_error : 1;
    } __attribute__((__packed__));

    struct config_register {
        u8 first_port_interrupt : 1;
        u8 second_port_interrupt : 1;
        u8 system_flag : 1; // 0 = OS should NOT be running lmfao
        u8 zero0 : 1;
        u8 first_port_clock : 1; // 1 = disabled
        u8 second_port_clock : 1; // 1 = disabled
        u8 first_port_translation : 1;
        u8 zero1 : 1;
    } __attribute__((__packed__));

    static_assert(sizeof(status_register) == 1);
    static_assert(sizeof(config_register) == 1);

    i32 probe(uacpi_namespace_node* node, uacpi_namespace_node_info* info);
    void init(acpi::acpi& acpi);
    status_register read_status();
    config_register read_config();
    void write_config(config_register config);
    void wait_for_command_completion();
    void wait_for_read();
    void send_data_to_device(i8 device, u8 data);
    bool reset_device(i8 device);
    void keyboard_interrupt(const IDT::ISR_Registers* regs);
}
