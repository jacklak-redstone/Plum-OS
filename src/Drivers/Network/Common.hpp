#pragma once
#include "kernel/log.h"
#include "std/types.hpp"

namespace NET {
    constexpr uint16_t ARP_Ether_Type = 0x0806;
    constexpr uint16_t IPv4_Ether_Type = 0x0800;
    constexpr uint16_t IPv4_Protocol_ICMP = 1;
    constexpr uint16_t IPv4_Protocol_UDP = 17;

    static uint16_t Bswap_16(const uint16_t x) {
        return (x << 8) | (x >> 8);
    }

    static uint32_t Bswap_32(const uint32_t x) {
        return ((x << 24) & 0xFF000000) |
               ((x << 8)  & 0x00FF0000) |
               ((x >> 8)  & 0x0000FF00) |
               ((x >> 24) & 0x000000FF);
    }

    // 192, 168, 0, 1
    constexpr uint32_t make_ipv4(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) {
        return (static_cast<uint32_t>(a) << 24) |
            (static_cast<uint32_t>(b) << 16) |
            (static_cast<uint32_t>(c) << 8 ) |
            (static_cast<uint32_t>(d));
    }

    static void ipv4_to_str(uint32_t ip, char *out, bool reverse = false) {
        if (reverse) {
            ip = Bswap_32(ip);
        }
        const uint8_t a = (ip >> 24) & 0xFF;
        const uint8_t b = (ip >> 16) & 0xFF;
        const uint8_t c = (ip >> 8) & 0xFF;
        const uint8_t d = (ip) & 0xFF;

        char* p = out;

        auto write_num = [&](uint8_t v) {
            if (v >= 100) {
                *p++ = '0' + (v / 100);
                v %= 100;
            }
            if (v >= 10) {
                *p++ = '0' + (v / 10);
                v %= 10;
            }
            *p++ = '0' + v;
        };

        write_num(a); *p++ = '.';
        write_num(b); *p++ = '.';
        write_num(c); *p++ = '.';
        write_num(d); *p++ = '\0';
    }

    static uint16_t checksum(const void* data, int len) {
        const auto* ptr = static_cast<const uint16_t *>(data);

        uint32_t sum = 0;

        while (len > 1) {
            sum += *ptr++;
            len -= 2;
        }

        if (len == 1) {
            sum += *(uint8_t*)ptr;
        }

        // fold 32 -> 16
        while (sum >> 16) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }

        return static_cast<uint16_t>(~sum);
    }

    struct EthernetHeader {
        uint8_t dst_mac[6];
        uint8_t src_mac[6];
        uint16_t ethertype;
    } __attribute__((packed));

    // ----------------------------------------------
    // ARP
    struct ARPHeader {
        uint16_t hardware_type; // Ethernet = 1
        uint16_t protocol_type; // IPv4 = 0x0800
        uint8_t mac_len; // 6
        uint8_t ip_len; // 4
        uint16_t opcode; // request=1, reply=2

        uint8_t sender_mac[6];
        uint32_t sender_ip;

        uint8_t target_mac[6];
        uint32_t target_ip;
    } __attribute__((packed));

    struct ARPPacket {
        EthernetHeader eth;
        ARPHeader arp;
    } __attribute__((packed));

    // ----------------------------------------------
    // IPv4
    struct IPv4Header {
        uint8_t  ihl_version;
        uint8_t  tos;
        uint16_t total_length;
        uint16_t id;
        uint16_t flags_frag;
        uint8_t  ttl;
        uint8_t  protocol;
        uint16_t checksum;
        uint32_t src_ip;
        uint32_t dst_ip;
    } __attribute__((packed));

    struct IPv4Packet {
        EthernetHeader eth;
        IPv4Header ip;
    } __attribute__((packed));

    struct ICMPHeader {
        uint8_t type;
        uint8_t code;
        uint16_t checksum;
    } __attribute__((packed));

    struct ICMPPacket {
        uint8_t type;
        uint8_t code;
        uint16_t checksum;
        uint16_t id;
        uint16_t seq;
        uint8_t data[32];
    } __attribute__((packed));

    struct UDPHeader {
        uint16_t src_port;
        uint16_t dst_port;
        uint16_t length;
        uint16_t checksum;
    } __attribute__((packed));
}
