#include "system.hpp"

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
#include "Drivers/USB/xHCI/xHCI.hpp"
#include "Memory/mem_helper.h"

extern u64 kernel_address_vert;
extern u64 kernel_address_phys;

namespace systemPL {
    drivers::ahci::ahci ahci;
    framebuffer::framebuffer fb;
    fs::partition::partition_manager partition_manager;

    void Init(framebuffer::framebuffer_info framebuffer_info, u64 heap_addr) {
        init_tss();
        Paging::Init(); // 4KB page size
        IDT::IDT_Install();
        Time::Set_PIT(100); // 100Hz

        // Heap Initialization
        heap::heap_init(1024*1024*8, heap_addr);
        Paging::Map_memory_vp(heap_addr, to_physical(heap_addr), 1024*1024*8, Paging::Profile::KernelData);

        // Paging
        Paging::Map_memory(0x0, 1024*1024*16, Paging::Profile::UserCode);

        u64 kernel_size = reinterpret_cast<u64>(&Linker::__kernel_end) - kernel_address_vert;
        Paging::Map_memory_vp(kernel_address_vert, kernel_address_phys, kernel_size, Paging::Profile::KernelCode | Paging::Writable);

        kb::flush_keyboard();

        Paging::Enable_paging();

        x64::set_INT_flag(true); // Enable interrupts

        fb.init(framebuffer_info);

        fb.swap();

        // USB::m_xhci_driver.init_device();
        // USB::m_xhci_driver.start_device();

        fb.swap();

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

        kernel_rsp = reinterpret_cast<u64>(&Linker::stack_top);
        enter_user_space();
    }
}
