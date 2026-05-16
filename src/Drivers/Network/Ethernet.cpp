#include "Ethernet.hpp"

#include "Common.hpp"
#include "kernel/log.h"
#include "std/types.hpp"
#include "std/mem_common.hpp"
#include "IPv4/IPv4.hpp"

namespace NET {
    void receive_ethernet(Net_Device *dev, const uint8_t *frame, uint16_t len) {
        auto Ether_header = (EthernetHeader *)frame;

        switch (Bswap_16(Ether_header->ethertype)) {
            case ARP_Ether_Type: {
                const auto packet = (ARPPacket *)frame;
                // Check if ARP is to us
                if (packet->arp.target_ip != Bswap_32(make_ipv4(10,0,0,2))) return;
                if (packet->arp.opcode == Bswap_16(2)) return;

                char buf[16];
                ipv4_to_str(packet->arp.sender_ip, buf, true);
                log::info("icmp from: %s", buf);
                log::info("[ NET ] ARP from: %s", buf);
                ARPPacket reply{};

                mem::memcpy(reply.eth.dst_mac, packet->eth.src_mac, 6);     // To sender
                mem::memcpy(reply.eth.src_mac, dev->get_mac(), 6);      // From me

                reply.eth.ethertype = Bswap_16(ARP_Ether_Type); // 0x0806

                reply.arp.hardware_type = Bswap_16(1); // 1 - Ethernet

                reply.arp.protocol_type = Bswap_16(IPv4_Ether_Type); // 0x0800

                reply.arp.mac_len = 6;  // xx.xx.xx.xx.xx.xx MAC
                reply.arp.ip_len = 4;   // xx.xx.xx.xx IPv4

                reply.arp.opcode = Bswap_16(2); // 2 - Reply

                mem::memcpy(reply.arp.sender_mac, dev->get_mac(), 6);       // Me MAC
                reply.arp.sender_ip = Bswap_32(make_ipv4(10, 0, 0, 2));   // My ip 10.0.0.2

                mem::memcpy(reply.arp.target_mac, packet->eth.src_mac, 6);      // Sender MAC
                reply.arp.target_ip = packet->arp.sender_ip;                        // Sender IP

                dev->send(reinterpret_cast<uint8_t *>(&reply), sizeof(ARPPacket));
                return;;
            }

            case IPv4_Ether_Type: {
                receive_IPv4(dev, (IPv4Packet *)frame);
                return;;
            }

            default:
                log::info("[ NET ] Unknown: %x", static_cast<uint64_t>(Bswap_16(Ether_header->ethertype)));
        }
    }
}
