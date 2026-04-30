#include "ahci_port.h"
#include "ahci.h"
#include "ahci_helper.h"
#include "arch/x86_64/Common/Common.hpp"
#include "kernel/log.h"
#include "kernel/Sleep.hpp"
#include "std/mem_common.hpp"
#include "std/printf.hpp"

namespace drivers::ahci {
    i8 ahci_port::get_command_slot() const {
        const u32 slots = port->sact | port->command_issue;
        for (u8 i = 0; i < num_command_slots; ++i) {
            if ((slots & (1 << i)) == 0)
                return i;
        }

        log::warn("Failed to find free command slot.");
        return -1;
    }

    bool ahci_port::wait_for_port() const {
        for (int i = 0; i < 100000000; ++i) {
            if ((port->tfd & (ATA_DEV_BUSY_BIT | ATA_DEV_DRQ_BIT)) == 0) {
                return true;
            }
        }

        log::error("Port is hung.");
        return false;
    }

    bool ahci_port::wait_for_port_completion(const u8 slot) {
        for (int i = 0; i < 100000000; ++i) {
            if (command_slots[slot].error) {
                log::error("&4Command failed.");
                command_slots[slot].error = false;
                return false;
            }
            if ((port->command_issue & (1 << slot)) == 0)
                return true;
        }
        log::error("Command timed out.");
        command_slots[slot].error = false;
        return false;
    }

    void ahci_port::configure(const port_type type, volatile hba_port* port, u8 port_num, volatile hba_memory* hba) {
        this->type = type;
        this->port_num = port_num;
        this->port = port;
        this->num_command_slots = hba->capabilities.num_command_slots;
        this->bits_is_64 = hba->capabilities.supports_64_bit_addressing;
        active = true;

        stop();

        // Command List
        command_list = static_cast<command_header*>(allocate_virtual_memory(sizeof(command_header) * 32, 1024));
        port->command_list_base = static_cast<u32>(reinterpret_cast<u64>(command_list));
        if (bits_is_64)
            port->command_list_base_upper = static_cast<u32>(reinterpret_cast<u64>(command_list) >> 32);

        // Received FIS
        received = static_cast<received_fis*>(allocate_virtual_memory(sizeof(received_fis), 256));
        port->fis_base_address = static_cast<u32>(reinterpret_cast<u64>(received));
        if (bits_is_64)
            port->fis_base_address_upper = static_cast<u32>(reinterpret_cast<u64>(received) >> 32);

        for (int i = 0; i < 32; ++i) {
            command_list[i].prd_table_length = 0;

            command_slots[i].table = static_cast<command_table*>(allocate_virtual_memory(sizeof(command_table), 128));
            command_slots[i].complete = false;
            command_slots[i].error = false;

            command_list[i].cmd_table_base_address = static_cast<u32>(reinterpret_cast<u64>(command_slots[i].table));
            if (bits_is_64)
                command_list[i].cmd_table_base_address_upper = static_cast<u32>(reinterpret_cast<u64>(command_slots[i].table) >> 32);
        }

        start();

        // Configure which interrupts to enable
        clear_interrupt_errors();
        port->interrupts_enabled.cold_port_detect_interrupt = true;
        port->interrupts_enabled.task_file_error_interrupt = true;
        port->interrupts_enabled.host_bus_fatal_error_interrupt = true;
        port->interrupts_enabled.host_bus_data_error_interrupt = true;
        port->interrupts_enabled.interface_fatal_error_interrupt = true;
        port->interrupts_enabled.overflow_interrupt = true;
        port->interrupts_enabled.incorrect_port_multiplier_interrupt = true;
        port->interrupts_enabled.unknown_fis_interrupt = true;
        port->interrupts_enabled.port_connect_change_interrupt = true;
        port->interrupts_enabled.device_to_host_fis_interrupt = true;
    }

    void ahci_port::start() const {
        while (port->command_status & CMD_CR_BIT) {}
        port->command_status |= CMD_FRE_BIT | CMD_ST_BIT;
    }

    void ahci_port::stop() const {
        port->command_status &= ~CMD_ST_BIT;
        while (port->command_status & CMD_CR_BIT) {}
        port->command_status &= ~CMD_FRE_BIT;
        while (port->command_status & CMD_FR_BIT) {}
    }

    void ahci_port::comreset() const {
        port->sctl = (port->sctl & ~0xF) | 0x1;
        for (volatile int i = 0; i < 100000; i++) {}
        port->sctl = port->sctl & ~0xF;

        for (int i = 0; i < 1000000; i++) {
            if ((port->ssts & 0xF) == 3)
                break;
        }

        port->serr = 0xFFFFFFFF;
    }

