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

extern u64 kernel_address_vert;
extern u64 kernel_address_phys;
extern u64 hddm_offset;

namespace systemPL {
    drivers::ahci::ahci ahci;
    framebuffer::framebuffer fb;
    fs::partition::partition_manager partition_manager;

    void Init(framebuffer::framebuffer_info framebuffer, u64 heap_addr) {
        init_tss();

        //Multiboot::Init(static_cast<uint8_t *>(mbi));
        Paging::Init(); // 4KB page size

        // GDT is done in gdt.asm and elevate.asm
        IDT::IDT_Install();

        // Timer frequency
        Time::Set_PIT(100); // 100Hz

        // Heap Initialization
        heap::heap_init(1024*1024*8, heap_addr);
        Paging::Map_memory_vp(heap_addr, heap_addr - hddm_offset, 1024*1024*8, Paging::Profile::KernelData);

        // Paging
        Paging::Map_memory(0x0, 1024*1024*16, Paging::Profile::UserCode);

        // Helper: kernel virtual address → physical address
        auto kphys = [&](u64 virt) { return virt - kernel_address_vert + kernel_address_phys; };

        // .limine_requests (load base up to __kernel_start)
        u64 limine_size = reinterpret_cast<u64>(&Linker::__kernel_start) - kernel_address_vert;
        if (limine_size > 0)
            Paging::Map_memory_vp(kernel_address_vert, kernel_address_phys, limine_size, Paging::Profile::KernelCode);

        // .text — executable, not writable
        u64 text_start = reinterpret_cast<u64>(&Linker::__kernel_text_start);
        u64 text_size  = reinterpret_cast<u64>(&Linker::__kernel_text_end) - text_start;
        if (text_size > 0)
            Paging::Map_memory_vp(text_start, kphys(text_start), text_size, Paging::Profile::KernelCode);

        // .rodata — not writable, not execute
        u64 rodata_start = reinterpret_cast<u64>(&Linker::__kernel_rodata_start);
        u64 rodata_size  = reinterpret_cast<u64>(&Linker::__kernel_rodata_end) - rodata_start;
        if (rodata_size > 0)
            Paging::Map_memory_vp(rodata_start, kphys(rodata_start), rodata_size, Paging::Profile::KernelCode);

        // .data — writable, no execute
        u64 data_start = reinterpret_cast<u64>(&Linker::__kernel_data_start);
        u64 data_size  = reinterpret_cast<u64>(&Linker::__kernel_data_end) - data_start;
        if (data_size > 0)
            Paging::Map_memory_vp(data_start, kphys(data_start), data_size, Paging::Profile::KernelData);

        // .bss — writable, no execute (holds globals, IDT, PML4, etc.)
        u64 bss_start = reinterpret_cast<u64>(&Linker::__kernel_bss_start);
        u64 bss_size  = reinterpret_cast<u64>(&Linker::__kernel_bss_end) - bss_start;
        if (bss_size > 0)
            Paging::Map_memory_vp(bss_start, kphys(bss_start), bss_size, Paging::Profile::KernelData);

        u64 stack_virt = reinterpret_cast<u64>(&Linker::stack_bottom);
        u64 stack_phys = stack_virt - kernel_address_vert + kernel_address_phys;
        u64 stack_size = reinterpret_cast<u64>(&Linker::stack_top)
                       - reinterpret_cast<u64>(&Linker::stack_bottom);
        Paging::Map_memory_vp(stack_virt, stack_phys, stack_size, Paging::Profile::KernelStack);

        u64 ustack_virt = reinterpret_cast<u64>(&Linker::user_stack_bottom);
        u64 ustack_phys = ustack_virt - kernel_address_vert + kernel_address_phys;
        u64 ustack_size = reinterpret_cast<u64>(&Linker::user_stack_top)
                        - reinterpret_cast<u64>(&Linker::user_stack_bottom);
        Paging::Map_memory_vp(ustack_virt, ustack_phys, ustack_size, Paging::Profile::UserData);

        // Framebuffer address from Limine is an HHDM virtual address
        Paging::Map_memory_vp(
            reinterpret_cast<u64>(framebuffer.base),
            reinterpret_cast<u64>(framebuffer.base) - hddm_offset,
            framebuffer.size,
            Paging::Profile::MMIO
        );

        kb::flush_keyboard();

        // Map Limine's stack — it lives in HHDM space at an arbitrary physical
        // address we don't otherwise know. Without this, the ret inside
        // Enable_paging() faults immediately after CR3 is loaded.
        {
            u64 rsp;
            asm volatile("mov %%rsp, %0" : "=r"(rsp));
            // Limine gives a 64 KiB stack; cover it plus guard room (128 KiB window)
            u64 base = (rsp - 0x10000) & ~(u64)0xFFF;
            Paging::Map_memory_vp(base, base - hddm_offset, 0x20000, Paging::Profile::KernelStack);
        }

        Paging::Enable_paging();

        x64::set_INT_flag(true); // Enable interrupts

        fb.init(framebuffer);

        fb.swap();

        // USB::m_xhci_driver.init_device();
        // USB::m_xhci_driver.start_device();

        fb.swap();

        ahci.init();
        fb.swap();
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

        log::info("About to enter user space\n");
        log::info("About to enter user space\n");
        log::info("About to enter user space\n");
        fb.swap();
        log::info("About to enter user space\n");
        log::info("About to enter user space\n");
        log::info("About to enter user space\n");
        fb.swap();
        enter_user_space();
    }
}
