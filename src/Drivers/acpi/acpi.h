#pragma once
#include "std/types.hpp"
#include "std/vector.hpp"
#include "uacpi/tables.h"
#include "uacpi/uacpi.h"
#include "uacpi/utilities.h"

namespace drivers::acpi {
    struct acpi_driver {
        const char* device_name;
        const char* const* pnp_ids;
        int (*device_probe)(uacpi_namespace_node* node, uacpi_namespace_node_info* info);
    };

    class acpi {
        public:
        acpi() = default;
        ~acpi() = default;

        i32 init();
        void register_driver(const acpi_driver &driver);
        void enumerate_bus();

        static void* find_table(const uacpi_char* signature);

        private:
        uacpi_iteration_decision init_device(void* ctx, uacpi_namespace_node *node, uacpi_u32 node_depth) const;

        std::vector<acpi_driver> drivers;
    };
}
