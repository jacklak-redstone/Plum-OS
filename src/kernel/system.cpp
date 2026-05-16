#include "system.hpp"

#include "acpi.h"
#include "limine.h"
#include "linker_info.hpp"
#include "log.h"
#include "kernel/Memory/heap.hpp"
#include "arch/x86_64/IDT/IDT.hpp"
#include "arch/x86_64/Common/Common.hpp"
#include "Drivers/Keyboard.hpp"
#include "kernel/Sleep.hpp"
#include "kernel/Paging.hpp"
#include "arch/x86_64/gdt/gdt.h"
#include "Drivers/achi/ahci.h"
#include "Drivers/achi/ahci_device.h"
#include "Drivers/fs/partition/partition_manager.h"
#include "Drivers/GPU/framebuffer.hpp"
#include "Drivers/ps2/ps2.h"
#include "Drivers/USB/xHCI/xHCI.hpp"
#include "Memory/mem_helper.h"
#include "arch/x86_64/IDT/APIC.hpp"
#include "../Drivers/Network/Drivers/RTL8139.hpp"
#include "Drivers/hpet/hpet.h"
#include "uacpi/uacpi.h"
#include "Drivers/Network/Ethernet.hpp"

extern u64 kernel_address_vert;
extern u64 kernel_address_phys;
extern volatile limine_memmap_request memmap_request;

namespace systemPL {
    drivers::ahci::ahci ahci;
    framebuffer::framebuffer fb;
    fs::partition::partition_manager partition_manager;
    drivers::acpi::acpi acpi;
    IDT::IOAPIC ioapic;

    void Init(framebuffer::framebuffer_info framebuffer_info, u64 heap_addr) {
        init_tss();
        Paging::Init(); // 4KB page size

        // Heap Initialization
        heap::heap_init(1024*1024*64, heap_addr);
        Paging::Map_memory_vp(heap_addr, to_physical(heap_addr), 1024*1024*64, Paging::Profile::KernelData);

        // Paging
        Paging::Map_memory(0x0, 1024*1024*96, Paging::Profile::UserCode);

        u64 kernel_size = reinterpret_cast<u64>(&Linker::__kernel_end) - reinterpret_cast<u64>(&Linker::__kernel_start);
        Paging::Map_memory_vp(kernel_address_vert, kernel_address_phys, kernel_size, Paging::Profile::KernelCode | Paging::Writable);

        // Map HHDM
        for (u64 i = 0; i < memmap_request.response->entry_count; i++) {
            const auto* entry = memmap_request.response->entries[i];
            switch (entry->type) {
                case LIMINE_MEMMAP_USABLE:
                case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                case LIMINE_MEMMAP_ACPI_NVS:
                case LIMINE_MEMMAP_FRAMEBUFFER:
                    Paging::Map_memory_vp(hhdm_offset + entry->base, entry->base, entry->length, Paging::Profile::KernelData);
                    break;
                default:
                    break;
            }
        }

        Paging::Enable_paging();

        fb.init(framebuffer_info);

        IDT::IDT_Install();
        Time::Set_PIT(100); // 100Hz

        uint8_t acpi_early_buf[4096];
        uacpi_setup_early_table_access(acpi_early_buf, sizeof(acpi_early_buf));
        ioapic.init();
        acpi.init();

        log::info("\n");

        hpet::init();

        log::info("\n");

        USB::m_xhci_driver.init_device();
        USB::m_xhci_driver.start_device();

        RTL8139::driver.Init();

        drivers::ps2::init(acpi);
        acpi.enumerate_bus();

        log::info("\n");

        ahci.init();
        for (int i = 0; i < 32; ++i) {
            auto device = ahci.request_device(i);
            if (!device.is_active())
                continue;

            device.initialize();
            auto size = static_cast<double>(device.get_sector_count() * device.get_sector_size());
            log::info("Device info for device %i: \n"
                                "\tModel: %s\n"
                                "\tFirmware version: %s\n"
                                "\tSize: %f%s\n", i, device.get_model(), device.get_firmware(), size, std::format_size(size));

            //auto buffer = static_cast<u16*>(heap::malloc_align(device.get_sector_size(), 4));
            //mem::memset(buffer, 0, device.get_sector_size());
            //const auto value = "Hello World!\n";
            //mem::memmove(buffer, value, std::strlen(value));
            //device.write(0, 1, buffer);
            //heap::free_align(buffer);
            //
            //buffer = static_cast<u16*>(heap::malloc_align(device.get_sector_size(), 4));
            //device.read(0, 1, buffer);
            //std::kernel::printf("Read output: %s", buffer);
            //heap::free_align(buffer);
        }

        fb.swap();

        Time::Sleep(100);

        auto device = ahci.request_device(0);
        partition_manager.init(device);

        fb.swap();

        enter_user_space();
    }
}
