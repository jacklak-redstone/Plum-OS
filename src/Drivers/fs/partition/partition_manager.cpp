#include "partition_manager.h"

#include "kernel/log.h"

namespace fs::partition {
    // https://gist.github.com/xobs/91a84d29152161e973d717b9be84c4d0
    u32 crc32(const u8* data, const u32 len) {
        int i = 0;
        u32 crc = 0xFFFFFFFF;
        while (i < len) {
            crc = crc ^ data[i];
            for (int j = 7; j >= 0; j--) {
                const u32 mask = -(crc & 1);
                crc = (crc >> 1) ^ (0xEDB88320 & mask);
            }
            i = i + 1;
        }
        return ~crc;
    }

    void partition_manager::init(drivers::ahci::ahci_device& dev) {
        device = &dev;

        const auto buf = static_cast<u16*>(heap::malloc_align(512, 4));
        if (!dev.read(1, 1, buf)) {
            log::error("[ GPT ] Failed to read partition header.");
            return;
        }
        header = reinterpret_cast<gpt_header*>(buf);

        if (!validate_gpt()) {
            log::warn("[ GPT ] Failed to validate GPT header.");
            heap::free_align(buf);
            return;
        }

        log::success("[ GPT ] Found valid partition header!");

        const u32 total_size = header->partition_entry_size * header->partition_entry_count;
        const u32 sectors = total_size / dev.get_sector_size();
        const auto partitions_buf = static_cast<u16*>(heap::malloc_align(total_size, 4));

        if (!dev.read(header->partition_entry_lba, sectors, partitions_buf)) {
            heap::free_align(partitions_buf);
            heap::free_align(buf);
            return;
        }

        if (!validate_gpt_partition_entries(partitions_buf)) {
            log::warn("[ GPT ] Failed to validate GPT partition array.");
            heap::free_align(partitions_buf);
            heap::free_align(buf);
            return;
        }

        for (u32 i = 0; i < header->partition_entry_count; i++) {
            const auto* entry = reinterpret_cast<gpt_partition*>(reinterpret_cast<u8*>(partitions_buf) + i * header->partition_entry_size);
            if (mem::memcmp(entry->type_guid, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == true)
                continue;
            if (entry->starting_lba == 0)
                continue;
            if (entry->name[0] == 0)
                continue;

            char name_buf[37] = {};
            for (int j = 0; j < 36; j++) {
                u16 wc = entry->name[j];
                if (wc == 0) break;
                name_buf[j] = static_cast<char>(wc & 0xFF);
            }

            log::info("[ GPT ] Found partition %i with name of '%s'", i, name_buf);
            auto part_size = static_cast<double>(entry->ending_lba - entry->starting_lba) * static_cast<double>(dev.get_sector_size());
            log::info("[ GPT ] Partition size: %f%s", part_size, std::format_size(part_size));
            partitions.push_back(*entry);
        }

        heap::free_align(partitions_buf);
        heap::free_align(buf);
    }

    void partition_manager::list_partitions() {
        for (u32 i = 0; i < header->partition_entry_count; i++) {
            auto partition = &partitions.data[i];
            if (mem::memcmp(partition->type_guid, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == true)
                continue;
            if (partition->starting_lba == 0)
                continue;
            if (partition->name[0] == 0)
                continue;

            char name_buf[37] = {};
            for (int j = 0; j < 36; j++) {
                u16 wc = partition->name[j];
                if (wc == 0) break;
                name_buf[j] = static_cast<char>(wc & 0xFF);
            }

            log::info("[ GPT ] Found partition %i with name of '%s'", i, name_buf);
            auto part_size = static_cast<double>(partition->ending_lba - partition->starting_lba) * static_cast<double>(device->get_sector_size());
            log::info("[ GPT ] Partition size: %f%s", part_size, std::format_size(part_size));
        }
    }

    bool partition_manager::validate_gpt() const {
        if (mem::memcmp(&header->signature, "EFI PART", 8) == false) {
            log::warn("[ GPT ] Failed to find signature bytes for GPT partition.");
            return false;
        }

        gpt_header header_copy = *header;
        header_copy.header_checksum = 0;
        const u32 computed_crc = crc32(reinterpret_cast<const u8*>(&header_copy), header->header_size);
        if (computed_crc != header->header_checksum) {
            log::warn("[ GPT ] Partition header checksum mismatch.");
            return false;
        }

        if (header->lba_header != 1) {
            auto lba = header->lba_header;
            log::warn("[ GPT ] LBA header mismatch: got %x, expected 1", lba);
            return false;
        }

        return true;
    }

    bool partition_manager::validate_gpt_partition_entries(const u16* entries_buf) const {
        const u32 array_size = header->partition_entry_size * header->partition_entry_count;
        const u32 computed_crc = crc32(reinterpret_cast<const u8*>(entries_buf), array_size);
        if (computed_crc != header->partition_entry_checksum) {
            log::warn("[ GPT ] Partition array checksum mismatch.");
            return false;
        }

        return true;
    }
}