    bool ahci_port::identify(const u16* buffer) {
        clear_interrupt_errors();
        const auto slot = get_command_slot();
        if (slot == -1)
            return false;

        auto& header = command_list[slot];
        header.fis_length = sizeof(fis::reg_h2d) / sizeof(uint32_t);
        header.write = 0;
        header.prd_table_length = 1;
        header.prefetchable = true;
        header.clear = true;

        const auto table = command_slots[slot].table;
        mem::memset(table, 0, sizeof(command_table));
        table->prdt[0].data_base_address = static_cast<u32>(reinterpret_cast<u64>(buffer));
        if (bits_is_64)
            table->prdt[0].data_base_address_upper = static_cast<u32>(reinterpret_cast<u64>(buffer) >> 32);
        table->prdt[0].data_byte_count = 512 - 1;
        table->prdt[0].interrupt_on_complete = true;

        const auto command_fis = reinterpret_cast<fis::reg_h2d*>(table->command_fis);
        mem::memset(command_fis, 0, sizeof(fis::reg_h2d));
        command_fis->fis_type = static_cast<u8>(fis::type::FIS_TYPE_REG_H2D);
        command_fis->command_control = 1;
        command_fis->command = ATA_CMD_IDENTIFY;

        return issue_command(slot);
    }

    bool ahci_port::read(const u64 start, u32 count, u16* buffer, const u16 sector_size) {
        clear_interrupt_errors();

        const auto slot = get_command_slot();
        if (slot == -1)
            return false;

        const u32 start_lo = static_cast<u32>(start);
        const u32 start_hi = static_cast<u32>(start >> 32);
        const u32 original_count = count;
        const u32 sectors_per_prdt = (8 * 1024) / sector_size;

        auto& header = command_list[slot];
        header.fis_length = sizeof(fis::reg_h2d) / sizeof(uint32_t);
        header.write = 0;
        header.prd_table_length = static_cast<uint16_t>((count - 1) / sectors_per_prdt) + 1;
        header.prefetchable = true;
        header.clear = true;

        if (header.prd_table_length > MAX_PRDT_SIZE)
            return false;

        const auto table = command_slots[slot].table;
        mem::memset(table, 0, sizeof(command_table));

        // 8K bytes (16 sectors) per PRDT
        for (int i = 0; i < header.prd_table_length - 1; i++)
        {
            table->prdt[i].data_base_address = static_cast<uint32_t>(reinterpret_cast<u64>(buffer));
            if (bits_is_64)
                table->prdt[i].data_base_address_upper = static_cast<uint32_t>(reinterpret_cast<u64>(buffer) >> 32);
            table->prdt[i].data_byte_count = 8 * 1024 - 1; // 8K bytes (this value should always be set to 1 less than the actual value)
            table->prdt[i].interrupt_on_complete = true;
            buffer += 4 * 1024;	// 4K words
            count -= sectors_per_prdt; // 16 sectors
        }

        // Last entry
        table->prdt[header.prd_table_length - 1].data_base_address = static_cast<uint32_t>(reinterpret_cast<u64>(buffer));
        if (bits_is_64)
            table->prdt[header.prd_table_length - 1].data_base_address_upper = static_cast<uint32_t>(reinterpret_cast<u64>(buffer) >> 32);
        table->prdt[header.prd_table_length - 1].data_byte_count = count * sector_size - 1; // 512 bytes per sector
        table->prdt[header.prd_table_length - 1].interrupt_on_complete = true;

        const auto command_fis = reinterpret_cast<fis::reg_h2d*>(table->command_fis);
        mem::memset(command_fis, 0, sizeof(fis::reg_h2d));
        command_fis->fis_type = static_cast<u8>(fis::type::FIS_TYPE_REG_H2D);
        command_fis->command_control = 1;
        command_fis->command = ATA_CMD_READ_DMA_EX;

        command_fis->lba0 = static_cast<u8>(start_lo);
        command_fis->lba1 = static_cast<u8>(start_lo >> 8);
        command_fis->lba2 = static_cast<u8>(start_lo >> 16);
        command_fis->device = 1 << 6; // set lba mode

        command_fis->lba3 = static_cast<u8>(start_lo >> 24);
        command_fis->lba4 = static_cast<u8>(start_hi);
        command_fis->lba5 = static_cast<u8>(start_hi >> 8);

        command_fis->count_lo = original_count & 0xFF;
        command_fis->count_hi = (original_count >> 8) & 0xFF;

        return issue_command(slot);
    }

