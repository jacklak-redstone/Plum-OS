#pragma once
#include "Net_Device.hpp"
#include "std/types.hpp"

namespace NET {
    void receive_ethernet(Net_Device* dev,const uint8_t* frame,uint16_t len);
}