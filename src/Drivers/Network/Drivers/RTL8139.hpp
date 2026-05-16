#pragma once
#include "arch/x86_64/IDT/IDT.hpp"
#include "Drivers/PCI.hpp"
#include "Drivers/Network/Net_Device.hpp"
#include "std/types.hpp"

namespace RTL8139 {
    class RTL8139 : public NET::Net_Device {
    public:
        RTL8139() = default;
        ~RTL8139() override;

        bool Init() override;

        bool send(const uint8_t *data, uint16_t len) override;

        [[nodiscard]] const uint8_t *get_mac() const override;

        [[nodiscard]] const char *get_name() const override;

    private:
        bool Find_Device();
        bool Reset_Device();
        void Configure_Device();
        static void RX_handler(const IDT::ISR_Registers *regs);

        PCI::PCI_Device device{};

        static uint16_t IO_base;
        uint8_t mac[6] = {};
        uint16_t rx_offset{};
        uint8_t *rx_buffer{};

        uint8_t tx_slot{};
        uint8_t *tx_buffers[4]{};
        uint8_t *tx_base{};

        // Constants
        static constexpr uint16_t REG_MAC       = 0x00; // MAC Start
        static constexpr uint16_t REG_RBSTART   = 0x30; // RX Buffer Start Address
        static constexpr uint16_t REG_CMD       = 0x37; // Command Register
        static constexpr uint16_t REG_CAPR      = 0x38; // Read Pointer (Current address of packet Read)
        static constexpr uint16_t REG_IMR       = 0x3C; // interrupt Mask
        static constexpr uint16_t REG_ISR       = 0x3E; // Status Register
        static constexpr uint16_t REG_TCR       = 0x40; // Transmit Configuration Register
        static constexpr uint16_t REG_RCR       = 0x44; // Receive Configuration Register
        static constexpr uint16_t REG_CONFIG1   = 0x52; // Config reg 1
        static constexpr uint16_t REG_TSAD0     = 0x20; // Transmit Start address descriptor
        static constexpr uint16_t REG_TSD0      = 0x10; // Transmit status descriptor
        static constexpr uint32_t RX_BUF_SIZE   = 8192 + 16 + 1500; // Buffer size + overhead

        static constexpr uint32_t RCR_VAL =
        (1 << 0) |   // AAP  accept all
        (1 << 1) |   // APM  accept physical match
        (1 << 2) |   // AM   accept multicast
        (1 << 3) |   // AB   accept broadcast
        (1 << 7) |   // WRAP buffer wrap
        (4 << 8);    // MXDMA 256 B
    };

    extern RTL8139 driver;
}
