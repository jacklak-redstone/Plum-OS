#pragma once
#include "Drivers/ata/fis.h"
#include "std/types.hpp"

namespace drivers::ahci {
    struct hba_memory;

    constexpr u64 SATA_SIG_ATAPI = 0xEB140101;
    constexpr u64 SATA_SIG_ATA =   0x00000101;
    constexpr u64 SATA_SIG_SEMB =  0xC33C0101;
    constexpr u64 SATA_SIG_PM =    0x96690101;

    constexpr u64 CMD_CR_BIT = 0x8000;
    constexpr u64 CMD_FRE_BIT = 0x0010;
    constexpr u64 CMD_FR_BIT = 0x4000;
    constexpr u64 CMD_ST_BIT = 0x0001;

    constexpr u8 MAX_PRDT_SIZE = 8;
    constexpr u32 PORT_BUFFER_SIZE = 128 * 1024;

    constexpr u32 ATA_CMD_IDENTIFY = 0xEC;
    constexpr u32 ATA_CMD_READ_DMA = 0xC8;
    constexpr u32 ATA_CMD_READ_DMA_EX = 0x25;
    constexpr u32 ATA_CMD_WRITE_DMA = 0xCA;
    constexpr u32 ATA_CMD_WRITE_DMA_EX = 0x35;

    constexpr u32 ATA_DEV_BUSY_BIT = 0x80;
    constexpr u32 ATA_DEV_DRQ_BIT =  0x08;


    enum class port_type : u8 {
        none = 0,
        sata = 1,
        semb = 2,
        pm = 3,
        satapi = 4,
    };

    struct command_header {
        u8 fis_length : 5; // Command FIS length in DWORDS, 2 ~ 16
        u8 atapi : 1;
        u8 write : 1; // 1: H2D, 0: D2H
        u8 prefetchable : 1;

        u8 reset : 1;
        u8 bist : 1;
        u8 clear : 1; // Clear busy upon R_OK
        u8 reserved : 1;
        u8 port_mul_port : 4;

        u16 prd_table_length;

        volatile u32 prd_byte_count;

        u32 cmd_table_base_address;
        u32 cmd_table_base_address_upper;

        u32 reserved1[4];
    };

    struct received_fis {
        // 0x00
        fis::dma_setup dma_setup;
        u8 pad0[4];

        // 0x20
        fis::pio_setup pio_setup;
        u8 pad01[12];

        // 0x40
        fis::reg_d2h register_d2h;
        u8 pad2[4];

        // 0x58
        u8 sdbfis[8];

        // 0x60
        u8 unknown_fis[64];

        // 0xA0
        u8 reserved[0x100-0xA0];
    };

    struct interrupts {
        u32 device_to_host_fis_interrupt : 1;
        u32 pio_setup_fis_interrupt : 1;
        u32 dma_setup_fis_interrupt : 1;
        u32 set_device_bits_fis_interrupt : 1;
        u32 unknown_fis_interrupt : 1;
        u32 descriptor_processed_interrupt : 1;
        u32 port_connect_change_interrupt : 1;
        u32 device_mechanical_presence_interrupt : 1;
        u32 reserved : 14;
        u32 phy_ready_change_interrupt : 1;
        u32 incorrect_port_multiplier_interrupt : 1;
        u32 overflow_interrupt : 1;
        u32 reserved2 : 1;
        u32 interface_non_fatal_error_interrupt : 1;
        u32 interface_fatal_error_interrupt : 1;
        u32 host_bus_data_error_interrupt : 1;
        u32 host_bus_fatal_error_interrupt : 1;
        u32 task_file_error_interrupt : 1;
        u32 cold_port_detect_interrupt : 1;
    } __attribute__((packed));

    struct hba_port {
        volatile u32 command_list_base;		// 0x00, command list base address, 1K-byte aligned
        volatile u32 command_list_base_upper;		// 0x04, command list base address upper 32 bits
        volatile u32 fis_base_address;		// 0x08, FIS base address, 256-byte aligned
        volatile u32 fis_base_address_upper;		// 0x0C, FIS base address upper 32 bits
        volatile interrupts interrupt_status;
        volatile interrupts interrupts_enabled;
        volatile u32 command_status;		// 0x18, command and status
        volatile u32 rsv0;		// 0x1C, Reserved
        volatile u32 tfd;		// 0x20, task file data
        volatile u32 sig;		// 0x24, signature
        volatile u32 ssts;		// 0x28, SATA status (SCR0:SStatus)
        volatile u32 sctl;		// 0x2C, SATA control (SCR2:SControl)
        volatile u32 serr;		// 0x30, SATA error (SCR1:SError)
        volatile u32 sact;		// 0x34, SATA active (SCR3:SActive)
        volatile u32 command_issue;		// 0x38, command issue
        volatile u32 sntf;		// 0x3C, SATA notification (SCR4:SNotification)
        volatile u32 fbs;		// 0x40, FIS-based switch control
        volatile u32 rsv1[11];	// 0x44 ~ 0x6F, Reserved
        volatile u32 vendor[4];	// 0x70 ~ 0x7F, vendor specific
    };

    struct prdt_entry {
        u32 data_base_address;
        u32 data_base_address_upper;
        u32 reserved0;

        u32 data_byte_count : 22; // 4M max
        u32 reserved1 : 9;
        u32 interrupt_on_complete : 1;
    };

    struct command_table {
        // 0x00
        u8 command_fis[64];

        // 0x40
        u8 atapi_command[16]; // 12 or 16 bytes

        // 0x50
        u8 reserved[48];

        // 0x80
        prdt_entry prdt[MAX_PRDT_SIZE];	// Physical region descriptor table entries, 0 ~ 65535
    };

    struct command_slot {
        command_table* table {};
        volatile bool complete = false;
        volatile bool error = false;
    };

    class ahci_port {
    public:
        ahci_port() : command_list(nullptr), received(nullptr), type(port_type::none), port(nullptr),
                      num_command_slots(0) {
        }

        ~ahci_port() = default;

        void configure(port_type type, volatile hba_port *port, u8 port_num, volatile hba_memory* hba);

        void start() const;
        void stop() const;

        void comreset() const;

        bool read(u64 start, u32 count, u16* buffer, u16 sector_size);
        bool write(u64 start, u32 count, const u16* buffer, u16 sector_size);

        bool identify(const u16* buffer);
        void on_interrupt();

        [[nodiscard]] bool is_active() const;
    private:
        [[nodiscard]] i8 get_command_slot() const;
        [[nodiscard]] bool wait_for_port() const;
        [[nodiscard]] bool wait_for_port_completion(u8 slot);
        bool issue_command(u8 slot);
        void clear_interrupt_errors();

        command_header* command_list;
        received_fis* received;

        //command_table* cmd_tables[32];
        command_slot command_slots[32];
    public:
        bool active = false;
        volatile bool has_received_command_data = false;
        bool bits_is_64 = false;
        volatile bool has_errored = false;
        port_type type;
        volatile hba_port* port;
        u8 port_num = 0;
        u8 num_command_slots;
    };
}
