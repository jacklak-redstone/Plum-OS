#include "RTL8139.hpp"
#include "Drivers/PCI.hpp"
#include "kernel/log.h"
#include "arch/x86_64/Common/Common.hpp"
#include "kernel/Sleep.hpp"
#include "kernel/Memory/heap.hpp"
#include "kernel/Memory/mem_helper.h"
#include "std/mem_common.hpp"
#include "Drivers/Network/Ethernet.hpp"

namespace RTL8139 {

    using namespace NET;

    RTL8139 driver;

    uint16_t RTL8139::IO_base = 0;

    const uint8_t *RTL8139::get_mac() const {
        return mac;
    }

    const char *RTL8139::get_name() const {
        return "RTL8139";
    }

    RTL8139::~RTL8139() = default;

    bool RTL8139::Find_Device() {
        device = PCI::find_vendor_class(0x10EC, 0x02, 0x00);

        if (device.vendor_id == 0) {
            log::error("[ NET ] RTL8139 not found");
            return false;
        }

        if (device.device_id != 0x8139) {
            log::error("[ NET ] RTL8139 Found Realtek but not RTL8139: %x", device.device_id);
            return false;
        }

        return true;
    }

    bool RTL8139::Reset_Device() {
        uint16_t cmd = PCI::pci_read16(device.bus, device.device, device.function, 0x04);
        cmd |= (1 << 2) | (1 << 0); // bus master, IO space
        PCI::pci_write16(device.bus, device.device, device.function, 0x04, cmd);

        IO_base = device.bar[0] & 0xFFFC;

        // Restart Device

        x64::outb(IO_base + REG_CONFIG1, 0x0); // Power on

        // Software reset
        x64::outb(IO_base + REG_CMD, 0x10);
        int timeout = 5;
        while( (x64::inb(IO_base + REG_CMD) & 0x10) != 0) {
            if (timeout-- <= 0) {
                log::error("[ NET ] RTL8139 Timed out");
                return false;
            }
            Time::Sleep(10);
        }

        // -----------------
        // Read MAC
        for (int i = 0; i < 6; i++)
            mac[i] = x64::inb(IO_base + i);

        log::info(
        "[ NET ] RTL8139 MAC: %x:%x:%x:%x:%x:%x",
        mac[0], mac[1], mac[2],
        mac[3], mac[4], mac[5]);

        return true;
    }

    void RTL8139::Configure_Device() {
        log::info("%x driver", &driver);
        // -----------------
        // RX buffer
        rx_buffer = static_cast<uint8_t *>(heap::malloc_align(RX_BUF_SIZE, 4096));
        x64::outl(IO_base + REG_RBSTART, to_physical(rx_buffer));

        // RX config
        x64::outl(IO_base + REG_RCR, RCR_VAL);

        // enable interrupts
        x64::outw(IO_base + REG_IMR, 0x0005);

        // Clear pending interrupts
        x64::outw(IO_base + REG_ISR, 0xFFFF);

        // Enable RX TX
        x64::outb(IO_base + REG_CMD, 0x0C);

        // Reset CARP
        rx_offset = 0;
        x64::outw(IO_base + REG_CAPR, 0xFFF0);

        // Setup TX buffers
        tx_base = static_cast<uint8_t *>(heap::malloc_align(2048 * 4, 4096));
        tx_buffers[0] = tx_base;
        tx_buffers[1] = tx_base + 2048;
        tx_buffers[2] = tx_base + 4096;
        tx_buffers[3] = tx_base + 6144;
    }

    bool RTL8139::Init() {
        if (!Find_Device())
            return false;

        log::success("[ NET ] RTL8139 Found");

        if (!Reset_Device())
            return false;

        Configure_Device();

        PCI::install_interrupt(device, RX_handler, 15);

        log::success("[ NET ] RTL8139 Initialized");

        return true;
    }

    void RTL8139::RX_handler(const IDT::ISR_Registers *regs) {
        uint16_t isr = x64::inw(IO_base + REG_ISR);
        x64::outw(IO_base + REG_ISR, isr);

        while ((x64::inb(IO_base + REG_CMD) & 0x01) == 0) {
            // Header: 2B status, 2B length
            const auto* hdr    = reinterpret_cast<uint16_t*>(driver.rx_buffer + driver.rx_offset);
            uint16_t  status = hdr[0];
            uint16_t  length = hdr[1];

            if (!(status & 0x1)) {
                log::info("[ NET ] RTL8139 ROK not set");
                break;
            }
            if (status & (1 << 1)) {
                log::info("[ NET ] RTL8139 Frame alignment error");
            }
            if (status & (1 << 2)) {
                log::info("[ NET ] RTL8139 CRC error");
            }
            if (status & (1 << 3)) {
                log::info("[ NET ] RTL8139 Packet too long");
            }
            if (status & (1 << 4)) {
                log::info("[ NET ] RTL8139 Runt packet");
            }

            uint8_t* packet = driver.rx_buffer + driver.rx_offset + 4;

            receive_ethernet(&driver, packet, length);

            // Advance and align to 4B
            driver.rx_offset = (driver.rx_offset + length + 4 + 3) & ~3;
            driver.rx_offset %= 8192;

            x64::outw(IO_base + REG_CAPR, driver.rx_offset - 16);
        }
    }

    bool RTL8139::send(const uint8_t* data, uint16_t length) {
        if (length > 1500) {
            log::error("[ NET ] RTL8139 Packet too large: %u", length);
            return false;
        }

        mem::memcpy(tx_buffers[tx_slot], data, length);

        const auto phys_tx = to_physical(tx_buffers[tx_slot]);

        // Set TX address and length
        x64::outl(IO_base + REG_TSAD0 + tx_slot * 4, static_cast<uint32_t>(phys_tx));
        x64::outl(IO_base + REG_TSD0  + tx_slot * 4, length & 0x1FFF);

        tx_slot = (tx_slot + 1) % 4;
        return true;
    }
}
