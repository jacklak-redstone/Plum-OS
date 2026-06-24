#pragma once
#include <std/types.hpp>

#include "xHCI_device.hpp"
#include "xHCI_regs.hpp"
#include "xHCI_rings.hpp"
#include "arch/x86_64/IDT/IDT.hpp"

namespace USB {
    constexpr uint16_t USB_DESCRIPTOR_REQUEST(uint8_t type, uint8_t index) {
        return (type << 8) | index;
    }

    constexpr uint8_t USB_DESCRIPTOR_DEVICE                          = 0x01;

    struct usb_descriptor_header {
        uint8_t bLength;
        uint8_t bDescriptorType;
    } __attribute__((packed));
    static_assert(sizeof(usb_descriptor_header) == 2);

    struct usb_device_descriptor {
        usb_descriptor_header header;
        uint16_t bcdUsb;
        uint8_t bDeviceClass;
        uint8_t bDeviceSubClass;
        uint8_t bDeviceProtocol;
        uint8_t bMaxPacketSize0;
        uint16_t idVendor;
        uint16_t idProduct;
        uint16_t bcdDevice;
        uint8_t iManufacturer;
        uint8_t iProduct;
        uint8_t iSerialNumber;
        uint8_t bNumConfigurations;
    } __attribute__((packed));
    static_assert(sizeof(usb_device_descriptor) == 18);

    class xhci_driver {
    public:
        bool init_device();
        bool start_device();
        bool shutdown_device();

        uintptr_t m_xhci_base;
    private:
        uint8_t irq_number;

        volatile xhci_capability_registers *m_cap_regs;
        volatile xhci_operational_registers *m_op_regs;
        volatile xhci_runtime_registers *m_runtime_regs;

        xhci_extended_capability *m_extended_capabilities_head = nullptr;

        // CAPLENGTH
        uint8_t m_capability_regs_length;

        // HCSPARAMS1
        uint8_t m_max_device_slots;
        uint8_t m_max_interrupters;
        uint8_t m_max_ports;

        // HCSPARAMS2
        uint8_t m_isochronous_scheduling_threshold;
        uint8_t m_erst_max;
        uint8_t m_max_scratchpad_buffers;

        // hccparams1
        bool m_64bit_addressing_capability;
        bool m_bandwidth_negotiation_capability;
        bool m_64byte_context_size;
        bool m_port_power_control;
        bool m_port_indicators;
        bool m_light_reset_capability;
        uint32_t m_extended_capabilities_offset;

        uint64_t* m_dcbaa;
        uint64_t* m_dcbaa_virtual;

        xhci_command_ring *m_command_ring = nullptr;
        xhci_event_ring *m_event_ring = nullptr;
        xhci_doorbell_manager *m_doorbell_manager = nullptr;

        std::vector<xhci_command_completion_trb_t*> m_command_completion_events;

        volatile uint8_t m_command_irq_completion = 0;

        std::vector<uint8_t> m_usb3_ports;

        bool is_running = false;

    private:
        static void _xhci_irq_handler(const IDT::ISR_Registers *regs);
        static void _process_events();

        void _parse_capability_registers();
        void _parse_extended_capability_registers();

        void _log_capability_registers();
        void _log_operational_registers();

        void _log_usbsts();

        // port number is 0-based
        static xhci_portsc_register _read_portsc_reg(uint8_t port_num);

        // port number is 0-based
        void _write_portsc_reg(xhci_portsc_register reg, uint8_t port_num);

        bool _is_usb3_port(uint8_t port_id);

        bool _reset_host_controller();
        bool _start_host_controller();

        void _configure_operational_register();
        void _setup_dcbaa();

        void _configure_runtime_registers();
        void _acknowledge_irq(uint8_t interrupter);

        xhci_command_completion_trb_t *_send_command_trb(xhci_trb_t* cmd_trb, uint32_t timeout = 200);

        // port number is 0-based
        bool _reset_port(uint8_t port_num);

        const char* _usb_speed_to_string(uint8_t speed);
        uint8_t _get_port_speed(uint8_t port);

        uint8_t _enable_device_slot();

        // Creates a device context buffer and inserts it into DCBAA
        bool _create_device_context(uint8_t slot_id);

        // port is 0-based
        void _setup_device(uint8_t port);
        void _enumerate_device(xhci_device *device);

        uint16_t _initial_max_packet_size(uint8_t speed);
        void _configure_ctrl_ep_input_context(xhci_device* device, uint16_t max_packet_size);

        void _address_device(xhci_device* device, bool bsr);

        int32_t _get_device_descriptor(xhci_device* device, void* out, uint16_t length);

        int32_t _send_control_transfer(xhci_device* device,xhci_device_request_packet& request,void* buffer, uint32_t length);

        uint32_t _read_mfindex() const;
    };

    extern xhci_driver m_xhci_driver;
}
