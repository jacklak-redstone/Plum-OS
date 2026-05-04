#include "acpi.h"
#include <asm-generic/errno-base.h>
#include "kernel/log.h"
#include "kernel/system.hpp"
#include "std/string.h"

i32 drivers::acpi::acpi::init() {
    uacpi_status ret = uacpi_initialize(0);
    if (uacpi_unlikely_error(ret)) {
        log::error("[ uACPI ] Failed to initialize uACPI: %s", uacpi_status_to_string(ret));
        return -ENODEV;
    }

    ret = uacpi_namespace_load();
    if (uacpi_unlikely_error(ret)) {
        log::error("[ uACPI ] Failed to load namespaces: %s", uacpi_status_to_string(ret));
        return -ENODEV;
    }

    ret = uacpi_namespace_initialize();
    if (uacpi_unlikely_error(ret)) {
        log::error("[ uACPI ] Failed to initialize namespaces: %s", uacpi_status_to_string(ret));
        return -ENODEV;
    }

    log::success("[ uACPI ] Initialized uACPI!");
    return 0;
}

void drivers::acpi::acpi::register_driver(const acpi_driver& driver) {
    drivers.push_back(driver);
}

void drivers::acpi::acpi::enumerate_bus() {
    log::info("[ uACPI ] Enumerating buses...");
    uacpi_namespace_for_each_child(uacpi_namespace_root(), [](void* ctx, uacpi_namespace_node* node, uacpi_u32 node_depth) -> uacpi_iteration_decision {
        auto* self = static_cast<acpi*>(ctx);
        return self->init_device(ctx, node, node_depth);
    }, nullptr, UACPI_OBJECT_DEVICE_BIT, UACPI_MAX_DEPTH_ANY, this);
}

void* drivers::acpi::acpi::find_table(const uacpi_char* signature) {
    uacpi_table table;
    const auto ret = uacpi_table_find_by_signature(signature, &table);
    if (uacpi_unlikely_error(ret)) {
        log::error("[ uACPI ] Failed to find table: %s", uacpi_status_to_string(ret));
        return nullptr;
    }
    return table.ptr;
}

uacpi_iteration_decision drivers::acpi::acpi::init_device(void* ctx, uacpi_namespace_node *node, uacpi_u32 node_depth) const
{
    uacpi_namespace_node_info* info;
    const uacpi_status ret = uacpi_get_namespace_node_info(node, &info);
    if (uacpi_unlikely_error(ret)) {
        const char* path = uacpi_namespace_node_generate_absolute_path(node);
        log::error("[ uACPI ] Unable to retrieve node %s information: %s", path, uacpi_status_to_string(ret));
        uacpi_free_absolute_path(path);
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    const acpi_driver* drv = nullptr;

    if (info->flags & UACPI_NS_NODE_INFO_HAS_HID) {
        for (i32 i = 0; i < drivers.size; i++) {
            auto& driver = drivers.data[i];
            for (const char* const* id = driver.pnp_ids; *id != nullptr; id++) {
                if (std::str_cmp(info->hid.value, *id) == true) {
                    drv = &driver;
                    break;
                }
            }
            if (drv)
                break;
        }
    }

    if (drv == nullptr && (info->flags & UACPI_NS_NODE_INFO_HAS_CID)) {
        for (i32 i = 0; i < drivers.size; i++) {
            auto& driver = drivers.data[i];
            for (const char* const* id = driver.pnp_ids; *id != nullptr; id++) {
                for (u32 j = 0; j < info->cid.num_ids; j++) {
                    if (std::str_cmp(info->cid.ids[j].value, *id) == true) {
                        drv = &driver;
                        break;
                    }
                }
                if (drv)
                    break;
            }
            if (drv)
                break;
        }
    }

    if (drv != nullptr) {
        auto res = drv->device_probe(node, info);
        if (res != UACPI_STATUS_OK) {
            log::error("[ uACPI ] Failed to probe %s!", drv->device_name);
        }
    }

    uacpi_free_namespace_node_info(info);
    return UACPI_ITERATION_DECISION_CONTINUE;
}
