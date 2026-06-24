#include "xHCI.hpp"

#include <kernel/Sleep.hpp>
#include <kernel/Memory/heap.hpp>

#include "xHCI_common.hpp"
#include "xHCI_mem.hpp"
#include "std/printf.hpp"
#include "Drivers/PCI.hpp"
#include "xHCI_trb.hpp"
#include "xHCI_rings.hpp"
#include "arch/x86_64/IDT/IDT.hpp"
#include "xHCI_ext_cap.hpp"
#include "xHCI_device_ctx.hpp"
#include "xHCI_device.hpp"
#include "arch/x86_64/Common/Common.hpp"
#include "kernel/log.h"

namespace USB {
    xhci_driver m_xhci_driver;

    bool xhci_driver::init_device() {
        // Get xHCI device from PCI
        const PCI::PCI_Device usb = PCI::Find_Class(0x0030030C);

        // Get base mmio address
        if (usb.vendor_id == 0) {
            log::error("No xHCI Device found");
            return false;
        }

        const uint32_t bar0 = usb.bar[0];
        const uint32_t bar1 = usb.bar[1];

        if (bar0 & 1) {
            log::error("xHCI should not use IO BAR");
            return false;
        }
        const uint32_t type = (bar0 >> 1) & 0b11;

        if (type == 0b00) {
            // 32bit addr
            m_xhci_base = bar0 & ~0xFULL;
        } else if (type == 0b10) {
            // 64bit addr
            m_xhci_base = (static_cast<uint64_t>(bar1) << 32) | (bar0 & ~0xFULL);
        }

        xhci_map_mmio(m_xhci_base, 0x10000);

        Time::Sleep(100);

        _parse_capability_registers();
        //_log_capability_registers();
        //_log_operational_registers();

        _parse_extended_capability_registers();

        if (!_reset_host_controller()) {
            return false;
        }

        _configure_operational_register();
        //_log_operational_registers();

        _configure_runtime_registers();

        irq_number = 10;
        PCI::install_interrupt(usb, _xhci_irq_handler, irq_number);

        return true;
    }

    bool xhci_driver::start_device() {
        if (!_start_host_controller()) {
            log::error("Failed to start the host controller");
            return false;
        }

        //log::success("Controller started!\n");

        for (uint8_t port = 0; port < m_max_ports; port++) {
            xhci_portsc_register portsc = _read_portsc_reg(port);

            if (portsc.csc && portsc.ccs) {
                bool reset_successful = _reset_port(port);

                if (reset_successful) {
                    log::success("Device connected on port #&a%u &f- %s", port, _usb_speed_to_string(portsc.port_speed));
                    _setup_device(port);
                } else {
                    log::error("&cFailed &fto reset port #&a%u &fafter connection detection", port);
                }
            }
        }

        is_running = true;
        return true;
    }

    bool xhci_driver::shutdown_device() {
        is_running = false;
        return true;
    }

    void xhci_driver::_xhci_irq_handler(const IDT::ISR_Registers *regs) {
        _process_events();

        m_xhci_driver._acknowledge_irq(0);
    }

    void xhci_driver::_process_events() {
        std::vector<xhci_trb_t*> events;
        if (m_xhci_driver.m_event_ring->has_unprocessed_events()) {
            m_xhci_driver.m_event_ring->dequeue_events(events);
        }

        uint8_t command_completion_status = 0;

        for (size_t i = 0; i < events.size(); i++) {
            xhci_trb_t *event = events[i];
            switch (event->trb_type) {
                case XHCI_TRB_TYPE_CMD_COMPLETION_EVENT: {
                    command_completion_status = 1;
                    m_xhci_driver.m_command_completion_events.push_back(reinterpret_cast<xhci_command_completion_trb_t *>(event));
                    break;
                }

                default: break;
            }
        }
        m_xhci_driver.m_command_irq_completion = command_completion_status;
    }

