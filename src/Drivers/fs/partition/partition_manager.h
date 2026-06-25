#pragma once
#include "gpt_partition.h"
#include "Drivers/achi/ahci_device.h"
#include "std/vector.hpp"

namespace fs::partition {
    class partition_manager {
    public:
        partition_manager() : header(nullptr), device(nullptr) { }

        ~partition_manager() = default;

        void init(drivers::ahci::ahci_device& dev);
        void list_partitions() const;
        u32 get_partition_count() const { return partitions.size(); }
        bool get_partition(u32 id, gpt_partition &partition);
    private:
        [[nodiscard]] bool validate_gpt() const;
        [[nodiscard]] bool validate_gpt_partition_entries(const u16* entries_buf) const;

        gpt_header* header;
        std::vector<gpt_partition> partitions;
        drivers::ahci::ahci_device* device;
    };
}
