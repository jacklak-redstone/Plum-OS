#include <uacpi/kernel_api.h>

#include "limine.h"
#include "arch/x86_64/Common/Common.hpp"
#include "arch/x86_64/IDT/IDT.hpp"
#include "Drivers/PCI.hpp"
#include "kernel/log.h"
#include "kernel/Paging.hpp"
#include "kernel/Memory/heap.hpp"
#include "kernel/Sleep.hpp"
#include "kernel/Memory/mem_helper.h"
#include "kernel/system.hpp"
#include "uacpi/log.h"

extern volatile limine_rsdp_request rsdp_request;

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr* out_rsdp_address) {
    *out_rsdp_address = reinterpret_cast<uacpi_phys_addr>(rsdp_request.response->address);
    return UACPI_STATUS_OK;
}

void* uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    Paging::Map_memory_vp(to_virtual(addr), addr, len, Paging::Profile::MMIO);
    return reinterpret_cast<void*>(to_virtual(addr));
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {

}

void uacpi_kernel_log(uacpi_log_level level, const uacpi_char* msg) {
    char message[256];
    const auto len = std::strcpy(message, msg);
    message[len - 1] = '\0';
    switch (level) {
        case UACPI_LOG_ERROR:
            log::error("[ uACPI ] %s", message);
            break;
        case UACPI_LOG_WARN:
            log::warn("[ uACPI ] %s", message);
            break;
        case UACPI_LOG_INFO:
            log::info("[ uACPI ] %s", message);
            break;
        case UACPI_LOG_DEBUG:
        case UACPI_LOG_TRACE:
            log::info("[ uACPI ] %s", message);
            break;
    }
}

uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address, uacpi_handle *out_handle) { // TODO: add real pci device open
    auto* addr = static_cast<uacpi_pci_address*>(heap::malloc(sizeof(uacpi_pci_address)));
    if (!addr)
        return UACPI_STATUS_OUT_OF_MEMORY;
    *addr = address;
    *out_handle = addr;
    return UACPI_STATUS_OK;
}
void uacpi_kernel_pci_device_close(uacpi_handle handle) {
    heap::free(handle);
}