    void xhci_driver::_parse_capability_registers() {
        m_cap_regs = reinterpret_cast<volatile xhci_capability_registers *>(m_xhci_base);

        m_capability_regs_length = m_cap_regs->caplength;

        m_max_device_slots = XHCI_MAX_DEVICE_SLOTS(m_cap_regs);
        m_max_interrupters = XHCI_MAX_INTERRUPTERS(m_cap_regs);
        m_max_ports = XHCI_MAX_PORTS(m_cap_regs);

        m_isochronous_scheduling_threshold = XHCI_IST(m_cap_regs);
        m_erst_max = XHCI_ERST_MAX(m_cap_regs);
        m_max_scratchpad_buffers = XHCI_MAX_SCRATCHPAD_BUFFERS(m_cap_regs);

        m_64bit_addressing_capability = XHCI_AC64(m_cap_regs);
        m_bandwidth_negotiation_capability = XHCI_BNC(m_cap_regs);
        m_64byte_context_size = XHCI_CSZ(m_cap_regs);
        m_port_power_control = XHCI_PPC(m_cap_regs);
        m_port_indicators = XHCI_PIND(m_cap_regs);
        m_light_reset_capability = XHCI_LHRC(m_cap_regs);
        m_extended_capabilities_offset = XHCI_XECP(m_cap_regs) * sizeof(uint32_t);

        m_op_regs = reinterpret_cast<volatile xhci_operational_registers*>(m_xhci_base + m_capability_regs_length);

        m_runtime_regs = reinterpret_cast<volatile xhci_runtime_registers*>(m_xhci_base + m_cap_regs->rtsoff);

        m_doorbell_manager = new xhci_doorbell_manager(m_xhci_base + m_cap_regs->dboff);
    }

    void xhci_driver::_parse_extended_capability_registers() {
        volatile uint32_t* head_cap_ptr = reinterpret_cast<volatile uint32_t*>(
            m_xhci_base + m_extended_capabilities_offset
        );

        m_extended_capabilities_head = new xhci_extended_capability(head_cap_ptr);

        auto node = m_extended_capabilities_head;

        while (node) {
            if (node->id() == xhci_extended_capability_code::supported_protocol) {
                xhci_usb_supported_protocol_capability cap(node->base());

                uint8_t first_port = cap.compatible_port_offset - 1;
                uint8_t last_port = first_port + cap.compatible_port_count - 1;

                if (cap.major_revision_version == 3) {
                    for (uint8_t port = first_port; port <= last_port; port++) {
                        m_usb3_ports.push_back(port);
                    }
                }
            }


            node = node->next();
        }
    }

    void xhci_driver::_log_capability_registers() {
        log::info("&f===== Xhci Capability Registers (&a%x&f) =====", reinterpret_cast<uint64_t>(m_cap_regs));
        log::info("&f    Length                : &a%i", m_capability_regs_length);
        log::info("&f    Max Device Slots      : &a%i", m_max_device_slots);
        log::info("&f    Max Interrupters      : &a%i", m_max_interrupters);
        log::info("&f    Max Ports             : &a%i", m_max_ports);
        log::info("&f    IST                   : &a%i", m_isochronous_scheduling_threshold);
        log::info("&f    ERST Max Size         : &a%i", m_erst_max);
        log::info("&f    Scratchpad Buffers    : &a%i", m_max_scratchpad_buffers);
        log::info("&f    64-bit Addressing     : &e%s", m_64bit_addressing_capability ? "yes" : "no");
        log::info("&f    Bandwidth Negotiation : &a%i", m_bandwidth_negotiation_capability);
        log::info("&f    64-byte Context Size  : &e%s", m_64byte_context_size ? "yes" : "no");
        log::info("&f    Port Power Control    : &a%i", m_port_power_control);
        log::info("&f    Port Indicators       : &a%i", m_port_indicators);
        log::info("&f    Light Reset Available : &a%i\n", m_light_reset_capability);
        //log::info("\n");
    }

    void xhci_driver::_log_operational_registers() {
        log::info("&7===== &fXhci Operational Registers (&a%x&f) &7=====", reinterpret_cast<uint64_t>(m_op_regs));
        log::info("&f    usbcmd     : &a%x", m_op_regs->usbcmd);
        log::info("&f    usbsts     : &a%x", m_op_regs->usbsts);
        log::info("&f    pagesize   : &a%x", m_op_regs->pagesize);
        log::info("&f    dnctrl     : &a%x", m_op_regs->dnctrl);
        log::info("&f    crcr       : &a%x", m_op_regs->crcr);
        log::info("&f    dcbaap     : &a%x", m_op_regs->dcbaap);
        log::info("&f    config     : &a%x\n", m_op_regs->config);
        //log::info("\n");
    }

