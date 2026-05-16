#pragma once
#include "Drivers/Network/Common.hpp"
#include "Drivers/Network/Net_Device.hpp"

namespace NET {
    void receive_IPv4(Net_Device *dev, IPv4Packet *packet);
}
