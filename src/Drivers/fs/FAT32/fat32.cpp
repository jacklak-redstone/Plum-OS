#include "fat32.hpp"
#include "Drivers/fs/partition/gpt_partition.h"
#include "Drivers/fs/partition/partition_manager.h"
#include "kernel/Sleep.hpp"

namespace fs::FAT32 {
    fat32_manager::~fat32_manager() {
        if (fat_partitions.empty())
            return;

        for (int i = 0; i < fat_partitions.size(); i++) {
            heap::free_align(&fat_partitions[i]);
        }
    }

    void fat32_manager::init(drivers::ahci::ahci_device &dev) {
        fat_partitions.clear();
        device = &dev;
        part_manager.init(*device);

        for (int i = 0; i < part_manager.get_partition_count(); i++) {
            auto fat = static_cast<fat_info *>(heap::malloc_align(sizeof(fat_info), 4));
            if (!part_manager.get_partition(i, fat->gpt_partition))
                continue;

            if (!device->read(fat->gpt_partition.starting_lba, 1, reinterpret_cast<u16 *>(&fat->bpb)))
                return;

            if (fat->bpb.root_entry_count == 0 && fat->bpb.FAT_size == 0) {
                std::kernel::printf("&aFAT32 ");
                fat_partitions.push_back(*fat);
            }

            char oem_name[9];
            for (int s = 0; s < 8; s++) {
                oem_name[s] = fat->bpb.OEM_Name[s];
            }
            oem_name[8] = '\0';

            char volume_string[12];
            for (int s = 0; s < 11; s++) {
                volume_string[s] = fat->bpb.ebpb.volume_label_string[s];
            }
            volume_string[11] = '\0';

            std::kernel::printf("&7partition &f%i: &7volume = &f%s &7oem = &f%s\n", i, volume_string, oem_name);
        }
    }
}