    void xhci_driver::_log_usbsts() {
        const uint32_t status = m_op_regs->usbsts;
        log::info("===== USBSTS =====");
        if (status & XHCI_USBSTS_HCH)  log::info("    Host Controlled Halted");
        if (status & XHCI_USBSTS_HSE)  log::info("    Host System Error");
        if (status & XHCI_USBSTS_EINT) log::info("    Event Interrupt");
        if (status & XHCI_USBSTS_PCD)  log::info("    Port Change Detect");
        if (status & XHCI_USBSTS_SSS)  log::info("    Save State Status");
        if (status & XHCI_USBSTS_RSS)  log::info("    Restore State Status");
        if (status & XHCI_USBSTS_SRE)  log::info("    Save/Restore Error");
        if (status & XHCI_USBSTS_CNR)  log::info("    Controller Not Ready");
        if (status & XHCI_USBSTS_HCE)  log::info("    Host Controller Error\n");
        //log::info("\n");
    }

    xhci_portsc_register xhci_driver::_read_portsc_reg(uint8_t port_num) {
        uint64_t reg_base = reinterpret_cast<uint64_t>(m_xhci_driver.m_op_regs) + (0x400 + (0x10 * port_num));

        xhci_portsc_register reg;
        reg.raw = *reinterpret_cast<volatile uint32_t *>(reg_base);

        return reg;
    }

    void xhci_driver::_write_portsc_reg(xhci_portsc_register reg, uint8_t port_num) {
        uint64_t reg_base = reinterpret_cast<uint64_t>(m_op_regs) + (0x400 + (0x10 * port_num));
        *reinterpret_cast<volatile uint32_t *>(reg_base) = reg.raw;
    }

    bool xhci_driver::_is_usb3_port(uint8_t port_id) {
        for (size_t i = 0; i < m_usb3_ports.size(); i++) {
            if (m_usb3_ports[i] == port_id) {
                return true;
            }
        }
        return false;
    }

    bool xhci_driver::_reset_host_controller() {
        // Clear Run/Stop bit
        uint32_t usbcmd = m_op_regs->usbcmd;
        usbcmd &= ~XHCI_USBCMD_RUN_STOP;
        m_op_regs->usbcmd = usbcmd;

        // Wait for HCHalted bit to be set
        uint32_t timeout = 20; // 200ms timeout
        while (!(m_op_regs->usbsts & XHCI_USBSTS_HCH)) {
            if (--timeout <= 0) {
                log::error("Host controller did not halt within %ums", timeout);
                return false;
            }

            Time::Sleep(10);
        }

        // Set reset bit
        usbcmd = m_op_regs->usbcmd;
        usbcmd |= XHCI_USBCMD_HCRESET;
        m_op_regs->usbcmd = usbcmd;

        // Wait for Reset bit and CNR bit to clear
        timeout = 100; // 1000ms timeout
        while (m_op_regs->usbcmd & XHCI_USBCMD_HCRESET || m_op_regs->usbsts & XHCI_USBSTS_CNR) {
            if (--timeout == 0) {
                log::error("Host controller did not reset within %ums", timeout);
                return false;
            }

            Time::Sleep(10);
        }

        Time::Sleep(50);

        if (m_op_regs->usbcmd != 0)
            return false;

        if (m_op_regs->dnctrl != 0)
            return false;

        if (m_op_regs->crcr != 0)
            return false;

        if (m_op_regs->dcbaap != 0)
            return false;

        if (m_op_regs->config != 0)
            return false;

        return true;
    }

