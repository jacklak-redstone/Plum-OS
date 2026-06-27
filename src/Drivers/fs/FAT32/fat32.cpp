#include "fat32.hpp"
#include "Drivers/fs/partition/gpt_partition.h"
#include "Drivers/fs/partition/partition_manager.h"

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

            if (!device->read_bytes(fat->gpt_partition.starting_lba, sizeof(BPB), reinterpret_cast<u16 *>(&fat->bpb)))
                return;

            uint32_t data_start = fat->gpt_partition.starting_lba + fat->bpb.rsvd_sector_count + fat->bpb.num_of_FATs * fat->bpb.ebpb.sectors_per_FAT;
            fat->data_start = data_start;

            if (fat->bpb.root_entry_count == 0 && fat->bpb.FAT_size == 0) {
                std::kernel::printf("\n&aFAT32 &eid:&f%u ", fat_partitions.size());
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

    void fat32_manager::read(uint32_t first_cluster, uint8_t partition, int8_t depth, int8_t origin_d) {
        auto fat = fat_partitions[partition];
        if (first_cluster == 0)
            first_cluster = fat.bpb.ebpb.root_directory_cluster;

        const uint32_t cluster_size = fat.bpb.bytes_per_sector * fat.bpb.sectors_per_cluster;
        std::kernel::printf("cluster size: %u\n", cluster_size);

        const uint32_t lba = fat.data_start + (first_cluster - 2) * fat.bpb.sectors_per_cluster;

        auto *buffer = static_cast<file_entry *>(heap::malloc(cluster_size));

        device->read_bytes(lba, cluster_size, reinterpret_cast<uint16_t *>(buffer));

        char long_name[255] = {};
        const auto *entry = buffer;
        for (int i = 0; i < cluster_size/sizeof(file_entry); i++) {

            char short_name[12];
            for (int n = 0; n < 11; n++) {
                short_name[n] = entry[i].file_name[n];
            }
            short_name[11] = '\0';

            if (entry[i].file_name[0] == 0x0)
                break;

            if (static_cast<uint8_t>(entry[i].file_name[0]) == 0xE5) // Deleted file
                continue;

            if (entry[i].attributes == 0x08) {
                std::kernel::printf("VOLUME: %s\n", short_name);
                continue;
            }

            // Long file name
            if (entry[i].attributes == 0x0F) {
                auto *l_name = (long_file_name *)&entry[i];
                uint8_t idx = ((l_name->order & 0x1F) - 1) * 13;
                if (idx >= 255) {
                    long_name[254] = '\0';
                    continue;
                }

                for (int id = 0; id < 5; id++) {
                    if (l_name->name_1[id] == 0xFFFF) // garbage
                        continue;
                    if (l_name->name_1[id] == 0x0000) { // end of name
                        long_name[idx] = '\0';
                        break;
                    }

                    long_name[idx] = static_cast<char>(l_name->name_1[id]);
                    idx++;
                }

                for (int id = 0; id < 6; id++) {
                    if (l_name->name_2[id] == 0xFFFF) // garbage
                        continue;
                    if (l_name->name_2[id] == 0x0000) { // end of name
                        long_name[idx] = '\0';
                        break;
                    }

                    long_name[idx] = static_cast<char>(l_name->name_2[id]);
                    idx++;
                }

                for (int id = 0; id < 2; id++) {
                    if (l_name->name_3[id] == 0xFFFF) // garbage
                        continue;
                    if (l_name->name_3[id] == 0x0000) { // end of name
                        long_name[idx] = '\0';
                        break;
                    }

                    long_name[idx] = static_cast<char>(l_name->name_3[id]);
                    idx++;
                }
                continue;
            }

            for (int d = 0; d < (origin_d - depth); d++)
                std::kernel::printf("\t");

            if (entry[i].attributes & 0x10)
                std::kernel::printf("&aDIR: ");
            else if (entry[i].attributes & 0x20)
                std::kernel::printf("&eFILE: ");

            if (long_name[0] != '\0') {


                std::kernel::printf("%s ", long_name);

                long_name[0] = '\0';
            } else {
                std::kernel::printf("%s ", short_name);
            }

            std::kernel::printf("&7attr: &f%x &7size: &f%u\n", entry[i].attributes, entry[i].file_size);

            // Recursive search
            if (entry[i].attributes & 0x10 && short_name[0] != '.') {
                uint32_t cluster_dir = entry[i].first_cluster_low;
                cluster_dir |= (entry[i].first_cluster_high << 16);
                read(cluster_dir, partition, depth-1, origin_d);
            }
        }

        heap::free(buffer);
    }
}
