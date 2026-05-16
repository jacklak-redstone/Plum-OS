#include "IPv4.hpp"
#include "Drivers/Network/Common.hpp"
#include "Drivers/Network/Net_Device.hpp"
#include "Drivers/Network/IPv4/ICMP.hpp"

namespace NET {
    void receive_IPv4(Net_Device *dev, IPv4Packet *packet) {
        auto ip = &packet->ip;
        if (ip->dst_ip != Bswap_32(make_ipv4(10,0,0,2))) return;

        switch (ip->protocol) {
            case IPv4_Protocol_ICMP: {
                receive_ICMP(dev, packet);
                return;
            }
            case IPv4_Protocol_UDP: {
                return;
            }
            default:
                return;
        }
    }
}
