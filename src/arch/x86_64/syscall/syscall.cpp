#include "syscall.h"

#include <kernel/Sleep.hpp>

#include "arch/x86_64/Common/Common.hpp"
#include "Drivers/Keyboard.hpp"
#include "Drivers/PCI.hpp"
#include "Drivers/vga.h"
#include "Drivers/GPU/framebuffer.hpp"
#include "Drivers/hpet/hpet.h"
#include "kernel/system.hpp"
#include "kernel/Memory/heap.hpp"

extern "C" u64 user_rsp = 0;

extern "C" u64 user_rcx = 0;
extern "C" u64 user_r11 = 0;

auto validate_user_ptr = [](const u64 ptr) -> bool {
    return ptr != 0;
};

enum class syscall : u64 {
    write = 0,
    put_char = 1,
    serial_write = 2,
    serial_put_char = 3,
    get_key = 4,
    exit = 5,
    sleep = 6,
    pci = 7,
    heap = 8,
    swap_framebuffer = 9,
    list_partitions = 10,
};

extern "C" u64 dispatch_syscall(u64 id, u64 arg1, u64 arg2, u64 arg3) {
    switch (static_cast<syscall>(id)) {
        case syscall::write:
            if (!validate_user_ptr(arg1))
                return static_cast<u64>(-1);
            for (int i = 0; reinterpret_cast<const char*>(arg1)[i] != '\0'; i++) {
                systemPL::fb.put_char(reinterpret_cast<const char*>(arg1)[i], color_to_rgb(static_cast<Color>(arg2)));
            }
            return 0;
        case syscall::put_char:
            systemPL::fb.put_char(static_cast<char>(arg1), color_to_rgb(static_cast<Color>(arg2)));
            return 0;
        case syscall::serial_put_char:
            while (!(x64::inb(0x3F8 + 5) & 0x20)) { }
            x64::outb(0x3F8, static_cast<char>(arg1));
            return 0;
        case syscall::serial_write: {
            if (!validate_user_ptr(arg1))
                return static_cast<u64>(-1);
            const auto text = reinterpret_cast<const char *>(arg1);
            for (int i = 0; text[i] != '\0'; i++) {
                while (!(x64::inb(0x3F8 + 5) & 0x20)) { }
                x64::outb(0x3F8, text[i]);
            }
            return 0;
        }
	    case syscall::get_key:
                return static_cast<u64>(kb::get_char());
        case syscall::exit:
            return 0;
        case syscall::sleep:
            systemPL::fb.swap(); // TODO remove all the swaps everywhere and just keep swapping at a fixed itnerval on a separate thread when we have threads
            //Time::Sleep(arg1);
            hpet::sleep_us(arg1);
            return 0;
        case syscall::pci:
            PCI::Test();
            return 0;
        case syscall::heap:
            heap::dump_heap(arg1);
            return 0;
        case syscall::swap_framebuffer:
            systemPL::fb.swap();
            return 0;
        case syscall::list_partitions:
            systemPL::partition_manager.list_partitions();
            return 0;
        default:
            return static_cast<u64>(-1); // ENOSYS
    }
}
