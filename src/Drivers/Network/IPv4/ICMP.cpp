#include "ICMP.hpp"
#include "Drivers/Network/Common.hpp"
#include "Drivers/Network/Net_Device.hpp"
#include "kernel/log.h"
#include "std/mem_common.hpp"

namespace NET {
    void receive_ICMP(Net_Device *dev, IPv4Packet *packet) {
        const uint8_t ip_header_len = (packet->ip.ihl_version & 0x0F) * 4;

        auto* frame = reinterpret_cast<uint8_t *>(packet);

        auto* icmp = reinterpret_cast<ICMPHeader *>(frame + 14 + ip_header_len);

        char buf[16];
        ipv4_to_str(packet->ip.src_ip, buf, true);
        log::info("icmp from: %s", buf);
        if (icmp->type == 8) { // Echo request
            icmp->type = 0; // Reply
            icmp->code = 0;

            const uint32_t tmp_ip = packet->ip.src_ip;
            packet->ip.src_ip = packet->ip.dst_ip;
            packet->ip.dst_ip = tmp_ip;

            uint8_t tmp_mac[6];
            mem::memcpy(tmp_mac, packet->eth.dst_mac, 6);
            mem::memcpy(packet->eth.dst_mac, packet->eth.src_mac, 6);
            mem::memcpy(packet->eth.src_mac, tmp_mac, 6);


            packet->ip.checksum = 0;
            packet->ip.checksum = checksum(&packet->ip, ip_header_len);

            icmp->checksum = 0;
            icmp->checksum = checksum(icmp, Bswap_16(packet->ip.total_length) - ip_header_len);

            dev->send(reinterpret_cast<uint8_t *>(packet), Bswap_16(packet->ip.total_length) + 14);
        }
    }
}
