#pragma once
#include "Drivers/achi/ahci_device.h"
#include "Drivers/fs/partition/partition_manager.h"

namespace fs::FAT32 {
    struct eBPB {
        uint32_t sectors_per_FAT;
        uint16_t flags;
        uint16_t FAT_version;
        uint32_t root_directory_cluster;
        uint16_t FSinfo_sector_number;
        uint16_t backup_sector;
        uint8_t rsvd[12];
        uint8_t drive_number;
        uint8_t flags_Windows_NT;
        uint8_t signature; // 0x28 or 0x29
        uint32_t volume_id;
        uint8_t volume_label_string[11];
        uint8_t identifier_string[8]; // "FAT32  "
        uint8_t boot_code[420];
        uint16_t boot_partition_signature; // 0xAA55
    } __attribute((packed));

    struct BPB { // BIOS Parameter Block
        uint8_t jmp_instruction[3]; // yea what??
        uint8_t OEM_Name[8];
        uint16_t bytes_per_sector;
        uint8_t sectors_per_cluster;
        uint16_t rsvd_sector_count;
        uint8_t num_of_FATs;
        uint16_t root_entry_count;
        uint16_t total_sectors;
        uint8_t media_type;
        uint16_t FAT_size;
        uint16_t sectors_per_track;
        uint16_t heads_count;
        uint32_t hidden_sectors_count;
        uint32_t large_sector_count;
        eBPB ebpb;
    } __attribute__((packed));

    struct fat_info {
        partition::gpt_partition gpt_partition;
        BPB bpb;
    } __attribute__((packed));

    class fat32_manager {
    public:
        fat32_manager() = default;
        ~fat32_manager();
        void init(drivers::ahci::ahci_device& dev);

    private:

        drivers::ahci::ahci_device* device = nullptr;
        partition::partition_manager part_manager;
        std::vector<fat_info> fat_partitions;
    };
}
