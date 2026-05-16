#pragma once
#include "Drivers/Network/Common.hpp"
#include "Drivers/Network/Net_Device.hpp"

namespace NET {
    void receive_ICMP(Net_Device *dev, IPv4Packet *packet);
}