    bool xhci_driver::_start_host_controller() {
        uint32_t usbcmd = m_op_regs->usbcmd;
        usbcmd |= XHCI_USBCMD_RUN_STOP;
        usbcmd |= XHCI_USBCMD_INTERRUPTER_ENABLE;
        m_op_regs->usbcmd = usbcmd;

        uint32_t retries = 0;
        while (m_op_regs->usbsts & XHCI_USBSTS_HCH) {
            if (retries++ >= 100) {
                log::error("Host controller did not halt within %ums", retries);
                return false;
            }

            Time::Sleep(10);
        }

        if (m_op_regs->usbsts & XHCI_USBSTS_CNR) {
            return false;
        }

        return true;
    }

    void xhci_driver::_configure_operational_register() {
        m_op_regs->dnctrl = 0xffff;

        m_op_regs->config = static_cast<uint32_t>(m_max_device_slots);

        // Setup DCBAA
        _setup_dcbaa();

        m_command_ring = new xhci_command_ring(XHCI_COMMAND_RING_TRB_COUNT);
        m_op_regs->crcr = m_command_ring->get_physical_base() | m_command_ring->get_cycle_bit();
    }

    void xhci_driver::_setup_dcbaa() {
        uint64_t dcbaa_size = sizeof(uintptr_t) * (m_max_device_slots + 1);

        m_dcbaa = static_cast<uint64_t *>(alloc_xhci_memory(dcbaa_size, XHCI_DEVICE_CONTEXT_ALIGNMENT, XHCI_DEVICE_CONTEXT_BOUNDARY));
        m_dcbaa_virtual = static_cast<uint64_t *>(heap::malloc((m_max_device_slots + 1) * sizeof(uint64_t)));

        if (m_max_scratchpad_buffers > 0) {
            uint64_t *scrachpad_array = static_cast<uint64_t *>(
                alloc_xhci_memory(m_max_scratchpad_buffers * sizeof(uint64_t), XHCI_DEVICE_CONTEXT_ALIGNMENT, XHCI_DEVICE_CONTEXT_BOUNDARY));

            for (uint32_t i = 0; i < m_max_scratchpad_buffers; i++) {
                void *scrachpad = alloc_xhci_memory(PAGE_SIZE, XHCI_SCRATCHPAD_BUFFERS_ALIGNMENT, XHCI_SCRATCHPAD_BUFFERS_BOUNDARY);
                uint64_t scrachpad_addr = xhci_get_physical_addr(scrachpad);
                scrachpad_array[i] = scrachpad_addr;
            }

            uint64_t scrachpad_array_physical_base = xhci_get_physical_addr(scrachpad_array);
            m_dcbaa[0] = scrachpad_array_physical_base;

            m_dcbaa_virtual[0] = scrachpad_array_physical_base;
        }

        m_op_regs->dcbaap = xhci_get_physical_addr(m_dcbaa);
    }

    void xhci_driver::_configure_runtime_registers() {
        volatile xhci_interrupter_registers* interrupter_regs = &m_runtime_regs->ir[0];

        // Enable interrupts
        uint32_t iman = interrupter_regs->iman;
        iman |= XHCI_IMAN_INTERRUPT_ENABLE;
        interrupter_regs->iman = iman;

        m_event_ring = new xhci_event_ring(XHCI_EVENT_RING_TRB_COUNT, interrupter_regs);

        _acknowledge_irq(0);
    }

    void xhci_driver::_acknowledge_irq(const uint8_t interrupter) {
        m_op_regs->usbsts = XHCI_USBSTS_EINT;

        volatile xhci_interrupter_registers* interrupter_regs = &m_runtime_regs->ir[interrupter];

        uint32_t iman = interrupter_regs->iman;
        iman |= XHCI_IMAN_INTERRUPT_PENDING;
        interrupter_regs->iman = iman;
    }