uacpi_status uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset, uacpi_u8 *value) {
    auto* addr = static_cast<uacpi_pci_address*>(device);
    *value = PCI::pci_read8(addr->bus, addr->device, addr->function, offset);
    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset, uacpi_u16 *value) {
    auto* addr = static_cast<uacpi_pci_address*>(device);
    *value = PCI::pci_read16(addr->bus, addr->device, addr->function, offset);
    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset, uacpi_u32 *value) {
    auto* addr = static_cast<uacpi_pci_address*>(device);
    *value = PCI::pci_read32(addr->bus, addr->device, addr->function, offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset, uacpi_u8 value) {
    auto* addr = static_cast<uacpi_pci_address*>(device);
    PCI::pci_write8(addr->bus, addr->device, addr->function, offset, value);
    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset, uacpi_u16 value) {
    auto* addr = static_cast<uacpi_pci_address*>(device);
    PCI::pci_write16(addr->bus, addr->device, addr->function, offset, value);
    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset, uacpi_u32 value) {
    auto* addr = static_cast<uacpi_pci_address*>(device);
    PCI::pci_write32(addr->bus, addr->device, addr->function, offset, value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle) {
    *out_handle = reinterpret_cast<uacpi_handle>(base);
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) { }

uacpi_status uacpi_kernel_io_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8* out) {
    *out = x64::inb(reinterpret_cast<uacpi_io_addr>(handle) + offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16* out) {
    *out = x64::inw(reinterpret_cast<uacpi_io_addr>(handle) + offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32* out) {
    *out = x64::inl(reinterpret_cast<uacpi_io_addr>(handle) + offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 val) {
    x64::outb(reinterpret_cast<uacpi_io_addr>(handle) + offset, val);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 val) {
    x64::outw(reinterpret_cast<uacpi_io_addr>(handle) + offset, val);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 val) {
    x64::outl(reinterpret_cast<uacpi_io_addr>(handle) + offset, val);
    return UACPI_STATUS_OK;
}

void *uacpi_kernel_alloc(uacpi_size size) {
    return heap::malloc(size);
}

void uacpi_kernel_free(void *mem) {
    heap::free(mem);
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    if (Time::hz == 0)
        return Time::tick;
    return (static_cast<uacpi_u64>(Time::tick) * 1000000000ULL) / Time::hz;
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    Time::Sleep(10);
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
    Time::Sleep(msec);
}

uacpi_handle uacpi_kernel_create_mutex(void) {
    return heap::malloc(1);
}
void uacpi_kernel_free_mutex(uacpi_handle handle) {
    heap::free(handle);
}


uacpi_handle uacpi_kernel_create_event(void) {
    return heap::malloc(1);
}
void uacpi_kernel_free_event(uacpi_handle handle) {
    heap::free(handle);
}


uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    return reinterpret_cast<uacpi_thread_id>(1);
}


uacpi_interrupt_state uacpi_kernel_disable_interrupts(void) {
    uacpi_interrupt_state state;
    asm volatile(
        "pushfq\n"
        "pop %0\n"
        "cli\n"
        : "=r"(state)
        :
        : "memory"
    );
    return state;
}

void uacpi_kernel_restore_interrupts(uacpi_interrupt_state state) {
    asm volatile(
        "push %0\n"
        "popfq\n"
        :
        : "r"(state)
        : "memory"
    );
}

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle, uacpi_u16) {
    return UACPI_STATUS_OK;
}
void uacpi_kernel_release_mutex(uacpi_handle) {

}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16) {
    return UACPI_TRUE;
}

void uacpi_kernel_signal_event(uacpi_handle) {

}

void uacpi_kernel_reset_event(uacpi_handle) {

}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request* req) {
    switch (req->type) {
        case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
            log::warn("[ uACPI ] Firmware breakpoint hit");
            asm volatile("int3");
            return UACPI_STATUS_OK;
        case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
            log::error("[ uACPI ] Firmware fatal: type=%x code=%x arg=%x", req->fatal.type, req->fatal.code, req->fatal.arg);
            asm volatile("cli; hlt");
            return UACPI_STATUS_OK;
        default:
            return UACPI_STATUS_UNIMPLEMENTED;
    }
}

uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx, uacpi_handle *out_irq_handle) {
    const auto vector = static_cast<u8>(irq + 32);
    const auto iso = apic::IOAPIC::resolve_irq(static_cast<u8>(irq));
    systemPL::ioapic.route(iso.gsi, vector,
                           apic::IOAPIC::DeliveryMode::Fixed,
                           iso.level_triggered ? apic::IOAPIC::TriggerMode::LEVEL : apic::IOAPIC::TriggerMode::EDGE,
                           iso.active_low, false);
    IDT::install_uacpi_handler(handler, vector, ctx);
    *out_irq_handle = reinterpret_cast<uacpi_handle>(static_cast<u64>(irq));
    return UACPI_STATUS_OK;
}


uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle irq_handle) {
    const u32 irq = static_cast<u32>(reinterpret_cast<u64>(irq_handle));
    IDT::install_uacpi_handler(nullptr, static_cast<u8>(irq + 32), nullptr);
    return UACPI_STATUS_OK;
}

uacpi_handle uacpi_kernel_create_spinlock(void) {
    return heap::malloc(1);
}
void uacpi_kernel_free_spinlock(uacpi_handle handle) {
    heap::free(handle);
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle) {
    uacpi_cpu_flags flags;
    asm volatile(
        "pushfq\n"
        "pop %0\n"
        "cli\n"
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}
void uacpi_kernel_unlock_spinlock(uacpi_handle, uacpi_cpu_flags flags) {
    asm volatile(
        "push %0\n"
        "popfq\n"
        :
        : "r"(flags)
        : "memory"
    );
}

uacpi_status uacpi_kernel_schedule_work(uacpi_work_type, uacpi_work_handler handler, uacpi_handle ctx) {
    handler(ctx);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    return UACPI_STATUS_OK;
}