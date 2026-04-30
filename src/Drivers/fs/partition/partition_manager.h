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
        void list_partitions();
    private:
        [[nodiscard]] bool validate_gpt() const;
        [[nodiscard]] bool validate_gpt_partition_entries(const u16* entries_buf) const;

        gpt_header* header;
        std::vector<gpt_partition> partitions;
        drivers::ahci::ahci_device* device;
    };
}