    xhci_command_completion_trb_t *xhci_driver::_send_command_trb(xhci_trb_t* cmd_trb, const uint32_t timeout) {
        m_command_ring->enqueue(cmd_trb);
        m_doorbell_manager->ring_command_doorbell();

        uint64_t sleep_passed = 0;
        while (!m_command_irq_completion) {
            Time::Sleep(10);
            sleep_passed += 10;
            if (sleep_passed >= timeout) {
                log::error("Timeout");
                break;
            }
        }

        xhci_command_completion_trb_t* completion_trb = m_command_completion_events.size() ? m_command_completion_events[0] : nullptr;

        m_command_completion_events.clear();
        m_command_irq_completion = 0;

        if (!completion_trb) {
            std::kernel::printf("Failed to find completion TRB for command %i", cmd_trb->trb_type);
            return nullptr;
        }

        if (completion_trb->completion_code != XHCI_TRB_COMPLETION_CODE_SUCCESS) {
            log::error("Command TRB failed with error: %s", trb_completion_code_to_string(completion_trb->completion_code));
            return nullptr;
        }

        return completion_trb;
    }

    bool xhci_driver::_reset_port(uint8_t port_num) {
        xhci_portsc_register portsc = _read_portsc_reg(port_num);

        bool is_usb_3_port = _is_usb3_port(port_num);

        if (portsc.pp == 0) {
            portsc.pp = 1;
            _write_portsc_reg(portsc, port_num);
            Time::Sleep(20);
            portsc = _read_portsc_reg(port_num);

            if (portsc.pp == 0) {
                log::error("&cPort #&a%u &cFailed to power", port_num);
                return false;
            }
        }

        portsc.csc = 1;
        portsc.pec = 1;
        portsc.prc = 1;
        _write_portsc_reg(portsc, port_num);
        if (is_usb_3_port) {
            portsc.wpr = 1; // Usb 3 warm port reset (faster)
        } else {
            portsc.pr = 1; // Usb 2 port reset (slower)
        }
        _write_portsc_reg(portsc, port_num);
        int timeout = 10;
        while (timeout > 0) {
            portsc = _read_portsc_reg(port_num);

            if ((is_usb_3_port && portsc.wrc) || (!is_usb_3_port && portsc.prc)) {
                break;
            }
            timeout--;
            Time::Sleep(10);
        }

        if (timeout == 0) {
            log::error("Port #%au Port reset failed timed out", port_num);
            return false;
        }
        Time::Sleep(10); // to stabilize controller

        portsc.prc = 1; // Clear port reset change
        portsc.wrc = 1; // Clear warm reset change (USB 3.0)
        portsc.csc = 1; // Clear connect status change
        portsc.pec = 1; // Clear port enable/disable change
        portsc.ped = 0; // Don't clear the PED bit
        _write_portsc_reg(portsc, port_num);

        Time::Sleep(10); // to stabilize controller

        portsc = _read_portsc_reg(port_num);

        if (portsc.ped == 0) {
            return false;
        }

        return true;
    }

    const char *xhci_driver::_usb_speed_to_string(const uint8_t speed) {
        static const char* speed_string[7] = {
            "Invalid",
            "Full Speed (12 MB/s - USB2.0)",
            "Low Speed (1.5 Mb/s - USB 2.0)",
            "High Speed (480 Mb/s - USB 2.0)",
            "Super Speed (5 Gb/s - USB3.0)",
            "Super Speed Plus (10 Gb/s - USB 3.1)",
            "Undefined"
        };

        return speed_string[speed];
    }

    uint8_t xhci_driver::_get_port_speed(const uint8_t port) {
        const xhci_portsc_register portsc = _read_portsc_reg(port);
        return static_cast<uint8_t>(portsc.port_speed);
    }

    uint8_t xhci_driver::_enable_device_slot() {
        xhci_trb_t enable_slot_trb;
        mem::memset(&enable_slot_trb, 0, sizeof(xhci_trb_t));

        enable_slot_trb.trb_type = XHCI_TRB_TYPE_ENABLE_SLOT_CMD;

        const auto completion_trb = _send_command_trb(&enable_slot_trb);
        if (!completion_trb) {
            return 0;
        }

        return completion_trb->slot_id;
    }

    bool xhci_driver::_create_device_context(uint8_t slot_id) {
        const uint64_t device_context_size = m_64byte_context_size ? sizeof(xhci_device_context64) : sizeof(xhci_device_context32);

        void* ctx = alloc_xhci_memory(device_context_size,XHCI_DEVICE_CONTEXT_ALIGNMENT,XHCI_DEVICE_CONTEXT_BOUNDARY);

        if (!ctx) {
            log::error("Failed to allocate memory for a device context");
            return false;
        }

        m_dcbaa[slot_id] = xhci_get_physical_addr(ctx);

        m_dcbaa_virtual[slot_id] = reinterpret_cast<uint64_t>(ctx);

        return true;
    }

