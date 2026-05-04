#pragma once
#include "acpi.h"
#include "Drivers/achi/ahci.h"
#include "Drivers/fs/partition/partition_manager.h"
#include "Drivers/GPU/framebuffer.hpp"
#include "std/types.hpp"

struct Framebuffer;

namespace systemPL {
    extern "C" u64 kernel_rsp;

    extern "C" void enter_user_space();

    extern "C" void handle_syscall();

    extern drivers::ahci::ahci ahci;
    extern framebuffer::framebuffer fb;
    extern fs::partition::partition_manager partition_manager;
    extern drivers::acpi::acpi acpi;

    void Init(framebuffer::framebuffer_info framebuffer_info, u64 heap_addr);
}