    bool ahci_port::write(const u64 start, u32 count, const u16* buffer, const u16 sector_size) {
        clear_interrupt_errors();

        const auto slot = get_command_slot();
        if (slot == -1)
            return false;

        const u32 start_lo = static_cast<u32>(start);
        const u32 start_hi = static_cast<u32>(start >> 32);
        const u32 original_count = count;
        const u32 sectors_per_prdt = (8 * 1024) / sector_size;

        auto& header = command_list[slot];
        header.fis_length = sizeof(fis::reg_h2d) / sizeof(uint32_t);
        header.write = 1;
        header.prd_table_length = static_cast<uint16_t>((count - 1) / sectors_per_prdt) + 1;
        header.prefetchable = false;
        header.clear = true;

        if (header.prd_table_length > MAX_PRDT_SIZE)
            return false;

        const auto table = command_slots[slot].table;
        mem::memset(table, 0, sizeof(command_table));

        // 8K bytes (16 sectors) per PRDT
        for (int i = 0; i < header.prd_table_length - 1; i++)
        {
            table->prdt[i].data_base_address = static_cast<uint32_t>(reinterpret_cast<u64>(buffer));
            if (bits_is_64)
                table->prdt[i].data_base_address_upper = static_cast<uint32_t>(reinterpret_cast<u64>(buffer) >> 32);
            table->prdt[i].data_byte_count = 8 * 1024 - 1; // 8K bytes (this value should always be set to 1 less than the actual value)
            table->prdt[i].interrupt_on_complete = true;
            buffer += 4 * 1024;	// 4K words
            count -= sectors_per_prdt; // 16 sectors
        }

        // Last entry
        table->prdt[header.prd_table_length - 1].data_base_address = static_cast<uint32_t>(reinterpret_cast<u64>(buffer));
        if (bits_is_64)
            table->prdt[header.prd_table_length - 1].data_base_address_upper = static_cast<uint32_t>(reinterpret_cast<u64>(buffer) >> 32);
        table->prdt[header.prd_table_length - 1].data_byte_count = count * sector_size - 1; // 512 bytes per sector
        table->prdt[header.prd_table_length - 1].interrupt_on_complete = true;

        const auto command_fis = reinterpret_cast<fis::reg_h2d*>(table->command_fis);
        mem::memset(command_fis, 0, sizeof(fis::reg_h2d));
        command_fis->fis_type = static_cast<u8>(fis::type::FIS_TYPE_REG_H2D);
        command_fis->command_control = 1;
        command_fis->command = ATA_CMD_WRITE_DMA_EX;

        command_fis->lba0 = static_cast<u8>(start_lo);
        command_fis->lba1 = static_cast<u8>(start_lo >> 8);
        command_fis->lba2 = static_cast<u8>(start_lo >> 16);
        command_fis->device = 1 << 6; // set lba mode

        command_fis->lba3 = static_cast<u8>(start_lo >> 24);
        command_fis->lba4 = static_cast<u8>(start_hi);
        command_fis->lba5 = static_cast<u8>(start_hi >> 8);

        command_fis->count_lo = original_count & 0xFF;
        command_fis->count_hi = (original_count >> 8) & 0xFF;

        return issue_command(slot);
    }

    void ahci_port::on_interrupt() {
        if (port->interrupt_status.cold_port_detect_interrupt)
            log::warn("AHCI: Device on port %i has been removed or unable to be detected!", port_num);
        if (port->interrupt_status.task_file_error_interrupt) {
            log::error("AHCI: task file error (tfd: %x)", port->tfd);
            has_errored = true;
        }
        if (port->interrupt_status.host_bus_fatal_error_interrupt) {
            log::error("AHCI: host bus fatal error");
            has_errored = true;
        }
        if (port->interrupt_status.host_bus_data_error_interrupt) {
            log::error("AHCI: host bus data error");
            has_errored = true;
        }
        if (port->interrupt_status.interface_fatal_error_interrupt) {
            log::error("AHCI: interface fatal error (serr: %x)", port->serr);
            has_errored = true;
        }
        if (port->interrupt_status.interface_non_fatal_error_interrupt)
            log::warn("AHCI: interface non-fatal error (serr: %x)", port->serr);
        if (port->interrupt_status.overflow_interrupt)
            log::warn("AHCI: overflow error");
        if (port->interrupt_status.incorrect_port_multiplier_interrupt)
            log::warn("AHCI: incorrect port multiplier");
        if (port->interrupt_status.phy_ready_change_interrupt)
            log::warn("AHCI: PHY ready change");
        if (port->interrupt_status.port_connect_change_interrupt)
            log::warn("AHCI: device connected/disconnected on port %i", port_num);
        if (port->interrupt_status.device_to_host_fis_interrupt)
            has_received_command_data = true;

        if (has_errored) { // fatal error so do error recovery
            const u8 error_slot = (port->command_status >> 8) & 0x1F;
            const u32 pending = port->command_issue;

            stop();
            clear_interrupt_errors();
            if ((port->tfd & ATA_DEV_BUSY_BIT) || (port->tfd & ATA_DEV_DRQ_BIT)) {
                comreset();
                log::info("Doing a comreset...");
            }
            start();

            command_slots[error_slot].error = true;
            for (int i = 0; i < num_command_slots; i++) {
                if ((pending & (1 << i)) && i != error_slot)
                    port->command_issue |= (1 << i);
            }
        }

        clear_interrupt_errors();
    }

    bool ahci_port::issue_command(const u8 slot) {
        if (!wait_for_port())
            return false;
        x64::set_INT_flag(false);
        port->command_issue = 1 << slot;
        x64::set_INT_flag(true);
        if (!wait_for_port_completion(slot))
            return false;
        while (!has_received_command_data) {}
        has_received_command_data = false;
        return true;
    }

    void ahci_port::clear_interrupt_errors() {
        *reinterpret_cast<volatile u32*>(&port->interrupt_status) = 0xFFFFFFFF;
        port->serr = 0xFFFFFFFF;
        has_errored = false;
    }

    bool ahci_port::is_active() const {
        return active;
    }
}