    void xhci_driver::_setup_device(uint8_t port) {
        uint8_t port_speed = _get_port_speed(port);
        uint8_t port_id = port + 1;

        uint8_t slot_id = _enable_device_slot();
        if (!slot_id) {
            log::error("Failed to enable device slot for port &a%i", port);
            return;
        }

        if (!_create_device_context(slot_id)) {
            log::error("Failed to create device context for slot &a%i", slot_id);
            return;
        }

        xhci_device* device = new xhci_device(port_id, slot_id, port_speed, m_64byte_context_size);

        log::info("Allocated device:");
        log::info("  port  - &a%i", device->get_port());
        log::info("  slot  - &a%i", device->get_slot());
        log::info("  speed - &a%s", _usb_speed_to_string(device->get_speed()));
        log::info("  inctx - &a%x\n", device->get_input_ctx_dma());
        //log::info("");

        device->set_root_port_id(port_id);
        device->set_output_ctx(reinterpret_cast<void *>(m_dcbaa[slot_id]));

        _enumerate_device(device);
    }

    void xhci_driver::_enumerate_device(xhci_device *device) {
        uint8_t port_id = device->get_port();
        uint8_t slot_id = device->get_slot();
        uint8_t port_speed = device->get_speed();

        bool is_root_device = (device->route_string() == 0);
        uint8_t port_index = port_id - 1;

        // Configure the input context for Address Device command
        uint16_t max_packet_size = _initial_max_packet_size(port_speed);
        _configure_ctrl_ep_input_context(device, max_packet_size);

        _address_device(device, true);

        usb_device_descriptor desc = {};
        int32_t rc = -1;
        for (uint8_t attempt = 0; attempt < 3; attempt++) {
            rc = _get_device_descriptor(device, &desc, 8);
            Time::Sleep(10);
        }
        if (rc != 0)
            log::error("xhci: failed to read device descriptor for slot %u", slot_id);

        log::info("%u", desc.iSerialNumber);
    }

    uint16_t xhci_driver::_initial_max_packet_size(uint8_t speed) {
        switch (speed) {
            case XHCI_USB_SPEED_LOW_SPEED:
                return 8;
            case XHCI_USB_SPEED_FULL_SPEED:
            case XHCI_USB_SPEED_HIGH_SPEED:
                return 64;
            case XHCI_USB_SPEED_SUPER_SPEED:
            case XHCI_USB_SPEED_SUPER_SPEED_PLUS:
                return 512;
            default:
                return 8;
        }
    }

    void xhci_driver::_configure_ctrl_ep_input_context(xhci_device* device, uint16_t max_packet_size) {
        size_t ctx_size = XHCI_CSZ(m_cap_regs)
            ? sizeof(xhci_input_context64)
            : sizeof(xhci_input_context32);
        mem::memset(device->get_input_ctrl_ctx(), 0, ctx_size);

        auto* input_ctrl = device->get_input_ctrl_ctx();
        auto* slot_ctx = device->get_input_slot_ctx();
        auto* ep0_ctx = device->get_input_ctrl_ep_ctx();

        // Enable slot context (A0) and control endpoint context (A1)
        input_ctrl->add_flags = (1 << 0) | (1 << 1);
        input_ctrl->drop_flags = 0;

        slot_ctx->route_string = device->route_string();
        slot_ctx->speed = device->get_speed();
        slot_ctx->context_entries = 1;
        slot_ctx->interrupter_target = 0;

        if (device->route_string() == 0) {
            // Root hub device: port_id is the root hub port number
            slot_ctx->root_hub_port_num = device->get_port();
        } else {
            log::error("skill issue #1");
            //// Hub-downstream device: use the root port of the topology chain
            //slot_ctx->root_hub_port_num = device->root_port_id();
            //
            //// xHCI spec Section 6.2.2: parent_hub_slot_id and parent_port_number
            //// shall reference the nearest HS hub providing the TT, only for LS/FS
            //// devices. Walk up the hub chain to find it.
            //if (device->get_speed() == XHCI_USB_SPEED_LOW_SPEED ||
            //    device->get_speed() == XHCI_USB_SPEED_FULL_SPEED) {
            //    auto* hub = m_slot_devices[device->parent_slot_id()];
            //    uint8_t port_on_hub = device->parent_port_num();
            //    while (hub && hub->speed() != XHCI_USB_SPEED_HIGH_SPEED
            //               && hub->parent_slot_id() != 0) {
            //        port_on_hub = hub->parent_port_num();
            //        hub = m_slot_devices[hub->parent_slot_id()];
            //    }
            //    if (hub && hub->speed() == XHCI_USB_SPEED_HIGH_SPEED) {
            //        slot_ctx->parent_hub_slot_id = hub->slot_id();
            //        slot_ctx->parent_port_number = port_on_hub;
            //        slot_ctx->mtt = hub->mtt() ? 1 : 0;
            //        log::info("xhci: slot %u TT: hub_slot=%u hub_port=%u mtt=%u",
            //                  device->slot_id(), hub->slot_id(), port_on_hub,
            //                  hub->mtt() ? 1 : 0);
            //    } else {
            //        log::info("xhci: slot %u TT: no HS hub found (parent slot %u speed=%u)",
            //                  device->slot_id(), device->parent_slot_id(),
            //                  hub ? hub->speed() : 0xFF);
            //    }
            //}
        }

        std::kernel::printf("xhci: slot %u input ctx: route=%x speed=%u root_port=%u mps=%u\n",
                  device->get_slot(), slot_ctx->route_string, slot_ctx->speed,
                  slot_ctx->root_hub_port_num, max_packet_size);

        ep0_ctx->endpoint_state = XHCI_ENDPOINT_STATE_DISABLED;
        ep0_ctx->endpoint_type = XHCI_ENDPOINT_TYPE_CONTROL;
        ep0_ctx->max_packet_size = max_packet_size;
        ep0_ctx->max_burst_size = 0;
        ep0_ctx->error_count = 3;
        ep0_ctx->interval = 0;
        ep0_ctx->average_trb_length = 8;
        ep0_ctx->max_esit_payload_lo = 0;
        ep0_ctx->transfer_ring_dequeue_ptr =
            device->ctrl_ring()->get_physical_base();
        ep0_ctx->dcs = device->ctrl_ring()->get_cycle_bit();
    }

    void xhci_driver::_address_device(xhci_device* device, bool bsr) {
        // Construct the Address Device TRB
        xhci_address_device_command_trb_t address_trb;
        address_trb.input_context_physical_base = device->get_input_ctx_dma();
        address_trb.rsvd = 0;
        address_trb.cycle_bit = 0;
        address_trb.rsvd1 = 0;

        /*
            Block Set Address Request (BSR). When this flag is set to '0' the Address Device Command shall
            generate a USB SET_ADDRESS request to the device. When this flag is set to '1' the Address
            Device Command shall not generate a USB SET_ADDRESS request. Refer to section 4.6.5 for
            more information on the use of this flag.
        */
        address_trb.bsr = bsr ? 1 : 0;

        address_trb.trb_type = XHCI_TRB_TYPE_ADDRESS_DEVICE_CMD;
        address_trb.rsvd2 = 0;
        address_trb.slot_id = device->get_slot();

        _send_command_trb(reinterpret_cast<xhci_trb_t*>(&address_trb));
    }

    int32_t xhci_driver::_get_device_descriptor(xhci_device* device, void* out, uint16_t length) {
        xhci_device_request_packet req = {};
        req.bRequestType = 0x80; // Device to Host, Standard, Device
        req.bRequest = 6;        // GET_DESCRIPTOR
        req.wValue = USB_DESCRIPTOR_REQUEST(USB_DESCRIPTOR_DEVICE, 0);
        req.wIndex = 0;
        req.wLength = length;

        _send_control_transfer(device, req, out, length);

        return 0;
    }

    int32_t xhci_driver::_send_control_transfer(xhci_device* device,xhci_device_request_packet& request,void* buffer, uint32_t length) {
    xhci_transfer_ring* ring = device->ctrl_ring();

    // Use the device's persistent DMA buffer
    void* dma_buffer = device->ctrl_transfer_buffer();
    uintptr_t dma_buffer_phys = device->ctrl_transfer_buffer_phys();
    if (!dma_buffer || dma_buffer_phys == 0) {
        log::error("xhci: missing control transfer buffer for slot %u", device->get_slot());
        return -1;
    }
    if (length > PAGE_SIZE) {
        log::error("xhci: control transfer too large (%u bytes)", length);
        return -1;
    }

    bool is_in = (request.transfer_direction != 0);

    // For OUT data stage, copy caller data into DMA buffer before enqueue
    if (length > 0 && !is_in && buffer) {
        mem::memcpy(dma_buffer, buffer, length);
    } else {
        mem::memset(dma_buffer, 0, length > 0 ? length : 1);
    }

    // Setup Stage TRB
    xhci_setup_stage_trb_t setup = {};
    setup.trb_type = XHCI_TRB_TYPE_SETUP_STAGE;
    setup.request_packet = request;
    setup.trb_transfer_length = 8;
    setup.interrupter_target = 0;
    setup.idt = 1;
    setup.ioc = 0;
    // TRT: 0=No Data, 2=OUT Data, 3=IN Data
    setup.trt = (length > 0) ? (is_in ? 3 : 2) : 0;

    // Data Stage TRB (if there's data to transfer)
    xhci_data_stage_trb_t data = {};
    if (length > 0) {
        data.trb_type = XHCI_TRB_TYPE_DATA_STAGE;
        data.data_buffer = dma_buffer_phys;
        data.trb_transfer_length = length;
        data.td_size = 0;
        data.interrupter_target = 0;
        data.dir = is_in ? 1 : 0;
        data.ioc = 0;
        data.idt = 0;
        data.chain = 0;
    }

    // Status Stage TRB (direction opposite to data stage)
    xhci_status_stage_trb_t status = {};
    status.trb_type = XHCI_TRB_TYPE_STATUS_STAGE;
    status.interrupter_target = 0;
    status.ioc = 1; // Interrupt on completion
    status.dir = (length > 0) ? (is_in ? 0 : 1) : 1;

    // Serialize EP0 enqueue+doorbell+wait against concurrent callers
    // (e.g. hub task and HCD task both sending control transfers to the hub).

    //device->set_ctrl_completed(false);

    ring->enqueue(reinterpret_cast<xhci_trb_t*>(&setup));
    if (length > 0) {
        ring->enqueue(reinterpret_cast<xhci_trb_t*>(&data));
    }
    ring->enqueue(reinterpret_cast<xhci_trb_t*>(&status));

    //// VL805 quirk: avoid ringing the doorbell near the SOF boundary for
    //// FS non-periodic transfers behind a hub (TT babble avoidance).
    if (device->route_string() != 0 &&
        device->get_speed() == XHCI_USB_SPEED_FULL_SPEED) {
        for (uint32_t tries = 0; tries < 20; tries++) {
            if ((_read_mfindex() & 0x7) != 0)
                break;
            Time::Sleep(10);
        }
    }

    m_doorbell_manager->ring_doorbell(device->get_slot(), XHCI_DOORBELL_TARGET_CONTROL_EP_RING);

    constexpr uint64_t XFER_TIMEOUT_MS = 5;
    uint64_t deadline = Time::tick + XFER_TIMEOUT_MS * 1ULL;

    _process_events();
    m_event_ring->flush_unprocessed_events();

    while (Time::tick < deadline) {
        Time::Sleep(100);
        _process_events();
        m_event_ring->flush_unprocessed_events();
    }

    if (buffer && length > 0 && is_in) {
        mem::memcpy(buffer, dma_buffer, length);
    }

    return 0;
}

    uint32_t xhci_driver::_read_mfindex() const {
        if (!m_runtime_regs) {
            return 0xffffu;
        }
        return m_runtime_regs->mf_index & 0x3fff;
    }
}
